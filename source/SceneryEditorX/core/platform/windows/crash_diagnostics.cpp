/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * crash_diagnostics.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "crash_diagnostics.h"
#include <SceneryEditorX/filesystem/file_system.h>
#include <SceneryEditorX/logging/logging.hpp>

#ifdef SEDX_PLATFORM_WINDOWS
	#include <Windows.h>
	#include <DbgHelp.h>
	#include <crtdbg.h>
	#include <csignal>
	#include <cstdio>
	#include <cstring>
	#include <exception>
	#include <format>
	#include <mutex>
	#include <share.h>
	#pragma comment(lib, "dbghelp.lib")
#endif

// -------------------------------------------------------

namespace SceneryEditorX::Diagnostics
{
#ifdef SEDX_PLATFORM_WINDOWS
	namespace
	{
		std::mutex s_ReportMutex;
		bool s_Installed = false;

		/* SceneryEditorX.log, for crashes after Log::ShutDown() (static destructors). A plain array: it must outlive them. */
		wchar_t s_FallbackLogPath[MAX_PATH] = {};
		FILE *s_FallbackLog = nullptr;

		void FlushLogs()
		{
			for (const std::shared_ptr<spdlog::logger> *logger : { &Log::GetCoreLogger(), &Log::GetEditorLogger(), &Log::GetEditorConsoleLogger() })
			{
				if (*logger)
					(*logger)->flush();
			}
		}

		/* Writes one report line: through the engine log while it is up, otherwise appended to the log file directly. */
		template <typename... Args>
		void Emit(const std::format_string<Args...> format, Args &&...args)
		{
			if (Log::GetCoreLogger())
			{
				SEDX_CORE_FATAL_TAG("Crash", "{}", std::format(format, std::forward<Args>(args)...));
				return;
			}

			SYSTEMTIME time{};
			GetLocalTime(&time);
			const std::string line = std::format("[{:02}:{:02}:{:02}] [critical] Core: [Crash] {}\n", time.wHour, time.wMinute, time.wSecond,
				std::format(format, std::forward<Args>(args)...));

			OutputDebugStringA(line.c_str());
			if (!s_FallbackLog && s_FallbackLogPath[0] != L'\0')
				s_FallbackLog = _wfsopen(s_FallbackLogPath, L"ab", _SH_DENYNO);
			if (s_FallbackLog)
				std::fputs(line.c_str(), s_FallbackLog);
		}

		bool SymbolsReady()
		{
			static const bool s_SymbolsReady = []
			{
				SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
				return SymInitialize(GetCurrentProcess(), nullptr, TRUE) == TRUE;
			}();
			return s_SymbolsReady;
		}

		/* Logs one frame as symbol + source line, or module + offset when there are no symbols for it. */
		void LogFrame(const unsigned index, const DWORD64 address)
		{
			const HANDLE process = GetCurrentProcess();
			alignas(SYMBOL_INFO) char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
			auto *symbol = reinterpret_cast<SYMBOL_INFO *>(symbolBuffer);

			const char *name = "?";
			if (SymbolsReady())
			{
				symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
				symbol->MaxNameLen = MAX_SYM_NAME;
				DWORD64 displacement = 0;
				if (SymFromAddr(process, address, &displacement, symbol))
					name = symbol->Name;
			}

			IMAGEHLP_LINE64 line{};
			line.SizeOfStruct = sizeof(line);
			DWORD lineDisplacement = 0;
			if (SymbolsReady() && SymGetLineFromAddr64(process, address, &lineDisplacement, &line))
			{
				Emit("  #{:02} {} ({}:{})", index, name, line.FileName, line.LineNumber);
				return;
			}

			HMODULE module = nullptr;
			char modulePath[MAX_PATH] = {};
			if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
					reinterpret_cast<LPCSTR>(address), &module) && GetModuleFileNameA(module, modulePath, MAX_PATH))
			{
				const char *moduleName = std::strrchr(modulePath, '\\');
				Emit("  #{:02} {} ({}+0x{:x})", index, name, moduleName ? moduleName + 1 : modulePath, address - reinterpret_cast<DWORD64>(module));
			}
			else
			{
				Emit("  #{:02} {} (0x{:x}, not in a loaded module)", index, name, address);
			}
		}

		/* Logs the calling thread's stack, resolving symbols and source lines from the PDBs when available. */
		void LogStack(const unsigned long framesToSkip)
		{
			void *frames[64];
			const USHORT frameCount = CaptureStackBackTrace(framesToSkip + 1, static_cast<DWORD>(std::size(frames)), frames, nullptr);
			for (USHORT i = 0; i < frameCount; ++i)
				LogFrame(i, reinterpret_cast<DWORD64>(frames[i]));
		}

		/*
		 * Logs the stack of a faulting thread from its exception context. Unlike CaptureStackBackTrace from inside the
		 * filter, this starts at the faulting instruction and walks through frames without unwind data (code in a module
		 * that was already unloaded, JIT code), so the callers of a bad call still show up.
		 */
		void LogStack(const CONTEXT &faultContext)
		{
	#if defined(_M_X64)
			const HANDLE process = GetCurrentProcess();
			SymbolsReady();

			CONTEXT context = faultContext;
			STACKFRAME64 frame{};
			frame.AddrPC.Offset = context.Rip;
			frame.AddrPC.Mode = AddrModeFlat;
			frame.AddrFrame.Offset = context.Rbp;
			frame.AddrFrame.Mode = AddrModeFlat;
			frame.AddrStack.Offset = context.Rsp;
			frame.AddrStack.Mode = AddrModeFlat;

			for (unsigned i = 0; i < 64; ++i)
			{
				if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, GetCurrentThread(), &frame, &context, nullptr,
						SymFunctionTableAccess64, SymGetModuleBase64, nullptr) || frame.AddrPC.Offset == 0)
					break;
				LogFrame(i, frame.AddrPC.Offset);
			}
	#else
			(void)faultContext;
			LogStack(2);
	#endif
		}

		void Report(const char *reason, const unsigned long framesToSkip, const CONTEXT *faultContext = nullptr)
		{
			std::lock_guard lock(s_ReportMutex);
			Emit("{} (thread {})", reason, GetCurrentThreadId());
			if (faultContext)
				LogStack(*faultContext);
			else
				LogStack(framesToSkip + 1);
			FlushLogs();
			if (s_FallbackLog)
				std::fflush(s_FallbackLog);
		}

		void OnAbortSignal(int)
		{
			Report("abort() called", 1);
		}

		void OnTerminate()
		{
			std::string reason = "std::terminate called";
			if (const std::exception_ptr exception = std::current_exception())
			{
				try
				{
					std::rethrow_exception(exception);
				}
				catch (const std::exception &e)
				{
					reason += std::string(": uncaught exception: ") + e.what();
				}
				catch (...)
				{
					reason += ": uncaught non-std exception";
				}
			}

			Report(reason.c_str(), 1);
			std::abort();
		}

		int OnCrtReport(const int reportType, char *message, int *returnValue)
		{
			if (reportType == _CRT_ASSERT || reportType == _CRT_ERROR)
			{
				std::string reason = reportType == _CRT_ASSERT ? "CRT assertion failed: " : "CRT error: ";
				reason += message ? message : "";
				Report(reason.c_str(), 1);
			}

			if (returnValue)
				*returnValue = 0;
			return FALSE; // Let the CRT carry on with its default behaviour
		}

		LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS *info)
		{
			char reason[160];
			snprintf(reason, sizeof(reason), "Unhandled exception 0x%08lX at %p",
				info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionCode : 0ul,
				info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionAddress : nullptr);
			Report(reason, 0, info ? info->ContextRecord : nullptr);
			return EXCEPTION_CONTINUE_SEARCH;
		}
	}

	void InstallCrashHandlers()
	{
		if (s_Installed)
			return;

		s_Installed = true;
		const std::wstring logPath = (IO::FileSystem::GetAppDataRoaming() / "logs" / "SceneryEditorX.log").wstring();
		if (logPath.size() < std::size(s_FallbackLogPath))
			wcscpy_s(s_FallbackLogPath, logPath.c_str());

		std::signal(SIGABRT, OnAbortSignal);
		std::set_terminate(OnTerminate);
		_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, OnCrtReport);
		SetUnhandledExceptionFilter(OnUnhandledException);
	}
#else
	void InstallCrashHandlers() {}
#endif
}

// -------------------------------------------------------

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
#include <SceneryEditorX/logging/logging.hpp>

#ifdef SEDX_PLATFORM_WINDOWS
	#include <Windows.h>
	#include <DbgHelp.h>
	#include <crtdbg.h>
	#include <csignal>
	#include <exception>
	#include <mutex>
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

		void FlushLogs()
		{
			for (const std::shared_ptr<spdlog::logger> *logger : { &Log::GetCoreLogger(), &Log::GetEditorLogger(), &Log::GetEditorConsoleLogger() })
			{
				if (*logger)
					(*logger)->flush();
			}
		}

		/* Logs the calling thread's stack, resolving symbols and source lines from the PDBs when available. */
		void LogStack(const unsigned long framesToSkip)
		{
			const HANDLE process = GetCurrentProcess();
			static const bool s_SymbolsReady = []
			{
				SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
				return SymInitialize(GetCurrentProcess(), nullptr, TRUE) == TRUE;
			}();

			void *frames[64];
			const USHORT frameCount = CaptureStackBackTrace(framesToSkip + 1, static_cast<DWORD>(std::size(frames)), frames, nullptr);

			alignas(SYMBOL_INFO) char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
			auto *symbol = reinterpret_cast<SYMBOL_INFO *>(symbolBuffer);

			for (USHORT i = 0; i < frameCount; ++i)
			{
				const DWORD64 address = reinterpret_cast<DWORD64>(frames[i]);
				const char *name = "?";
				if (s_SymbolsReady)
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
				if (s_SymbolsReady && SymGetLineFromAddr64(process, address, &lineDisplacement, &line))
					SEDX_CORE_FATAL_TAG("Crash", "  #{:02} {} ({}:{})", i, name, line.FileName, line.LineNumber);
				else
					SEDX_CORE_FATAL_TAG("Crash", "  #{:02} {} (0x{:x})", i, name, address);
			}
		}

		void Report(const char *reason, const unsigned long framesToSkip)
		{
			std::lock_guard lock(s_ReportMutex);
			SEDX_CORE_FATAL_TAG("Crash", "{} (thread {})", reason, GetCurrentThreadId());
			LogStack(framesToSkip + 1);
			FlushLogs();
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
			Report(reason, 0);
			return EXCEPTION_CONTINUE_SEARCH;
		}
	}

	void InstallCrashHandlers()
	{
		if (s_Installed)
			return;

		s_Installed = true;
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

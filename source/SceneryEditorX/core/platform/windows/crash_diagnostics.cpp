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
	#include <Psapi.h>
	#include <TlHelp32.h>
	#include <crtdbg.h>
	#include <algorithm>
	#include <csignal>
	#include <cstdio>
	#include <cstring>
	#include <exception>
	#include <format>
	#include <mutex>
	#include <share.h>
	#include <string>
	#include <vector>
	#pragma comment(lib, "dbghelp.lib")
	#pragma comment(lib, "psapi.lib")
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

		/* Minidump of the process written next to the log on an unhandled exception (open it in Visual Studio). */
		wchar_t s_DumpPath[MAX_PATH] = {};

		/* Frames logged for each thread other than the crashing one, and how much of its stack is copied for them. */
		constexpr unsigned OTHER_THREAD_FRAMES = 24;
		constexpr size_t STACK_COPY_BYTES = 128 * 1024;

		/* Another thread's registers and the top of its stack, copied while it was suspended so it can be walked later. */
		struct ThreadSnapshot
		{
			DWORD id = 0;
			CONTEXT context{};
			std::vector<unsigned char> stack; // [context.Rsp, context.Rsp + stack.size())
		};

		/* The snapshot being walked (under s_ReportMutex): ReadSnapshotMemory serves its stack from the copy. */
		const ThreadSnapshot *s_WalkedSnapshot = nullptr;

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
		 * that was already unloaded, JIT code), so the callers of a bad call still show up. readMemory replaces
		 * ReadProcessMemory for the walk (another thread's stack is read from a copy, see ThreadSnapshot).
		 */
		void LogStack(const CONTEXT &faultContext, const unsigned maxFrames = 64, const PREAD_PROCESS_MEMORY_ROUTINE64 readMemory = nullptr)
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

			for (unsigned i = 0; i < maxFrames; ++i)
			{
				if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, GetCurrentThread(), &frame, &context, readMemory,
						SymFunctionTableAccess64, SymGetModuleBase64, nullptr) || frame.AddrPC.Offset == 0)
					break;
				LogFrame(i, frame.AddrPC.Offset);
			}
	#else
			(void)faultContext;
			(void)maxFrames;
			(void)readMemory;
			LogStack(2);
	#endif
		}

		std::string ToUtf8(const wchar_t *text)
		{
			std::string out;
			const int bytes = text ? WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr) : 0;
			if (bytes > 1)
			{
				out.resize(static_cast<size_t>(bytes) - 1);
				WideCharToMultiByte(CP_UTF8, 0, text, -1, out.data(), bytes, nullptr, nullptr);
			}
			return out;
		}

		/* StackWalk64 memory reader: the walked snapshot's stack comes from its copy, code and unwind data from memory. */
		BOOL CALLBACK ReadSnapshotMemory(const HANDLE process, const DWORD64 address, const PVOID buffer, const DWORD size, const LPDWORD bytesRead)
		{
	#if defined(_M_X64)
			if (const ThreadSnapshot *snapshot = s_WalkedSnapshot)
			{
				const DWORD64 stackStart = snapshot->context.Rsp;
				if (address >= stackStart && address + size <= stackStart + snapshot->stack.size())
				{
					std::memcpy(buffer, snapshot->stack.data() + (address - stackStart), size);
					if (bytesRead)
						*bytesRead = size;
					return TRUE;
				}
			}
	#endif

			SIZE_T read = 0;
			const BOOL ok = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(address), buffer, size, &read);
			if (bytesRead)
				*bytesRead = static_cast<DWORD>(read);
			return ok;
		}

		/*
		 * Registers and stack tops of the process's other threads. Each one is suspended only for GetThreadContext and
		 * ReadProcessMemory, which take no user-mode locks (a thread stopped inside the heap can't deadlock this), and all
		 * are taken before symbols load (which takes seconds), so they show what ran when the crashing thread faulted.
		 */
		std::vector<ThreadSnapshot> CaptureOtherThreads()
		{
			std::vector<ThreadSnapshot> snapshots;
	#if defined(_M_X64)
			const HANDLE toolhelp = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
			if (toolhelp == INVALID_HANDLE_VALUE)
				return snapshots;

			const DWORD processId = GetCurrentProcessId();
			const DWORD self = GetCurrentThreadId();
			THREADENTRY32 entry{};
			entry.dwSize = sizeof(entry);
			for (BOOL more = Thread32First(toolhelp, &entry); more; more = Thread32Next(toolhelp, &entry))
			{
				if (entry.th32OwnerProcessID != processId || entry.th32ThreadID == self)
					continue;

				const HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, entry.th32ThreadID);
				if (!thread)
					continue;

				ThreadSnapshot snapshot;
				snapshot.id = entry.th32ThreadID;
				snapshot.context.ContextFlags = CONTEXT_FULL;
				snapshot.stack.resize(STACK_COPY_BYTES); // Before the thread stops: the heap lock may be what it holds

				bool captured = false;
				if (SuspendThread(thread) != static_cast<DWORD>(-1))
				{
					captured = GetThreadContext(thread, &snapshot.context) != FALSE;
					size_t copied = 0;
					while (captured && copied < snapshot.stack.size())
					{
						// Page by page: the copy ends where the stack does (the first unreadable page).
						const DWORD64 at = snapshot.context.Rsp + copied;
						const size_t chunk = std::min<size_t>(snapshot.stack.size() - copied, 0x1000 - static_cast<size_t>(at & 0xFFF));
						SIZE_T read = 0;
						if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(at), snapshot.stack.data() + copied, chunk, &read) || read != chunk)
							break;
						copied += chunk;
					}
					ResumeThread(thread);
					snapshot.stack.resize(copied);
				}
				CloseHandle(thread);

				if (captured)
					snapshots.push_back(std::move(snapshot));
			}
			CloseHandle(toolhelp);
	#endif
			return snapshots;
		}

		/* The name a thread was given with SetThreadDescription, or empty. */
		std::string GetThreadName(const DWORD threadId)
		{
			std::string name;
			if (const HANDLE thread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, threadId))
			{
				PWSTR description = nullptr;
				if (SUCCEEDED(GetThreadDescription(thread, &description)) && description)
				{
					name = ToUtf8(description);
					LocalFree(description);
				}
				CloseHandle(thread);
			}
			return name;
		}

		void LogOtherThreads(const std::vector<ThreadSnapshot> &snapshots)
		{
			Emit("{} other threads at the time of the fault:", snapshots.size());
			for (const ThreadSnapshot &snapshot : snapshots)
			{
				const std::string name = GetThreadName(snapshot.id);
				Emit("Thread {} ({})", snapshot.id, name.empty() ? "unnamed" : name);
				s_WalkedSnapshot = &snapshot;
				LogStack(snapshot.context, OTHER_THREAD_FRAMES, ReadSnapshotMemory);
				s_WalkedSnapshot = nullptr;
			}
		}

		const char *DescribeRegionState(const DWORD state)
		{
			switch (state)
			{
				case MEM_COMMIT:	return "committed";
				case MEM_RESERVE:	return "reserved, not committed";
				case MEM_FREE:		return "free (nothing allocated there)";
				default:			return "unknown";
			}
		}

		/* For an access violation: which address faulted, how it was accessed and what the address space holds there. */
		void LogFaultAddress(const EXCEPTION_RECORD &record)
		{
			if ((record.ExceptionCode != EXCEPTION_ACCESS_VIOLATION && record.ExceptionCode != EXCEPTION_IN_PAGE_ERROR) || record.NumberParameters < 2)
				return;

			const ULONG_PTR operation = record.ExceptionInformation[0];
			const ULONG_PTR address = record.ExceptionInformation[1];
			const char *access = operation == 0 ? "read" : operation == 1 ? "write" : operation == 8 ? "execute" : "access";
			Emit("  Faulting {} of 0x{:x}", access, address);

			MEMORY_BASIC_INFORMATION region{};
			if (VirtualQuery(reinterpret_cast<LPCVOID>(address), &region, sizeof(region)) == sizeof(region))
			{
				const auto base = reinterpret_cast<uintptr_t>(region.BaseAddress);
				Emit("  Address space there: {} region 0x{:x}..0x{:x} (allocation base 0x{:x}), protection 0x{:x}", DescribeRegionState(region.State),
					base, base + region.RegionSize, reinterpret_cast<uintptr_t>(region.AllocationBase), region.Protect);
			}
		}

		/* Commit headroom: CPU allocations fail once the system commit limit (RAM + page file) is reached. */
		void LogMemoryStatus()
		{
			constexpr DWORDLONG MB = 1024ull * 1024ull;

			PROCESS_MEMORY_COUNTERS_EX process{};
			process.cb = sizeof(process);
			if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&process), sizeof(process)))
			{
				Emit("  Process memory: {} MB committed (peak {} MB), working set {} MB", process.PrivateUsage / MB, process.PeakPagefileUsage / MB,
					process.WorkingSetSize / MB);
			}

			MEMORYSTATUSEX system{};
			system.dwLength = sizeof(system);
			if (GlobalMemoryStatusEx(&system))
			{
				Emit("  System memory: {} MB of {} MB commit free, {} MB of {} MB physical free", system.ullAvailPageFile / MB, system.ullTotalPageFile / MB,
					system.ullAvailPhys / MB, system.ullTotalPhys / MB);
			}
		}

		/* Writes every thread's stack and registers, plus the memory they point at, for an unhandled exception. */
		void WriteMiniDump(EXCEPTION_POINTERS *exception)
		{
			if (s_DumpPath[0] == L'\0')
				return;

			const HANDLE file = CreateFileW(s_DumpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (file == INVALID_HANDLE_VALUE)
				return;

			MINIDUMP_EXCEPTION_INFORMATION info{};
			info.ThreadId = GetCurrentThreadId();
			info.ExceptionPointers = exception;
			info.ClientPointers = FALSE;
			const auto type = static_cast<MINIDUMP_TYPE>(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);
			const bool written = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, type, &info, nullptr, nullptr) != FALSE;
			CloseHandle(file);

			if (written)
				Emit("Minidump written to {}", ToUtf8(s_DumpPath));
		}

		void Report(const char *reason, const unsigned long framesToSkip, EXCEPTION_POINTERS *exception = nullptr,
					const std::vector<ThreadSnapshot> *otherThreads = nullptr)
		{
			std::lock_guard lock(s_ReportMutex);
			Emit("{} (thread {})", reason, GetCurrentThreadId());
			if (exception && exception->ExceptionRecord)
				LogFaultAddress(*exception->ExceptionRecord);
			LogMemoryStatus();

			if (exception && exception->ContextRecord)
				LogStack(*exception->ContextRecord);
			else
				LogStack(framesToSkip + 1);

			if (otherThreads)
				LogOtherThreads(*otherThreads);
			if (exception)
				WriteMiniDump(exception);

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
			// First, while the other threads are still close to where they were when this one faulted.
			std::vector<ThreadSnapshot> otherThreads;
			try
			{
				otherThreads = CaptureOtherThreads();
			}
			catch (...)
			{
				// Out of memory: report without them
			}

			char reason[160];
			snprintf(reason, sizeof(reason), "Unhandled exception 0x%08lX at %p",
				info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionCode : 0ul,
				info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionAddress : nullptr);
			Report(reason, 0, info, &otherThreads);
			return EXCEPTION_CONTINUE_SEARCH;
		}
	}

	void InstallCrashHandlers()
	{
		if (s_Installed)
			return;

		s_Installed = true;
		const std::filesystem::path logFolder = IO::FileSystem::GetAppDataRoaming() / "logs";
		const std::wstring logPath = (logFolder / "SceneryEditorX.log").wstring();
		if (logPath.size() < std::size(s_FallbackLogPath))
			wcscpy_s(s_FallbackLogPath, logPath.c_str());

		const std::wstring dumpPath = (logFolder / "SceneryEditorX_crash.dmp").wstring();
		if (dumpPath.size() < std::size(s_DumpPath))
			wcscpy_s(s_DumpPath, dumpPath.c_str());

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

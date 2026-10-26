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
 * console_panel.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <SceneryEditorX/renderer/ui/editor_panel.h>
#include <spdlog/sinks/base_sink.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @struct LogEntry
	 * @brief One captured log line, as shown in the console and the status bar.
	 */
	struct LogEntry
	{
		spdlog::level::level_enum level = spdlog::level::info;
		std::string time;		// Local wall-clock time, HH:MM:SS
		std::string source;		// Logger name (Core, Editor, ...)
		std::string message;
	};

	/**
	 * @class EditorLogSink
	 * @brief spdlog sink that keeps the most recent log lines in memory for the editor UI.
	 * Attached to the engine loggers by the editor layer; thread-safe (any thread may log).
	 */
	class EditorLogSink final : public spdlog::sinks::base_sink<std::mutex>
	{
	public:
		explicit EditorLogSink(size_t capacity = 4000) : m_Capacity(capacity) {}

		/* @brief Calls fn(const std::deque<LogEntry>&) while holding the sink lock. */
		template<typename Fn>
		void Read(Fn &&fn)
		{
			std::lock_guard lock(mutex_);
			fn(m_Entries);
		}

		/* @brief Removes every captured entry. */
		void Clear();

		/* @brief The most recent warning or error, if any (copied under the lock). */
		bool GetLatestProblem(LogEntry &out);

		/* @brief Incremented on every new entry; lets the console auto-scroll only when something arrived. */
		[[nodiscard]] uint64_t GetRevision() const { return m_Revision.load(std::memory_order_relaxed); }

	protected:
		void sink_it_(const spdlog::details::log_msg &msg) override;
		void flush_() override {}

	private:
		std::deque<LogEntry> m_Entries;
		LogEntry m_LatestProblem;
		bool m_HasProblem = false;
		size_t m_Capacity;
		std::atomic<uint64_t> m_Revision = 0;
	};

	/**
	 * @class ConsolePanel
	 * @brief "CONSOLE" panel: filterable view of the engine and editor log.
	 */
	class ConsolePanel : public UI::EditorPanel
	{
	public:
		explicit ConsolePanel(std::shared_ptr<EditorLogSink> sink);

		void OnUIRender(bool &isOpen) override;

	private:
		std::shared_ptr<EditorLogSink> m_Sink;
		ImGuiTextFilter m_Filter;
		uint64_t m_LastRevision = 0;
		bool m_ShowInfo = true;
		bool m_ShowWarnings = true;
		bool m_ShowErrors = true;
		bool m_AutoScroll = true;
	};

}

// -------------------------------------------------------

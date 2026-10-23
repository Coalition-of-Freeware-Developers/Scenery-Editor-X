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
 * console_panel.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "console_panel.h"
#include <ctime>
#include <Editor/core/editor_theme.h>
#include <Editor/core/editor_ui.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		std::string FormatLocalTime(const spdlog::log_clock::time_point &time)
		{
			const std::time_t seconds = spdlog::log_clock::to_time_t(time);
			std::tm local{};
			localtime_s(&local, &seconds);

			char buffer[16];
			std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &local);
			return buffer;
		}

		bool IsProblem(const spdlog::level::level_enum level)
		{
			return level >= spdlog::level::warn && level != spdlog::level::off;
		}

		ImVec4 LevelColor(const spdlog::level::level_enum level)
		{
			using namespace EditorTheme;
			if (level >= spdlog::level::err)
				return ToStyle(Palette::Error);
			if (level == spdlog::level::warn)
				return ToStyle(Palette::Warning);
			return ToStyle(Palette::Text);
		}

		const char *LevelIcon(const spdlog::level::level_enum level)
		{
			if (level >= spdlog::level::err)
				return SEDX_ICON_TIMES_CIRCLE;
			if (level == spdlog::level::warn)
				return SEDX_ICON_EXCLAMATION_TRIANGLE;
			return SEDX_ICON_INFO_CIRCLE;
		}
	}

	// -------------------------------------------------------

	void EditorLogSink::Clear()
	{
		std::lock_guard lock(mutex_);
		m_Entries.clear();
		m_HasProblem = false;
		m_Revision.fetch_add(1, std::memory_order_relaxed);
	}

	bool EditorLogSink::GetLatestProblem(LogEntry &out)
	{
		std::lock_guard lock(mutex_);
		if (!m_HasProblem)
			return false;

		out = m_LatestProblem;
		return true;
	}

	void EditorLogSink::sink_it_(const spdlog::details::log_msg &msg)
	{
		LogEntry entry;
		entry.level		= msg.level;
		entry.time		= FormatLocalTime(msg.time);
		entry.source	= std::string(msg.logger_name.data(), msg.logger_name.size());
		entry.message	= std::string(msg.payload.data(), msg.payload.size());

		if (IsProblem(entry.level))
		{
			m_LatestProblem = entry;
			m_HasProblem = true;
		}

		m_Entries.push_back(std::move(entry));
		while (m_Entries.size() > m_Capacity)
			m_Entries.pop_front();

		m_Revision.fetch_add(1, std::memory_order_relaxed);
	}

	// -------------------------------------------------------

	ConsolePanel::ConsolePanel(std::shared_ptr<EditorLogSink> sink) : m_Sink(std::move(sink))
	{
		m_Title = "CONSOLE";
	}

	void ConsolePanel::OnUIRender(bool &isOpen)
	{
		if (EditorUI::BeginPanel("CONSOLE###Console", &isOpen) && m_Sink)
		{
			// Toolbar: clear, level toggles, text filter, auto-scroll
			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_TRASH, "Clear").c_str()))
				m_Sink->Clear();

			ImGui::SameLine();
			ImGui::Checkbox("Info", &m_ShowInfo);
			ImGui::SameLine();
			ImGui::Checkbox("Warnings", &m_ShowWarnings);
			ImGui::SameLine();
			ImGui::Checkbox("Errors", &m_ShowErrors);
			ImGui::SameLine();
			ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
			ImGui::SameLine();
			EditorUI::SearchFilter("##ConsoleFilter", m_Filter);

			const uint64_t revision = m_Sink->GetRevision();
			const bool scrollToBottom = m_AutoScroll && revision != m_LastRevision;
			m_LastRevision = revision;

			constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable;
			if (ImGui::BeginTable("##ConsoleLog", 4, tableFlags))
			{
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("##Level", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				m_Sink->Read([&](const std::deque<LogEntry> &entries)
				{
					for (const LogEntry &entry : entries)
					{
						const bool isError = entry.level >= spdlog::level::err;
						const bool isWarning = entry.level == spdlog::level::warn;
						if ((isError && !m_ShowErrors) || (isWarning && !m_ShowWarnings) || (!isError && !isWarning && !m_ShowInfo))
							continue;
						if (!m_Filter.PassFilter(entry.message.c_str()) && !m_Filter.PassFilter(entry.source.c_str()))
							continue;

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::TextDisabled("%s", entry.time.c_str());
						ImGui::TableNextColumn();
						ImGui::TextColored(LevelColor(entry.level), "%s", LevelIcon(entry.level));
						ImGui::TableNextColumn();
						ImGui::TextDisabled("%s", entry.source.c_str());
						ImGui::TableNextColumn();
						ImGui::PushStyleColor(ImGuiCol_Text, LevelColor(entry.level));
						ImGui::TextUnformatted(entry.message.c_str());
						ImGui::PopStyleColor();
					}
				});

				if (scrollToBottom)
					ImGui::SetScrollHereY(1.0f);

				ImGui::EndTable();
			}
		}
		EditorUI::EndPanel();
	}

}

// -------------------------------------------------------

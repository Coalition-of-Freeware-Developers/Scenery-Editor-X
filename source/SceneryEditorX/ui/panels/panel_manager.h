/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* panel_manager.h
* -------------------------------------------------------
* Created: 4/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <imgui.h>

namespace SceneryEditor
{
	class EditorConsolePanel;
	class SettingsPanel;
	class PanelManager
	{
	public:

		virtual void OnEvent(Event &e) override;
        virtual void OnImGuiRender(bool &isOpen) override;
        virtual void OnProjectChanged(const Ref<Project> &project) override;

		void Focus();

		void SetProgress(const std::string &label, float progress);
        void ClearProgress();

    private:
        void RenderMenu(const ImVec2 &size);
        void RenderConsole(const ImVec2 &size);
        const char *GetMessageType(const ConsoleMessage &message) const;
        const ImVec4 &GetMessageColor(const ConsoleMessage &message) const;
        ImVec4 GetToolbarButtonColor(const bool value) const;
        static void PushMessage(const ConsoleMessage &message);

		const char *m_PanelName = "Log";
        bool m_ClearOnPlay = true;

        std::mutex m_MessageBufferMutex;
        std::vector<ConsoleMessage> m_MessageBuffer;

        bool m_EnableScrollToLatest = true;
        bool m_ScrollToLatest = false;
        float m_PreviousScrollY = 0.0f;

        int16_t m_MessageFilters = (int16_t)ConsoleMessageFlags::All;

        bool m_DetailedPanelOpen = false;

        std::string m_ProgressLabel;
        float m_Progress = 0.0f;

	private:
        friend class EditorConsoleSink;
	    static EditorConsolePanel *s_EditorConsolePanel;
	    static SettingsPanel *s_SettingsPanel;
	};
} // namespace SceneryEditor

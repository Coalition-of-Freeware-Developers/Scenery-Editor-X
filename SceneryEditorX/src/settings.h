#pragma once

#include <filesystem>

namespace SceneryEditorX::UI {

	class ProgramSettingsPanel
	{
	public:
		ProgramSettingsPanel();

		void OnImGuiRender();

	private:
		std::filesystem::path cfgSettings;
	};

	class UserSettingsPanel
	{
	public:
		UserSettingsPanel();

		void OnImGuiRender();

	private:
		std::filesystem::path userSettings;
		std::filesystem::path XP12Dir;
	};
}

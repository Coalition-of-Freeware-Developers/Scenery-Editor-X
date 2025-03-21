/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* EditorConfig.hpp
* -------------------------------------------------------
* Created: 5/2/2025
* -------------------------------------------------------
*/

#if (defined(_WIN) || defined(WIN32) || defined(_WINDOWS))

// -------------------------------------------------------

#include <string>
#include <vector>
#include "stdint.h"
#include "vulkan\vulkan_core.h"

#define ENABLE_VULKAN_VALIDATION 1
#define ENABLE_VULKAN_RTX 1

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorConfig
	{
		std::string Title = "Scenery Editor X";

		//static inline uint32_t width = 1280;
		//static inline uint32_t height = 720;
		uint16_t width = 1280;
		uint16_t height = 720;

		uint32_t fpsLimit = 144;

		bool vsync = false;
		bool fullscreen = false;
		bool resizable = true;
		bool borderless = false;
		bool maximized = false;
		bool minimized = false;

		uint32_t vk_api_version = VK_API_VERSION_1_0;
		std::vector<const char *> args;

	};
} // namespace SceneryEditorX

#endif // _WIN || WIN32 || _WINDOWS

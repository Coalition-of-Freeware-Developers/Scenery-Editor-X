/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* graphics_defs.h
* -------------------------------------------------------
* Created: 18/3/2025
* -------------------------------------------------------
*/

#pragma once
#include <volk.h>

struct SwapChainInfo
{
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t bufferCount = 2;
	uint8_t format = VK_FORMAT_R8G8B8A8_UNORM;
	bool fullscreen = false; 
	bool vsync = true;
};
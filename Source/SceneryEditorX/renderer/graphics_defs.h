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

#include <vulkan/vulkan.h>
#include "vk_core.h"

struct SwapChainInfo
{
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t bufferCount = 2;
	uint8_t format = VK_FORMAT_R8G8B8A8_UNORM;
	bool fullscreen = false; 
	bool vsync = true;
};

VkSampler GraphicsEngine::CreateSampler(float maxLod)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // TODO: create separate one for shadow maps
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;

    samplerInfo.anisotropyEnable = false;

    // what color to return when clamp is active in addressing mode
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    // if comparison is enabled, texels will be compared to a value an the result
    // is used in filtering operations, can be used in PCF on shadow maps
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = maxLod;

    VkSampler sampler = VK_NULL_HANDLE;
    auto result = vkCreateSampler(g_Device, &samplerInfo, nullptr, &sampler);
    EDITOR_LOG_ERROR("Failed to create texture sampler!");
    //DEBUG_VK(vkRes, "Failed to create texture sampler!");

    return sampler;
}

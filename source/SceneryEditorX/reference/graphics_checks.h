/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* graphics_checks.h
* -------------------------------------------------------
* Created: 19/3/2025
* -------------------------------------------------------
*/

#pragma once

#include <vulkan/vulkan.h>
#include <vector>

// -------------------------------------------------------

class VulkanChecks
{
public:
    void InitChecks(const std::vector<const char *> &extensions, const std::vector<const char *> &layers);

    bool IsExtensionSupported(std::vector<VkExtensionProperties> const &availExtensions, char const *const extension);

    uint32_t extensionCount = 0;
    uint32_t layerCount = 0;


private:
    // Function to check if required extensions are available
    void CheckExtensions(const std::vector<const char *> &extensions);

    // Function to check if required layers are available
    void CheckLayers(const std::vector<const char *> &layers);
};

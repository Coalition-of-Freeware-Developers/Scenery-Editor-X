/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_extensions.h
* -------------------------------------------------------
* Created: 30/3/2025
* -------------------------------------------------------
*/
#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace SceneryEditorX
{
/**
     * @brief Structure containing Vulkan extensions used throughout the renderer
     * 
     * This class provides centralized access to Vulkan extension listings
     * used by the graphics engine, physical device management, and validation.
     */
struct VulkanExtensions
{
    // Instance extensions required by the application
    static const std::vector<const char *> &GetRequiredExtensions();

    // Device extensions required by the application
    static const std::vector<const char *> &GetDeviceExtensions();

    // Validation layers (for debug builds)
    static const std::vector<const char *> &GetValidationLayers();

private:
    // Extension lists are stored as static members to ensure they exist
    // for the entire program lifetime and are initialized only once
    static const std::vector<const char *> requiredExtensions;
    static const std::vector<const char *> deviceExtensions;
    static const std::vector<const char *> validationLayers;
};

} // namespace SceneryEditorX

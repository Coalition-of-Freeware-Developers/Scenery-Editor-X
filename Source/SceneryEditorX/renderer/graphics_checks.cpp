/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* graphics_checks.cpp
* -------------------------------------------------------
* Created: 19/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/renderer/graphics_checks.h>
#include <SceneryEditorX/renderer/vk_util.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

// -------------------------------------------------------

void VulkanChecks::InitChecks(const std::vector<const char *> &extensions, const std::vector<const char *> &layers)
{
    // Get GLFW required extensions
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    if (!glfwExtensions)
    {
        EDITOR_LOG_ERROR("Failed to get GLFW required Vulkan extensions");
        return;
    }

    // Combine GLFW extensions with user provided extensions
    std::vector<const char *> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    requiredExtensions.insert(requiredExtensions.end(), extensions.begin(), extensions.end());

}

/**
 * @brief Checks to see if the device has support for the required extensions.
 * @param availExtensions 
 * @param extension 
 * @return true if the device has support for the required extensions.
 * @return false if the device cannot support the required extensions.
 */
bool VulkanChecks::IsExtensionSupported(std::vector<VkExtensionProperties> const &availExtensions,
                                        char const *const extension)
{
    for (const auto &availExtension : availExtensions)
    {
        if (strcmp(extension, availExtension.extensionName) == 0)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Checks to see if the required extensions are available.
 * @param extensions
 */
void VulkanChecks::CheckExtensions(const std::vector<const char *> &extensions)
{
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

#ifdef SEDX_DEBUG
    EDITOR_LOG_INFO("============================================");
    EDITOR_LOG_INFO("Available extensions");
    EDITOR_LOG_INFO("____________________________________________");
    for (const auto &extension : availableExtensions)
    {
        EDITOR_LOG_INFO("{}", ToString(extension.extensionName));
    }
    EDITOR_LOG_INFO("============================================");
#endif

    for (const char *required : extensions)
    {
        if (!IsExtensionSupported(availableExtensions, required))
        {
            EDITOR_LOG_ERROR("Required extension not found: {}", ToString(required));
            return;
        }
        EDITOR_LOG_INFO("Found extension: {}", ToString(required));
    }
}

// Function to check if required layers are available
void VulkanChecks::CheckLayers(const std::vector<const char *> &layers)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

#ifdef SEDX_DEBUG
    EDITOR_LOG_INFO("Available layer/s");
    EDITOR_LOG_INFO("____________________________________________");
    for (const auto &layer : availableLayers)
    {
        EDITOR_LOG_INFO("{}", ToString(layer.layerName));
    }
    EDITOR_LOG_INFO("============================================");
#endif

    for (const char *required : layers)
    {
        bool found = false;
        for (const auto &layer : availableLayers)
        {
            if (strcmp(required, layer.layerName) == 0)
            {
                found = true;
                EDITOR_LOG_INFO("Found layer: {}", ToString(required));
                break;
            }
        }
        if (!found)
        {
            EDITOR_LOG_ERROR("Required layer not found: {}", ToString(required));
            return;
        }
    }
}

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_checks.cpp
* -------------------------------------------------------
* Created: 21/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/renderer/vk_checks.h>
#include <vector>

// -------------------------------------------------------

void VulkanChecks::InitChecks(const std::vector<const char *> &extensions, const std::vector<const char *> &layers)
{
    CheckExtensions(extensions);
    CheckLayers(layers);
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
    for (auto &availExtensions : availExtensions)
    {
        if (strstr(availExtensions.extensionName, extension))
        {
            return true;
        }
    }

    return false;
}

// Function to check if required extensions are available
void VulkanChecks::CheckExtensions(const std::vector<const char *> &extensions)
{
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
#ifdef SEDX_DEBUG
    EDITOR_LOG_INFO("============================================");
    EDITOR_LOG_INFO("Available extensions");
    EDITOR_LOG_INFO("____________________________________________");
#endif
    for (const auto &[extensionName, specVersion] : availableExtensions)
    {
#ifdef SEDX_DEBUG
        EDITOR_LOG_INFO("{}", ToString(extensionName));
#endif
    }
#ifdef SEDX_DEBUG
    EDITOR_LOG_INFO("============================================");
#endif
    for (const char *required : extensions)
    {
        bool found = false;
        for (const auto &[extensionName, specVersion] : availableExtensions)
        {
            if (strcmp(required, extensionName) == 0)
            {
                found = true;
                EDITOR_LOG_INFO("Found extension: {}", ToString(required));
                break;
            }
        }
        if (!found)
        {
            EDITOR_LOG_ERROR("Required extension not found: {}", ToString(required));
            break;
        }
    }

    return;
}

// Function to check if required layers are available
void VulkanChecks::CheckLayers(const std::vector<const char *> &layers)
{
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
#ifdef SEDX_DEBUG
    EDITOR_LOG_INFO("Available layer/s");
    EDITOR_LOG_INFO("____________________________________________");
#endif
    for (const auto &layer : availableLayers)
    {
#ifdef SEDX_DEBUG
        EDITOR_LOG_INFO("{}", ToString(layer.layerName));
#endif
    }
#ifdef SEDX_DEBUG
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
                EDITOR_LOG_INFO("Found layer/s: {}", ToString(required));
                break;
            }
        }
        if (!found)
        {
            EDITOR_LOG_ERROR("Required layer not found: {}", ToString(required));
            break;
        }
    }

    return;
}

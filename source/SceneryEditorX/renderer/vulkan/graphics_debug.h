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
 * graphics_debug.h
 * -------------------------------------------------------
 * Created: 16/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "enums.h"

// -----------------------------------------------------------------

namespace SceneryEditorX
{
    class CommandList;

    struct LayerSettingsData
	{
        VkLayerSettingsCreateInfoEXT createInfo; // create info struct for passing to vkCreateInstance or vkCreateDevice
	    std::vector<VkLayerSettingEXT> settings; // persistent storage for VkLayerSettingEXT, which must remain valid while the layer is active
	};

    // -----------------------------------------------------------------

    class Debugging
    {
    public:
        /**
         * @brief Sets whether Vulkan validation layer integration is enabled.
         * @param enabled True to enable validation-layer wiring, false to disable it.
         */
        static void SetValidationLayerEnabled(bool enabled);

        /**
         * @brief Returns the Vulkan validation layer name used by the renderer.
         * @return The validation layer name string.
         */
        static const char *GetValidationLayerName();

        /**
         * @brief Checks whether the requested validation layer is available on this system.
         * @return True if the validation layer can be enabled.
         */
        static bool IsValidationLayerSupported();

        /**
         * @brief Initializes Vulkan debug callbacks and object-label function pointers.
         * @param instance The active Vulkan instance.
         */
        static void Initialize(VkInstance instance);

        /**
         * @brief Shuts down Vulkan debug callbacks.
         * @param instance The active Vulkan instance.
         */
        static void Shutdown(VkInstance instance);

        /**
         * @brief Sets a human-readable name for a GPU resource to improve debugging and profiling readability. 
         * This function is a no-op if validation layers are not enabled, as the necessary function pointers are not initialized.
         * @param resource The resource to set the name for
         * @param type The type of the resource (e.g. buffer, image, etc.)
         * @param name The name to assign to the resource, which will appear in validation messages and GPU debuggers like RenderDoc. 
         * Should be a string literal or persistently allocated string.
         */
        static void SetResourceName(void *resource, ResourceType type, const char *name);

        /**
         * @brief Retrieves the current layer settings for the Vulkan validation layers.
         * @return A LayerSettingsData structure containing the settings for the validation layers.
         */
        static LayerSettingsData GetLayerSettings();

        /**
         * @brief Inserts a GPU marker into the command list for debugging and profiling purposes. 
         * This function is a no-op if validation layers are not enabled, as the necessary function pointers are not initialized.
         * @param cmdList The command list to insert the marker into
         * @param name The name of the marker, which will appear in validation messages and GPU debuggers like RenderDoc
         * @param color The color of the marker, which will appear in GPU debuggers like RenderDoc
         */
        static void BeginMarker(const CommandList *cmdList, const char *name, const xMath::Vec4 &color);

        /**
         * @brief Ends a previously started GPU marker in the command list.
         * This function is a no-op if validation layers are not enabled, as the necessary function pointers are not initialized.
         * @param cmdList The command list to end the marker in
         */
        static void EndMarker(const CommandList *cmdList);

        static bool IsValidationLayerEnabled()       { return m_ValidationLayer; }
        static bool IsGpuAssistedValidationEnabled() { return m_Gpu_Assisted_Validation_Enabled; }
        static bool IsGpuMarkingEnabled()            { return m_Gpu_Marking_Enabled; }
        static bool IsGpuTimingEnabled()             { return m_Gpu_Timing_Enabled; }
        static bool IsRenderdocEnabled()             { return m_Renderdoc_Enabled; }
        static bool IsShaderOptimizationEnabled()    { return m_Shader_Optimization_Enabled; }
        static bool IsLoggingToFileEnabled()         { return m_Logging_To_File_Enabled; }
        static bool IsBreadcrumbsEnabled()           { return m_Breadcrumbs_Enabled; }

    private:
        inline static bool m_ValidationLayer				 = false; // enables vulkan validation layers for api error detection and debug message reporting
        inline static bool m_Gpu_Assisted_Validation_Enabled = false; // enables gpu-assisted validation to detect memory and synchronization errors during rendering
        inline static bool m_Logging_To_File_Enabled         = false; // writes diagnostic and validation messages to a persistent log file
        inline static bool m_Breadcrumbs_Enabled             = false; // records gpu execution markers to help identify the cause of gpu crashes
        inline static bool m_Renderdoc_Enabled               = false; // enables integration with renderdoc for frame capture and gpu debugging
        inline static bool m_Gpu_Marking_Enabled             = true;  // labels gpu resources and command markers to improve debugging and profiling readability
        inline static bool m_Gpu_Timing_Enabled              = true;  // measures gpu execution times for profiling and performance analysis
        inline static bool m_Shader_Optimization_Enabled     = true;  // enables shader compiler optimizations to improve performance and efficiency
    };

}

// -----------------------------------------------------------------

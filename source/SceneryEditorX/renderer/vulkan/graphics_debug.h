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
	    
    // -----------------------------------------------------------------

	struct LayerSettingsData
	{
	    VkLayerSettingsCreateInfoEXT createInfo;
	    std::vector<VkLayerSettingEXT> settings;
	};

    // -----------------------------------------------------------------

    class Debugging
    {
    public:
        static void SetResourceName(void *resource, ResourceType type, const char *name);
        static LayerSettingsData GetLayerSettings();

        static void BeginMarker(const CommandList *cmdList, const char *name, const xMath::Vec4 &color);
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

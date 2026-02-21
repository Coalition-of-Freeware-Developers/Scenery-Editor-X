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
 * graphics_checks.h
 * -------------------------------------------------------
 * Created: 08/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "enums.h"


#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	class GraphicsChecks
	{
	public:
	    enum class GPUExtension
	    {
	        SWAPCHAIN = BIT(0),
	        ATOMIC_FLOAT = BIT(1),
	    };
	
	    enum class GPULayer
	    {
	        VALIDATION,
	    };
	
	    enum class GPUFeature : uint8_t
	    {
	        SAMPLER_ANISOTROPY,
	        SAMPLE_RATE_SHADING,
	        FILL_MODE_NON_SOLID,
	        GEOMETRY_SHADER,
	        TESSELLATION_SHADER,
	        WIDE_LINES,
	        PIPELINE_STATISTICS_QUERY
	    };
	
	    enum class GPUFeatureEXT : uint8_t
	    {
	        ATOMICS_FLOAT32,
	        ATOMIC_ADD_FLOAT32,
	        ATOMIC_MIN_MAX_FLOAT32_2,
	        BINDLESS_SUPPORT,
	    };
	
	    struct InstanceProperties
	    {
	        std::vector<GPUExtension> requestedExtensions;
	        std::vector<GPULayer> requestedLayers;
	        std::vector<GPUFeature> requestedFeatures;
	        std::vector<GPUFeatureEXT> requestedFeaturesEXT;
	    };
	
	    static InstanceProperties GetDefaultInstanceProperties();
	
	    static void EnableRequestedFeaturesEXT(const InstanceProperties &gpuProps,
	                                           VkPhysicalDeviceShaderAtomicFloatFeaturesEXT &atomicFloatFeatures,
	                                           VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT &atomicFloat2Features,
	                                           VkPhysicalDeviceDescriptorIndexingFeatures &indexingFeatures,
	                                           bool &enableFloat,
	                                           bool &enableFloat2,
	                                           bool &enableBindless);
	    static bool CheckExtension(const char *extension, const std::vector<VkExtensionProperties> &props);
	    static std::vector<const char *> GetExtensionList(const std::vector<GPUExtension> &requestedExtensions);
	    static uint32_t GetVulkanAPIVersion();
	    static const char *GetLayerName(GPULayer layer);
	    static const char *GetExtensionName(GPUExtension extension);
	    static void EnableGpuFeature(VkPhysicalDeviceFeatures &deviceFeatures,
	                                 const VkPhysicalDeviceFeatures &supportedFeatures,
	                                 GPUFeature feature);
	    static std::vector<const char *> GetLayerList(const std::vector<GPULayer> &requestedLayers);
	    static bool CheckAPIVersion(uint32_t minVulkanVersion);
	    static bool CheckValidationLayerSupport();

	    void CheckLayers(const std::vector<const char *> &layers);
	    void CheckExtensions(const std::vector<const char *> &extensions);
	
	    bool portabilityEnumEnabled = false;
	};
	
} // namespace SceneryEditorX

// -------------------------------------------------------

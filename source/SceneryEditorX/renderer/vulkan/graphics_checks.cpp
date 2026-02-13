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
 * graphics_checks.cpp
 * -------------------------------------------------------
 * Created: 08/02/2026
 * -------------------------------------------------------
 */
#include "graphics_checks.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	GraphicsChecks::InstanceProperties GraphicsChecks::GetDefaultInstanceProperties()
	{
	    InstanceProperties defaultProps;
	
	    defaultProps.requestedExtensions = {GPUExtension::SWAPCHAIN, GPUExtension::ATOMIC_FLOAT};
	    defaultProps.requestedLayers = {GPULayer::VALIDATION};
	    defaultProps.requestedFeatures = {GPUFeature::SAMPLER_ANISOTROPY,
	                                      GPUFeature::SAMPLE_RATE_SHADING,
	                                      GPUFeature::FILL_MODE_NON_SOLID,
	                                      GPUFeature::GEOMETRY_SHADER,
	                                      GPUFeature::TESSELLATION_SHADER,
	                                      GPUFeature::WIDE_LINES,
	                                      GPUFeature::PIPELINE_STATISTICS_QUERY};
	    defaultProps.requestedFeaturesEXT = {GPUFeatureEXT::BINDLESS_SUPPORT};
	
	    return defaultProps;
	}

	void GraphicsChecks::EnableRequestedFeaturesEXT(const InstanceProperties &gpuProps,
	                                                VkPhysicalDeviceShaderAtomicFloatFeaturesEXT &atomicFloatFeatures,
	                                                VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT &atomicFloat2Features,
	                                                VkPhysicalDeviceDescriptorIndexingFeatures &indexingFeatures,
	                                                bool &enableFloat,
	                                                bool &enableFloat2,
	                                                bool &enableBindless)
	{
	    for (GPUFeatureEXT feature : gpuProps.requestedFeaturesEXT)
	    {
	        switch (feature)
	        {
	        case GPUFeatureEXT::ATOMICS_FLOAT32:
	            atomicFloatFeatures.shaderBufferFloat32Atomics = VK_TRUE;
	            enableFloat = true;
	            break;
	
	        case GPUFeatureEXT::ATOMIC_ADD_FLOAT32:
	            atomicFloatFeatures.shaderBufferFloat32AtomicAdd = VK_TRUE;
	            enableFloat = true;
	            break;
	
	        case GPUFeatureEXT::ATOMIC_MIN_MAX_FLOAT32_2:
	            atomicFloat2Features.shaderBufferFloat32AtomicMinMax = VK_TRUE;
	            enableFloat2 = true;
	            break;
	
	        case GPUFeatureEXT::BINDLESS_SUPPORT:
	            indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
	            indexingFeatures.runtimeDescriptorArray = VK_TRUE;
	            enableBindless = true;
	            break;
	        


	        default:
	            SEDX_ASSERT(false, "Feature not implemented yet.");
	            break;
	        }
	    }
	}
	
	bool GraphicsChecks::CheckExtension(const char *extension, const std::vector<VkExtensionProperties> &props)
	{
	    for (const VkExtensionProperties &extensionProps : props)
	    {
	        if (strcmp(extension, extensionProps.extensionName) == 0)
	            return true;
	    }
	
	    return false;
	}
	
	uint32_t GraphicsChecks::GetVulkanAPIVersion()
	{
	    uint32_t apiVersion = 0;
	    vkEnumerateInstanceVersion(&apiVersion);
	
	    return apiVersion;
	}
	
	const char *GraphicsChecks::GetLayerName(GPULayer layer)
	{
	    switch (layer)
	    {
	    case GPULayer::VALIDATION:
	        return "VK_LAYER_KHRONOS_validation";
	    default:
	        SEDX_ASSERT(false, "Layer not implemented yet");
	        break;
	    }
	    return "";
	}
	
	const char *GraphicsChecks::GetExtensionName(GPUExtension extension)
	{
	    switch (extension)
	    {
	    case GPUExtension::SWAPCHAIN:
	        return VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	    case GPUExtension::ATOMIC_FLOAT:
	        return VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME;
	    default:
	        SEDX_ASSERT(false, "Extension not implemented yet");
	        break;
	    }
	    return "";
	}
	
	void GraphicsChecks::EnableGpuFeature(VkPhysicalDeviceFeatures &deviceFeatures,
	                                      const VkPhysicalDeviceFeatures &supportedFeatures,
	                                      GPUFeature feature)
	{
	    static const std::unordered_map<GPUFeature, VkBool32 VkPhysicalDeviceFeatures::*> featureMap = {
	        {GPUFeature::SAMPLER_ANISOTROPY, &VkPhysicalDeviceFeatures::samplerAnisotropy},
	        {GPUFeature::SAMPLE_RATE_SHADING, &VkPhysicalDeviceFeatures::sampleRateShading},
	        {GPUFeature::FILL_MODE_NON_SOLID, &VkPhysicalDeviceFeatures::fillModeNonSolid},
	        {GPUFeature::GEOMETRY_SHADER, &VkPhysicalDeviceFeatures::geometryShader},
	        {GPUFeature::TESSELLATION_SHADER, &VkPhysicalDeviceFeatures::tessellationShader},
	        {GPUFeature::WIDE_LINES, &VkPhysicalDeviceFeatures::wideLines},
	        {GPUFeature::PIPELINE_STATISTICS_QUERY, &VkPhysicalDeviceFeatures::pipelineStatisticsQuery},
	    };
	
	    auto it = featureMap.find(feature);
	    if (it != featureMap.end())
	    {
	        VkBool32 VkPhysicalDeviceFeatures::*featurePtr = it->second;
	        if (supportedFeatures.*featurePtr)
	            deviceFeatures.*featurePtr = VK_TRUE;
	        else
	            SEDX_CORE_WARN("Requested GPU feature not supported");
	    }
	}
	
	std::vector<const char *> GraphicsChecks::GetLayerList(const std::vector<GPULayer> &requestedLayers)
	{
	    std::vector<const char *> layers;
	    layers.reserve(requestedLayers.size());
	    for (GPULayer layer : requestedLayers)
	        layers.push_back(GetLayerName(layer));
	
	    return layers;
	}
	
	std::vector<const char *> GraphicsChecks::GetExtensionList(const std::vector<GPUExtension> &requestedExtensions)
	{
	    std::vector<const char *> extensions;
	    extensions.reserve(requestedExtensions.size());
	    for (GPUExtension ext : requestedExtensions)
	        extensions.push_back(GetExtensionName(ext));
	
	    return extensions;
	}
	
	bool GraphicsChecks::CheckAPIVersion(const uint32_t minVulkanVersion)
	{
	    uint32_t instanceVersion;
	    if (VkResult result = vkEnumerateInstanceVersion(&instanceVersion); result != VK_SUCCESS)
	    {
	        SEDX_CORE_ERROR("Failed to enumerate instance version: {}", ToString(result));
	        return false;
	    }

	    if (instanceVersion < minVulkanVersion)
	    {
	        SEDX_CORE_ERROR_TAG("Graphics Engine", "Installed Vulkan API version is incompatible with the program!");
	        SEDX_CORE_ERROR("You have {}.{}.{}.{}",
	                           VK_API_VERSION_VARIANT(instanceVersion),
	                           VK_API_VERSION_MAJOR(instanceVersion),
	                           VK_API_VERSION_MINOR(instanceVersion),
	                           VK_API_VERSION_PATCH(instanceVersion));
	
	        SEDX_CORE_ERROR("You need at least {}.{}.{}.{}",
	                           VK_API_VERSION_VARIANT(minVulkanVersion),
	                           VK_API_VERSION_MAJOR(minVulkanVersion),
	                           VK_API_VERSION_MINOR(minVulkanVersion),
	                           VK_API_VERSION_PATCH(minVulkanVersion));
	
	        return false;
	    }
	
	    return true;
	}
	
	bool GraphicsChecks::CheckValidationLayerSupport()
	{
	    uint32_t layerCount;
	    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	
	    if (layerCount == 0)
	    {
	        SEDX_CORE_ERROR_TAG("Vulkan", "No validation layers available on this system");
	        return false;
	    }
	
	    std::vector<VkLayerProperties> availableLayers(layerCount);
	    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	
	    for (const auto &layerProperties : availableLayers)
	    {
	        if (strcmp("VK_LAYER_KHRONOS_validation", layerProperties.layerName) == 0)
	        {
	            SEDX_CORE_INFO_TAG("Vulkan","Khronos validation layer is available (version: {})", layerProperties.implementationVersion);
	            return true;
	        }
	    }
	
	    SEDX_CORE_WARN_TAG("Vulkan", "Khronos validation layer is not available");
	    return false;
	}
	
	void GraphicsChecks::CheckLayers(const std::vector<const char *> &layers)
	{
	    uint32_t layerCount = 0;
	    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	
	    std::vector<VkLayerProperties> availableLayers(layerCount);
	    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	
	#ifdef RAILNEX_DEBUG
	    for (const auto &layer : availableLayers)
	        SEDX_CORE_INFO("Available layer: {}", layer.layerName);
	#endif
	
	    for (const char *required : layers)
	    {
	        bool found = false;
	        for (const auto &layer : availableLayers)
	        {
	            if (strcmp(required, layer.layerName) == 0)
	            {
	                found = true;
	                SEDX_CORE_INFO("Found layer: {}", required);
	                break;
	            }
	        }
	        if (!found)
	        {
	            SEDX_CORE_ERROR("Required layer not found: {}", required);
	        }
	    }
	}
	
	void GraphicsChecks::CheckExtensions(const std::vector<const char *> &extensions)
	{
	    uint32_t extensionCount = 0;
	    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	
	    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
	
	#ifdef RAILNEX_DEBUG
	    for (const auto &[extensionName, specVersion] : availableExtensions)
	        SEDX_CORE_INFO("Available extension: {}", extensionName);
	#endif
	
	    for (const char *required : extensions)
	    {
	        bool found = false;
	        for (const auto &[extensionName, specVersion] : availableExtensions)
	        {
	            if (strcmp(required, extensionName) == 0)
	            {
	                found = true;
	                SEDX_CORE_INFO("Found extension: {}", required);
	                break;
	            }
	        }
	        if (!found)
	        {
	            SEDX_CORE_ERROR("Required extension not found: {}", required);
	        }
	    }
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

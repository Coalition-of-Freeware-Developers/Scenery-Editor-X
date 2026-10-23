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
// ReSharper disable CppInconsistentNaming
#include "graphics_checks.h"
#include <SceneryEditorX/renderer/vulkan/device.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	GraphicsChecks::InstanceProperties GraphicsChecks::GetDefaultInstanceProperties()
	{
		InstanceProperties defaultProps;
		
		defaultProps.requestedExtensions = {
		   GPUExtension::SWAPCHAIN,
		   GPUExtension::ATOMIC_FLOAT,
		   GPUExtension::FRAGMENT_SHADING_RATE,
		   GPUExtension::EXTENDED_DYNAMIC_STATE,
		   GPUExtension::MEMORY_BUDGET,
		   GPUExtension::HDR_METADATA,
		   GPUExtension::ROBUSTNESS2,
		   GPUExtension::SYNCHRONIZATION2,
		   GPUExtension::GET_MEMORY_REQUIREMENTS2,
		};
		defaultProps.requestedLayers = {
			GPULayer::VALIDATION
		};
		defaultProps.requestedFeatures = {
			GPUFeature::SAMPLER_ANISOTROPY,
			GPUFeature::SAMPLE_RATE_SHADING,
			GPUFeature::FILL_MODE_NON_SOLID,
			GPUFeature::GEOMETRY_SHADER,
			GPUFeature::TESSELLATION_SHADER,
			GPUFeature::WIDE_LINES,
			GPUFeature::SHADER_DRAW_PARAMS,
			GPUFeature::PIPELINE_STATISTICS_QUERY
		};
		defaultProps.requestedFeaturesEXT = {
			GPUFeatureEXT::BINDLESS_SUPPORT,
		};
	
		return defaultProps;
	}

	void GraphicsChecks::EnableRequestedFeaturesEXT(const InstanceProperties &gpuProps, RequestedFeaturesEXTState &outState)
	{
		outState.atomicFloatFeatures = {};
		outState.atomicFloat2Features = {};
		outState.indexingFeatures = {};
		outState.drawParametersFeatures = {};
		outState.atomicFloatFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT;
		outState.atomicFloat2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT;
		outState.indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		outState.drawParametersFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;

		outState.enableFloat = false;
		outState.enableFloat2 = false;
		outState.enableBindless = false;
		outState.enableDrawParameters = false;

		for (GPUFeatureEXT feature : gpuProps.requestedFeaturesEXT)
		{
			switch (feature)
			{
			case GPUFeatureEXT::ATOMICS_FLOAT32:
				outState.atomicFloatFeatures.shaderBufferFloat32Atomics = VK_TRUE;
				outState.enableFloat = true;
				break;
	
			case GPUFeatureEXT::ATOMIC_ADD_FLOAT32:
				outState.atomicFloatFeatures.shaderBufferFloat32AtomicAdd = VK_TRUE;
				outState.enableFloat = true;
				break;
	
			case GPUFeatureEXT::ATOMIC_MIN_MAX_FLOAT32_2:
				outState.atomicFloat2Features.shaderBufferFloat32AtomicMinMax = VK_TRUE;
				outState.enableFloat2 = true;
				break;
	
			case GPUFeatureEXT::BINDLESS_SUPPORT:
				outState.indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
				outState.indexingFeatures.runtimeDescriptorArray = VK_TRUE;
				outState.enableBindless = true;
				break;
			case GPUFeatureEXT::SHADER_DRAW_PARAMETERS:
				outState.drawParametersFeatures.shaderDrawParameters = VK_TRUE;
				outState.enableDrawParameters = true;
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
			{
				return true;
			}
		}
	
		return false;
	}
	
	uint32_t GraphicsChecks::GetVulkanAPIVersion()
	{
		uint32_t apiVersion = 0;
		vkEnumerateInstanceVersion(&apiVersion);
	
		return apiVersion;
	}
	
	const char* GraphicsChecks::GetLayerName(GPULayer layer)
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
	
	const char* GraphicsChecks::GetExtensionName(GPUExtension extension)
	{
		switch (extension)
		{
		case GPUExtension::SWAPCHAIN:
			return VK_KHR_SWAPCHAIN_EXTENSION_NAME;
		case GPUExtension::ATOMIC_FLOAT:
			return VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME;
		case GPUExtension::FRAGMENT_SHADING_RATE:
			return VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME;
		case GPUExtension::EXTENDED_DYNAMIC_STATE:
			return VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME;
		case GPUExtension::MEMORY_BUDGET:
			return VK_EXT_MEMORY_BUDGET_EXTENSION_NAME;
		case GPUExtension::HDR_METADATA:
			return VK_EXT_HDR_METADATA_EXTENSION_NAME;
		case GPUExtension::ROBUSTNESS2:
			return VK_KHR_ROBUSTNESS_2_EXTENSION_NAME;
		case GPUExtension::SYNCHRONIZATION2:
			return VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME;
		case GPUExtension::GET_MEMORY_REQUIREMENTS2:
			return VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME;
		case GPUExtension::MUTABLE_DESCRIPTOR_TYPE:
			return VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME;
		default:
			SEDX_ASSERT(false, "Extension not implemented yet");
			break;
		}
		return "";
	}
	
	void GraphicsChecks::EnableGPUFeature(VkPhysicalDeviceFeatures &deviceFeatures, const VkPhysicalDeviceFeatures &supportedFeatures, GPUFeature feature)
	{
		static const std::unordered_map<GPUFeature, VkBool32 VkPhysicalDeviceFeatures::*> FEATURE_MAP = {
			{GPUFeature::SAMPLER_ANISOTROPY, &VkPhysicalDeviceFeatures::samplerAnisotropy},
			{GPUFeature::SAMPLE_RATE_SHADING, &VkPhysicalDeviceFeatures::sampleRateShading},
			{GPUFeature::FILL_MODE_NON_SOLID, &VkPhysicalDeviceFeatures::fillModeNonSolid},
			{GPUFeature::GEOMETRY_SHADER, &VkPhysicalDeviceFeatures::geometryShader},
			{GPUFeature::TESSELLATION_SHADER, &VkPhysicalDeviceFeatures::tessellationShader},
			{GPUFeature::WIDE_LINES, &VkPhysicalDeviceFeatures::wideLines},
			{GPUFeature::PIPELINE_STATISTICS_QUERY, &VkPhysicalDeviceFeatures::pipelineStatisticsQuery},
		};

		if (auto it = FEATURE_MAP.find(feature); it != FEATURE_MAP.end())
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
		{
			layers.push_back(GetLayerName(layer));
		}
	
		return layers;
	}
	
	std::vector<const char *> GraphicsChecks::GetExtensionList(const std::vector<GPUExtension> &requestedExtensions)
	{
		std::vector<const char *> extensions;
		extensions.reserve(requestedExtensions.size());
		for (GPUExtension ext : requestedExtensions)
		{
			extensions.push_back(GetExtensionName(ext));
		}
	
		return extensions;
	}
	
	bool GraphicsChecks::CheckAPIVersion(const uint32_t minVulkanVersion)
	{
		uint32_t instanceVersion;
		if (VkResult result = vkEnumerateInstanceVersion(&instanceVersion); result != VK_SUCCESS)
		{
			SEDX_CORE_ERROR("Failed to enumerate instance version: {}", std::to_string(result));
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
	
	#ifdef SEDX_DEBUG
		for (const auto &layer : availableLayers)
			SEDX_CORE_TRACE("Available layer: {}", layer.layerName);
	#endif
	
		for (const char *required : layers)
		{
			bool found = false;
			for (const auto &layer : availableLayers)
			{
				if (strcmp(required, layer.layerName) == 0)
				{
					found = true;
					SEDX_CORE_TRACE("Found layer: {}", required);
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
	
	#ifdef SEDX_DEBUG
		for (const auto &[extensionName, specVersion] : availableExtensions)
		{
			SEDX_CORE_TRACE("Available extension: {}", extensionName);
		}
	#endif
	
		for (const char *required : extensions)
		{
			bool found = false;
			for (const auto &[extensionName, specVersion] : availableExtensions)
			{
				if (strcmp(required, extensionName) == 0)
				{
					found = true;
					SEDX_CORE_TRACE("Found extension: {}", required);
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

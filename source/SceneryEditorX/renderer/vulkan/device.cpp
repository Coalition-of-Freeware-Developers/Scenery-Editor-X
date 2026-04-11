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
 * device.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
// ReSharper disable CppInconsistentNaming
#include "device.h"
#include "enums.h"
#include "memory_allocator.h"
#include "queue_manager.h"
#include "swapchain.h"
#include <algorithm>
#include <SceneryEditorX/core/window/window.h>
#include <volk/volk.h>

// -----------------------------------------------------------------

namespace SceneryEditorX
{
	
#pragma region Static Properties

	/**
	 * @struct DeviceFeatures
	 * @brief Static handle to the Vulkan physical device selected for use by this Device instance.
	 */
	struct DeviceFeatures
	{
		VkPhysicalDeviceFeatures2 s_Features = {};
		VkPhysicalDeviceRobustness2FeaturesEXT s_FeaturesRobustness = {};
		VkPhysicalDeviceExtendedDynamicStateFeaturesEXT s_FeaturesExtendedDynamicState = {};
		VkPhysicalDeviceShaderAtomicFloatFeaturesEXT s_FeaturesAtomicFloat = {};
		VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT s_FeaturesAtomicFloat2 = {};
		VkPhysicalDeviceVulkan14Features s_Features_1_4 = {};
		VkPhysicalDeviceVulkan13Features s_Features_1_3 = {};
		VkPhysicalDeviceVulkan12Features s_Features_1_2 = {};
		VkPhysicalDeviceFragmentShadingRateFeaturesKHR s_FeaturesVrs = {};
		VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT s_FeaturesMutableDescriptor = {}; // xess
		VkPhysicalDeviceRayQueryFeaturesKHR s_FeaturesRayQuery = {};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR s_FeaturesAccelStruct = {};
		VkPhysicalDeviceRayTracingPipelineFeaturesKHR s_FeaturesRayTracingPipeline = {};
		bool s_IsShadingRateSupported = false;
		bool s_XessSupported = false;
		bool s_IsRayTracingSupported = false;
		bool s_IsBindlessSupported = false;
		bool s_WideLines = false;

		/**
		 * @brief Rebuild the pNext chain after copying the structure to fix dangling pointers.
		 * 
		 * When DeviceFeatures is copied, the pNext pointers still reference the old structure's
		 * member addresses. This method reconstructs the chain to point to this instance's members.
		 * The chain is built conditionally based on which features are supported.
		 */
		void RebuildPNextChain()
		{
		   // Build chain from tail to head.
			void *nextInChain = nullptr;

			// VRS is conditionally included
			if (s_IsShadingRateSupported)
			{
				s_FeaturesVrs.pNext = nextInChain;
				nextInChain = &s_FeaturesVrs;
			}

			// Extended dynamic state is conditionally included
			if (s_FeaturesExtendedDynamicState.extendedDynamicState == VK_TRUE)
			{
				s_FeaturesExtendedDynamicState.pNext = nextInChain;
				nextInChain = &s_FeaturesExtendedDynamicState;
			}

			// Atomic float features are conditionally included
			if (s_FeaturesAtomicFloat2.shaderBufferFloat32AtomicMinMax == VK_TRUE)
			{
				s_FeaturesAtomicFloat2.pNext = nextInChain;
				nextInChain = &s_FeaturesAtomicFloat2;
			}

			if (s_FeaturesAtomicFloat.shaderBufferFloat32Atomics == VK_TRUE ||
				s_FeaturesAtomicFloat.shaderBufferFloat32AtomicAdd == VK_TRUE)
			{
				s_FeaturesAtomicFloat.pNext = nextInChain;
				nextInChain = &s_FeaturesAtomicFloat;
			}
	
			// Robustness links to VRS if supported, otherwise nullptr
			s_FeaturesRobustness.pNext = nextInChain;
			nextInChain = &s_FeaturesRobustness;
	
			// Vulkan 1.2 features
			s_Features_1_2.pNext = nextInChain;
			nextInChain = &s_Features_1_2;
	
			// Vulkan 1.3 features
			s_Features_1_3.pNext = nextInChain;
			nextInChain = &s_Features_1_3;
	
			// Vulkan 1.4 features
			s_Features_1_4.pNext = nextInChain;
			nextInChain = &s_Features_1_4;
	
			/*
			// Mutable descriptor is conditionally included for XeSS
			if (s_XessSupported)
			{
				s_FeaturesMutableDescriptor.pNext = nextInChain;
				nextInChain = &s_FeaturesMutableDescriptor;
			}
			*/
	
			// Ray tracing features are conditionally included
			if (s_IsRayTracingSupported)
			{
				s_FeaturesAccelStruct.pNext = nextInChain;
				s_FeaturesRayQuery.pNext = &s_FeaturesAccelStruct;
				s_FeaturesRayTracingPipeline.pNext = &s_FeaturesRayQuery;
				nextInChain = &s_FeaturesRayTracingPipeline;
			}
	
			// s_Features is the head of the chain
			s_Features.pNext = nextInChain;
		}
	};

	/**
	 * @brief Structure to hold information about a physical GPU device.
	 * @note This structure is designed to be a snapshot of the GPU's properties and supported features at the time of enumeration. 
	 */
	struct HWDeviceInfo
	{
		uintptr_t guid = 0;											// Unique identifier for the GPU
		DeviceType type = DeviceType::MaxEnum;						// GPU type
		char name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE] = {};			// GPU name
		static const size_t BUFFER_SIZE = 128;						// Fixed-Size buffers (stack, no heap allocations)
		char apiVersion[BUFFER_SIZE] = "N/A";						// Supported Vulkan api version
		char driverVersion[BUFFER_SIZE] = "N/A";					// GPU driver version provided by vendor
		char vendorName[BUFFER_SIZE] = "N/A";						// GPU vendor name (e.g., nvidia, amd)
		uint32_t vendorId = 0;										// vendor unique id
		uint32_t memory = 0;										// total device memory in mb
		DeviceFeatures s_SupportedFeatures = {};					// Supported Vulkan features
		VkSurfaceCapabilitiesKHR surfaceCaps;						// Surface capabilities for the device (if it supports presentation)
		std::vector<VkExtensionProperties> extensions;				// List of supported Vulkan extensions for this device
		void *data = nullptr;										// pointer to device-specific extra data
	};

	uint32_t Device::m_PhysicalDeviceIndex = 0;						// Index of the currently selected physical device (GPU)
	VkPhysicalDevice Device::m_PhysicalDevice = VK_NULL_HANDLE;		// Handle to the currently selected Vulkan physical device (GPU)
	static std::vector<HWDeviceInfo> s_PhysicalDevice;				// Cache all GPU device info
	static std::vector<VkPhysicalDevice> s_PhysicalDeviceHandles;	// Cache of Vulkan physical device handles corresponding to the GPU info list
	static bool s_SubparDevice = false;								// Flag to indicate if device was selected with suboptimal features but will still run
	bool Device::m_DeviceLost = false;

	PFN_vkCmdSetFragmentShadingRateKHR	s_FragmentShadingRate = nullptr;
	PFN_vkGetBufferDeviceAddress		s_BufferDeviceAddress = nullptr; 

	/**
	 * @brief Convert the internal DeviceType enum used by the engine to the corresponding Vulkan physical device type enum. 
	 * This is used when creating a Vulkan instance and enumerating physical devices.
	 * @param deviceType Internal DeviceType enum value
	 * @return Corresponding VkPhysicalDeviceType enum value, or VK_PHYSICAL_DEVICE_TYPE_OTHER if the input is invalid
	 */
	static VkPhysicalDeviceType GetDeviceType(DeviceType deviceType)
	{
		SEDX_CORE_ASSERT(false, "Invalid DeviceType enum value");

		switch (deviceType)
		{
			case DeviceType::Other:			return VK_PHYSICAL_DEVICE_TYPE_OTHER;
			case DeviceType::Integrated:	return VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
			case DeviceType::Discrete:		return VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
			case DeviceType::External:		return VK_PHYSICAL_DEVICE_TYPE_CPU;
			case DeviceType::Virtual:		return VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;
			case DeviceType::MaxEnum:		return VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM;
			default:
				SEDX_CORE_ERROR_TAG("Device", "Unknown DeviceType value: {}", static_cast<int>(deviceType));
				return VK_PHYSICAL_DEVICE_TYPE_OTHER;
		}
	}

	/**
	 * @brief Get the surface capabilities for a given physical device.
	 * @param physicalDevice Vulkan physical device handle
	 * @return VkSurfaceCapabilitiesKHR structure containing the surface capabilities
	 */
	/*
	static VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(const VkPhysicalDevice physicalDevice)
	{
		Swapchain *swapchain = RenderContext::Get()->swapchain;
		VkSurfaceCapabilitiesKHR surfaceCaps{};
		VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, swapchain->GetSurface(), &surfaceCaps);
		SEDX_VK_RESULT_ASSERT(result, "Failed to get physical device surface capabilities");
		return surfaceCaps;
	}
	*/

	/**
	 * @brief Check if a given physical device supports presentation to a specific surface.
	 * @param physicalDevice Vulkan physical device handle
	 * @param queueFamilyIndex Index of the queue family to check
	 * @return true if the surface is supported, false otherwise
	 */
	/*
	static bool GetSurfaceSupport(const VkPhysicalDevice physicalDevice, const uint32_t queueFamilyIndex)
	{
		Swapchain *swapchain = RenderContext::Get()->swapchain;
		VkBool32 supported = VK_FALSE;
		VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, swapchain->GetSurface(), &supported);
		SEDX_VK_RESULT_ASSERT(result, "Failed to query physical device surface support");
		return supported == VK_TRUE;
	}
	*/

	/**
	 * @brief Convert a Vulkan physical device type enum to the internal DeviceType enum used by the engine. This is used when populating GPU information during device enumeration.
	 * @param type Vulkan physical device type enum
	 * @return Internal DeviceType enum
	 */
	static DeviceType GetDeviceType(VkPhysicalDeviceType type)
	{
		SEDX_CORE_ASSERT(type != VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM, "Invalid VkPhysicalDeviceType enum value");

		switch (type)
		{
			case VK_PHYSICAL_DEVICE_TYPE_OTHER:			 return DeviceType::Other;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return DeviceType::Integrated;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:	 return DeviceType::Discrete;
			case VK_PHYSICAL_DEVICE_TYPE_CPU:			 return DeviceType::External;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:	 return DeviceType::Virtual;
			case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM:		 return DeviceType::MaxEnum;
			default:
				SEDX_CORE_ERROR_TAG("Device", "Unknown VkPhysicalDeviceType value: {}", static_cast<int>(type));
				return DeviceType::Other;
		}
	}

	/**
	 * @brief Detect supported Vulkan features for a given physical device and populate the DeviceFeatures structure. 
	 * This is used during device setup to determine which features can be enabled.
	 * @param physicalDevice Vulkan physical device handle
	 * @return DeviceFeatures structure populated with supported features
	 */
	static DeviceFeatures DetectGPUFeatures(VkPhysicalDevice physicalDevice)
	{
		DeviceFeatures feat{};

		// Features that will be enabled
		feat.s_FeaturesExtendedDynamicState.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
		feat.s_FeaturesExtendedDynamicState.pNext = nullptr;
		feat.s_FeaturesAtomicFloat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT;
		feat.s_FeaturesAtomicFloat.pNext = &feat.s_FeaturesExtendedDynamicState;
		feat.s_FeaturesAtomicFloat2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT;
		feat.s_FeaturesAtomicFloat2.pNext = &feat.s_FeaturesAtomicFloat;
		feat.s_FeaturesVrs.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
		feat.s_FeaturesVrs.pNext = &feat.s_FeaturesAtomicFloat2;
		feat.s_FeaturesRobustness.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
		feat.s_FeaturesRobustness.pNext = &feat.s_FeaturesVrs;
		feat.s_Features_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		feat.s_Features_1_2.pNext = &feat.s_FeaturesRobustness;
		feat.s_Features_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		feat.s_Features_1_3.pNext = &feat.s_Features_1_2;
		feat.s_Features_1_4.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
		feat.s_Features_1_4.pNext = &feat.s_Features_1_3;
		feat.s_FeaturesMutableDescriptor.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT;
		feat.s_FeaturesMutableDescriptor.pNext = &feat.s_Features_1_4;
		feat.s_FeaturesAccelStruct.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		feat.s_FeaturesAccelStruct.pNext = &feat.s_FeaturesMutableDescriptor;
		feat.s_FeaturesRayQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
		feat.s_FeaturesRayQuery.pNext = &feat.s_FeaturesAccelStruct;
		feat.s_FeaturesRayTracingPipeline.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		feat.s_FeaturesRayTracingPipeline.pNext = &feat.s_FeaturesRayQuery;
		feat.s_Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		feat.s_Features.pNext = &feat.s_FeaturesRayTracingPipeline;

		// Detect which features are supported
		VkPhysicalDeviceExtendedDynamicStateFeaturesEXT supportExtendedDynamicState = {};
		supportExtendedDynamicState.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
		VkPhysicalDeviceShaderAtomicFloatFeaturesEXT supportAtomicFloat = {};
		supportAtomicFloat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT;
		supportAtomicFloat.pNext = &supportExtendedDynamicState;
		VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT supportAtomicFloat2 = {};
		supportAtomicFloat2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT;
		supportAtomicFloat2.pNext = &supportAtomicFloat;
		VkPhysicalDeviceFragmentShadingRateFeaturesKHR supportVrs = {};
		supportVrs.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
		supportVrs.pNext = &supportAtomicFloat2;
		VkPhysicalDeviceRobustness2FeaturesEXT supportRobustness = {};
		supportRobustness.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
		supportRobustness.pNext = &supportVrs;
		VkPhysicalDeviceVulkan12Features support12 = {};
		support12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		support12.pNext = &supportRobustness;
		VkPhysicalDeviceVulkan13Features support13 = {};
		support13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		support13.pNext = &support12;
		VkPhysicalDeviceVulkan14Features support14 = {};
		support14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
		support14.pNext = &support13;
		VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT supportMutableDescriptor = {};
		supportMutableDescriptor.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT;
		supportMutableDescriptor.pNext = &support14;
		VkPhysicalDeviceAccelerationStructureFeaturesKHR supportAccelStruct = {};
		supportAccelStruct.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		supportAccelStruct.pNext = &supportMutableDescriptor;
		VkPhysicalDeviceRayQueryFeaturesKHR supportRayQuery = {};
		supportRayQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
		supportRayQuery.pNext = &supportAccelStruct;
		VkPhysicalDeviceRayTracingPipelineFeaturesKHR supportRayTracingPipeline = {};
		supportRayTracingPipeline.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		supportRayTracingPipeline.pNext = &supportRayQuery;
		VkPhysicalDeviceFeatures2 support = {};
		support.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		support.pNext = &supportRayTracingPipeline;

		vkGetPhysicalDeviceFeatures2(physicalDevice, &support);

		// -------------------------------------------------------

		// Check if certain features are supported and enable them
		{
			// Variable Shading Rate
			feat.s_IsShadingRateSupported = supportVrs.attachmentFragmentShadingRate == VK_TRUE;
			if (feat.s_IsShadingRateSupported)
			{
				// enable this feature conditionally (no assert) as older GPUs like NV 1080 and Radeon RX Vega do not support it
				// support details:
				// https://vulkan.gpuinfo.org/listdevicescoverage.php?platform=windows&extension=VK_KHR_fragment_shading_rate
				feat.s_FeaturesVrs.attachmentFragmentShadingRate = VK_TRUE;
				if (supportVrs.pipelineFragmentShadingRate == VK_TRUE)
				{
					feat.s_FeaturesVrs.pipelineFragmentShadingRate = VK_TRUE;
				}

				// Extended dynamic state
				if (supportExtendedDynamicState.extendedDynamicState == VK_TRUE)
				{
					feat.s_FeaturesExtendedDynamicState.extendedDynamicState = VK_TRUE;
				}
			}
			else
			{
				supportRobustness.pNext = feat.s_FeaturesVrs.pNext; // remove from chain
			}

			// Misc
			{
				// Tessellation
				SEDX_CORE_ASSERT(support.features.tessellationShader == VK_TRUE);
				feat.s_Features.features.tessellationShader = VK_TRUE;

				// Depth Clamp
				SEDX_CORE_ASSERT(support.features.depthClamp == VK_TRUE);
				feat.s_Features.features.depthClamp = VK_TRUE;

				// 64-bit Integers in Shaders (needed for buffer device address)
				SEDX_CORE_ASSERT(support.features.shaderInt64 == VK_TRUE);
				feat.s_Features.features.shaderInt64 = VK_TRUE;

				// Anisotropic Filtering
				SEDX_CORE_ASSERT(support.features.samplerAnisotropy == VK_TRUE);
				feat.s_Features.features.samplerAnisotropy = VK_TRUE;

				// Line and point rendering
				SEDX_CORE_ASSERT(support.features.fillModeNonSolid == VK_TRUE);
				feat.s_Features.features.fillModeNonSolid = VK_TRUE;

				// Lines with adjustable thickness
				SEDX_CORE_ASSERT(support.features.wideLines == VK_TRUE);
				feat.s_Features.features.wideLines = VK_TRUE;

				// Cubemaps
				SEDX_CORE_ASSERT(support.features.imageCubeArray == VK_TRUE);
				feat.s_Features.features.imageCubeArray = VK_TRUE;

				// Pipeline Statistics
				SEDX_CORE_ASSERT(support.features.pipelineStatisticsQuery == VK_TRUE);
				feat.s_Features.features.pipelineStatisticsQuery = VK_TRUE;
			}

			// Quality of Life Improvements
			{
				// Dynamic render passes and no frame buffer objects
				SEDX_CORE_ASSERT(support13.dynamicRendering == VK_TRUE);
				feat.s_Features_1_3.dynamicRendering = VK_TRUE;

				// Better Synchronization
				SEDX_CORE_ASSERT(support13.synchronization2 == VK_TRUE);
				feat.s_Features_1_3.synchronization2 = VK_TRUE;

				// Timeline Semaphores
				SEDX_CORE_ASSERT(support12.timelineSemaphore == VK_TRUE);
				feat.s_Features_1_2.timelineSemaphore = VK_TRUE;

				// Timeline Semaphore Counter
				SEDX_CORE_ASSERT(support.features.shaderFloat64 == VK_TRUE);
				feat.s_Features.features.shaderFloat64 = VK_TRUE;
			}

			// Descriptors
			{
				SEDX_CORE_ASSERT(support12.descriptorBindingVariableDescriptorCount == VK_TRUE);
				feat.s_Features_1_2.descriptorBindingVariableDescriptorCount = VK_TRUE;

				SEDX_CORE_ASSERT(support12.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE);
				feat.s_Features_1_2.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;

				SEDX_CORE_ASSERT(support12.descriptorBindingPartiallyBound == VK_TRUE);
				feat.s_Features_1_2.descriptorBindingPartiallyBound = VK_TRUE;

				SEDX_CORE_ASSERT(support12.runtimeDescriptorArray == VK_TRUE);
				feat.s_Features_1_2.runtimeDescriptorArray = VK_TRUE;

				SEDX_CORE_ASSERT(support12.descriptorIndexing == VK_TRUE);
				feat.s_Features_1_2.descriptorIndexing = VK_TRUE;

				SEDX_CORE_ASSERT(supportRobustness.nullDescriptor == VK_TRUE);
				feat.s_FeaturesRobustness.nullDescriptor = VK_TRUE;

				SEDX_CORE_ASSERT(support14.pushDescriptor == VK_TRUE);
				feat.s_Features_1_4.pushDescriptor = VK_TRUE;
			}

			// AMD Fidelity FX
			{
				// spd
				SEDX_CORE_ASSERT(support12.shaderSubgroupExtendedTypes == VK_TRUE);
				feat.s_Features_1_2.shaderSubgroupExtendedTypes = VK_TRUE;

				// Wave64
				SEDX_CORE_ASSERT(support13.shaderDemoteToHelperInvocation == VK_TRUE);
				feat.s_Features_1_3.shaderDemoteToHelperInvocation = VK_TRUE;

				// float16 - If supported, fsr will opt for it, so don't assert
				if (support12.shaderFloat16 == VK_TRUE)
				{
					feat.s_Features_1_2.shaderFloat16 = VK_TRUE;
				}
				// int16 - If supported, fsr will opt for it, so don't assert
				if (support.features.shaderInt16 == VK_TRUE)
				{
					feat.s_Features.features.shaderInt16 = VK_TRUE;
				}
				// wave64 - If supported, fsr will opt for it, so don't assert
				if (support13.subgroupSizeControl == VK_TRUE)
				{
					feat.s_Features_1_3.subgroupSizeControl = VK_TRUE;
				}
			}

			// xess
			{
				SEDX_CORE_ASSERT(support12.shaderInt8 == VK_TRUE);
				feat.s_Features_1_2.shaderInt8 = VK_TRUE;

				SEDX_CORE_ASSERT(support13.shaderIntegerDotProduct == VK_TRUE);
				feat.s_Features_1_3.shaderIntegerDotProduct = VK_TRUE;

				SEDX_CORE_ASSERT(support12.scalarBlockLayout == VK_TRUE);
				feat.s_Features_1_2.scalarBlockLayout = VK_TRUE;

				if (supportMutableDescriptor.mutableDescriptorType == VK_TRUE)
				{
					feat.s_FeaturesMutableDescriptor.mutableDescriptorType = VK_TRUE;
					feat.s_XessSupported = true;
				}
				else
				{
					feat.s_Features.pNext = feat.s_Features.pNext; // Remove from chain
				}
			}

			// Ray Tracing
			{
				feat.s_IsRayTracingSupported = supportAccelStruct.accelerationStructure == VK_TRUE &&
					supportRayTracingPipeline.rayTracingPipeline == VK_TRUE && supportRayQuery.rayQuery == VK_TRUE;

				if (feat.s_IsRayTracingSupported)
				{
					SEDX_CORE_ASSERT(supportAccelStruct.accelerationStructure == VK_TRUE);
					feat.s_FeaturesAccelStruct.accelerationStructure = VK_TRUE;

					SEDX_CORE_ASSERT(supportRayTracingPipeline.rayTracingPipeline == VK_TRUE);
					feat.s_FeaturesRayTracingPipeline.rayTracingPipeline = VK_TRUE;

					SEDX_CORE_ASSERT(supportRayQuery.rayQuery == VK_TRUE);
					feat.s_FeaturesRayQuery.rayQuery = VK_TRUE;

					SEDX_CORE_ASSERT(support12.bufferDeviceAddress == VK_TRUE);
					feat.s_Features_1_2.bufferDeviceAddress = VK_TRUE;
				}
				else
				{
					// Remove from chain
					feat.s_Features.pNext = feat.s_FeaturesRayTracingPipeline.pNext;
				}
			}

			{
				// Geometry
				SEDX_CORE_ASSERT(support.features.geometryShader == VK_TRUE);
				feat.s_Features.features.geometryShader = VK_TRUE;
			}
		}

		return feat;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Device Manufacturer																							  ///
	///	Helper functions to identify GPU vendor based on device properties.											  ///
	///	Returns True if the GPU matches the vendor, false otherwise.												  ///
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	static bool IsNvidia(const HWDeviceInfo &hwInfo)
	{
		return hwInfo.vendorId == 0x10DE || strstr(hwInfo.name, "Nvidia") != nullptr || strstr(hwInfo.name, "nvidia") != nullptr;
	}

	static bool IsAmd(const HWDeviceInfo &hwInfo)
	{
		return hwInfo.vendorId == 0x1002 || hwInfo.vendorId == 0x1022 || strstr(hwInfo.name, "AMD") != nullptr ||  strstr(hwInfo.name, "amd") != nullptr;
	}

	static bool IsIntel(const HWDeviceInfo &hwInfo)
	{
		return hwInfo.vendorId == 0x8086 || hwInfo.vendorId == 0x163C || hwInfo.vendorId == 0x8087 || strstr(hwInfo.name, "Intel") != nullptr || strstr(hwInfo.name, "intel") != nullptr;
	}

	static bool IsArm(const HWDeviceInfo &hwInfo)
	{
		return hwInfo.vendorId == 0x13B5 || strstr(hwInfo.name, "Arm") != nullptr || strstr(hwInfo.name, "arm") != nullptr;
	}

	static bool IsQualcomm(const HWDeviceInfo &hwInfo)
	{
		return hwInfo.vendorId == 0x5143 || strstr(hwInfo.name, "Qualcomm") != nullptr || strstr(hwInfo.name, "qualcomm") != nullptr;
	}

	static bool IsBelowMinimumRequirements(const HWDeviceInfo &dvInfo)
	{
		constexpr uint32_t minMemoryMb = 4096;
		constexpr DeviceType minType = DeviceType::Discrete;

		const bool isOld =
			strstr(dvInfo.name, "GeForce GTX 10") != nullptr || strstr(dvInfo.name, "GeForce GTX 9") != nullptr ||
			strstr(dvInfo.name, "GeForce GTX 7") != nullptr || strstr(dvInfo.name, "GeForce GTX 6") != nullptr ||
			strstr(dvInfo.name, "Radeon R9") != nullptr || strstr(dvInfo.name, "Radeon RX 4") != nullptr ||
			strstr(dvInfo.name, "Radeon RX 5") != nullptr || strstr(dvInfo.name, "Radeon HD") != nullptr;

		return dvInfo.memory < minMemoryMb || dvInfo.type != minType || isOld;
	}

	static void PopulateDeviceInfo(VkPhysicalDevice physicalDevice, HWDeviceInfo &deviceInfo)
	{
		VkPhysicalDeviceProperties2 deviceProps;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps.properties);

		deviceInfo.guid = (static_cast<uintptr_t>(deviceProps.properties.vendorID) << 32) | static_cast<uintptr_t>(deviceProps.properties.deviceID);
		deviceInfo.type = GetDeviceType(deviceProps.properties.deviceType);
		deviceInfo.vendorId = deviceProps.properties.vendorID;

		strncpy(deviceInfo.name, deviceProps.properties.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE - 1);
		deviceInfo.name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE - 1] = '\0';

		snprintf(deviceInfo.apiVersion, HWDeviceInfo::BUFFER_SIZE, "%u.%u.%u",
				 VK_VERSION_MAJOR(deviceProps.properties.apiVersion),
				 VK_VERSION_MINOR(deviceProps.properties.apiVersion),
				 VK_VERSION_PATCH(deviceProps.properties.apiVersion));
		snprintf(deviceInfo.driverVersion, HWDeviceInfo::BUFFER_SIZE, "%u.%u.%u",
				 VK_VERSION_MAJOR(deviceProps.properties.driverVersion),
				 VK_VERSION_MINOR(deviceProps.properties.driverVersion),
				 VK_VERSION_PATCH(deviceProps.properties.driverVersion));

		// Vendor name is not directly available in Vulkan; we can map known vendor IDs to names.
		switch (deviceProps.properties.vendorID)
		{
			case 0x10DE: IsNvidia(deviceInfo); strncpy(deviceInfo.vendorName, "NVIDIA", HWDeviceInfo::BUFFER_SIZE - 1); break;
			case 0x1002:
			case 0x1022: IsAmd(deviceInfo); strncpy(deviceInfo.vendorName, "AMD", HWDeviceInfo::BUFFER_SIZE - 1); break;
			case 0x8086:
			case 0x8087:
			case 0x163C: IsIntel(deviceInfo); strncpy(deviceInfo.vendorName, "INTEL", HWDeviceInfo::BUFFER_SIZE - 1); break;
			case 0x13B5: IsArm(deviceInfo); strncpy(deviceInfo.vendorName, "ARM", HWDeviceInfo::BUFFER_SIZE - 1); break;
			case 0x5143: IsQualcomm(deviceInfo); strncpy(deviceInfo.vendorName, "QUALCOMM", HWDeviceInfo::BUFFER_SIZE - 1); break;

			default: strncpy(deviceInfo.vendorName, "UNKNOWN", HWDeviceInfo::BUFFER_SIZE - 1); break;
		}
		deviceInfo.vendorName[HWDeviceInfo::BUFFER_SIZE - 1] = '\0';

		// -----------------------------------------------------------------

		VkPhysicalDeviceMemoryProperties2 memProps;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps.memoryProperties);
		uint64_t totalMemory = 0;
		for (uint32_t i = 0; i < memProps.memoryProperties.memoryHeapCount; ++i)
		{
			if (memProps.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			{
				totalMemory += memProps.memoryProperties.memoryHeaps[i].size;
			}
		}
		deviceInfo.memory = static_cast<uint32_t>(totalMemory / (1024 * 1024)); // Convert to MB

		// -----------------------------------------------------------------

		//deviceInfo.surfaceCaps = GetSurfaceCapabilities(physicalDevice);
		deviceInfo.s_SupportedFeatures = DetectGPUFeatures(physicalDevice);


		// CRITICAL: Rebuild pNext chain after copying to fix dangling pointers
		// The pNext chain in the copied structure still points to the original stack addresses
		// from DetectGPUFeatures, which are now invalid. This rebuilds the chain to point to
		// the copied structure's members.
		deviceInfo.s_SupportedFeatures.RebuildPNextChain();

		// -----------------------------------------------------------------

		// XeSS support check (must occur after DetectGPUFeatures)
		{
			SEDX_CORE_ASSERT(deviceInfo.s_SupportedFeatures.s_Features_1_2.shaderInt8 == VK_TRUE);
			deviceInfo.s_SupportedFeatures.s_Features_1_2.shaderInt8 = VK_TRUE;

			SEDX_CORE_ASSERT(deviceInfo.s_SupportedFeatures.s_Features_1_3.shaderIntegerDotProduct == VK_TRUE);
			deviceInfo.s_SupportedFeatures.s_Features_1_3.shaderIntegerDotProduct = VK_TRUE;

			SEDX_CORE_ASSERT(deviceInfo.s_SupportedFeatures.s_Features_1_2.scalarBlockLayout == VK_TRUE);
			deviceInfo.s_SupportedFeatures.s_Features_1_2.scalarBlockLayout = VK_TRUE;

			if (deviceInfo.s_SupportedFeatures.s_FeaturesMutableDescriptor.mutableDescriptorType == VK_TRUE)
			{
				deviceInfo.s_SupportedFeatures.s_FeaturesMutableDescriptor.mutableDescriptorType = VK_TRUE;
				deviceInfo.s_SupportedFeatures.s_XessSupported = true;
			}
			else
			{
				// XeSS not supported - mutable descriptor will be excluded from chain in RebuildPNextChain
				deviceInfo.s_SupportedFeatures.s_XessSupported = false;
			}
		}
	}

	/**
	 * @brief Populate the list of physical devices and their properties from the Vulkan instance
	 * @param instance Vulkan instance handle
	 */
	static void PopulatePhysicalDevices(const VkInstance instance)
	{
		uint32_t deviceCount = 0;
		if (VkResult r = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr); r != VK_SUCCESS || deviceCount == 0)
		{
			SEDX_CORE_ERROR_TAG("Device", "No Vulkan physical devices found (VkResult: {})", static_cast<int>(r));
			s_PhysicalDevice.clear();
			s_PhysicalDeviceHandles.clear();
			return;
		}

		// Allocate storage for device handles
		std::vector<VkPhysicalDevice> devices(deviceCount);

		// Enumerate all physical devices in a single call
		if (VkResult r = vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()); r != VK_SUCCESS)
		{
			SEDX_CORE_ERROR_TAG("Device", "Failed to enumerate physical devices (VkResult: {})", static_cast<int>(r));
			s_PhysicalDevice.clear();
			s_PhysicalDeviceHandles.clear();
			return;
		}

		// Prepare storage - reserve memory to avoid reallocations
		s_PhysicalDevice.clear();
		s_PhysicalDevice.reserve(deviceCount);
		s_PhysicalDeviceHandles.clear();
		s_PhysicalDeviceHandles.reserve(deviceCount);

		// Populate device information for all devices
		for (uint32_t i = 0; i < deviceCount; ++i)
		{
			HWDeviceInfo deviceInfo = {};
			PopulateDeviceInfo(devices[i], deviceInfo);

			// Cache both the device info and the handle
			s_PhysicalDevice.push_back(deviceInfo);
			s_PhysicalDeviceHandles.push_back(devices[i]); // Store VkPhysicalDevice handle

			SEDX_CORE_TRACE_TAG("Device", "[{}] {} - {} ({} MB VRAM)", i, deviceInfo.name, deviceInfo.vendorName, deviceInfo.memory);
		}
	}

	/**
	 * @brief Calculate a feature score for a physical device
	 * @param deviceInfo Hardware device information with detected features
	 * @param requiredProps Required instance properties (features, extensions, etc.) to evaluate against each device
	 * @return Score value (higher is better), or -1 if required features are missing
	 */
	static int32_t ScoreDeviceFeatures(const HWDeviceInfo &deviceInfo, const GraphicsChecks::InstanceProperties &requiredProps)
	{

		const DeviceFeatures &features = deviceInfo.s_SupportedFeatures;
		int32_t score = 0;

		// Check critical required features first (must have, or return -1)
		bool hasCriticalFeatures = true;

		// Validate Vulkan 1.2 required features
		if (features.s_Features_1_2.descriptorIndexing != VK_TRUE ||
			features.s_Features_1_2.timelineSemaphore != VK_TRUE || features.s_Features_1_2.shaderInt8 != VK_TRUE ||
			features.s_Features_1_2.scalarBlockLayout != VK_TRUE)
		{
			SEDX_CORE_WARN_TAG("Device", "Device '{}' missing critical Vulkan 1.2 features", deviceInfo.name);
			hasCriticalFeatures = false;
		}

		// Validate Vulkan 1.3 required features
		if (features.s_Features_1_3.dynamicRendering != VK_TRUE ||
			features.s_Features_1_3.synchronization2 != VK_TRUE ||
			features.s_Features_1_3.shaderIntegerDotProduct != VK_TRUE)
		{
			SEDX_CORE_WARN_TAG("Device", "Device '{}' missing critical Vulkan 1.3 features", deviceInfo.name);
			hasCriticalFeatures = false;
		}

		// Validate core required features
		if (features.s_Features.features.samplerAnisotropy != VK_TRUE ||
			features.s_Features.features.fillModeNonSolid != VK_TRUE ||
			features.s_Features.features.geometryShader != VK_TRUE ||
			features.s_Features.features.tessellationShader != VK_TRUE)
		{
			SEDX_CORE_WARN_TAG("Device", "Device '{}' missing critical core features", deviceInfo.name);
			hasCriticalFeatures = false;
		}

		if (!hasCriticalFeatures)
		{
			return -1; // Device does not meet minimum requirements
		}

		// Base score from memory (1 point per GB)
		score += static_cast<int32_t>(deviceInfo.memory / 1024);

		// Score optional advanced features (10 points each)
		if (features.s_IsRayTracingSupported)
		{
			score += 10;
			SEDX_CORE_TRACE_TAG("Device", "Device '{}': Ray Tracing supported (+10)", deviceInfo.name);
		}

		if (features.s_IsShadingRateSupported)
		{
			score += 10;
			SEDX_CORE_TRACE_TAG("Device", "Device '{}': Variable Shading Rate supported (+10)", deviceInfo.name);
		}

		if (features.s_XessSupported)
		{
			score += 10;
			SEDX_CORE_TRACE_TAG("Device", "Device '{}': XeSS supported (+10)", deviceInfo.name);
		}

		if (features.s_IsBindlessSupported)
		{
			score += 5;
			SEDX_CORE_TRACE_TAG("Device", "Device '{}': Bindless descriptors supported (+5)", deviceInfo.name);
		}

		if (features.s_WideLines)
		{
			score += 2;
			SEDX_CORE_TRACE_TAG("Device", "Device '{}': Wide lines supported (+2)", deviceInfo.name);
		}

		// Score optional shader features (5 points each)
		if (features.s_Features_1_2.shaderFloat16 == VK_TRUE)
		{
			score += 5;
			SEDX_CORE_TRACE_TAG("Device", "Device '{}': Float16 shaders supported (+5)", deviceInfo.name);
		}

		if (features.s_Features.features.shaderInt16 == VK_TRUE)
		{
			score += 5;
			SEDX_CORE_TRACE_TAG("Device", "Device '{}': Int16 shaders supported (+5)", deviceInfo.name);
		}

		if (features.s_Features_1_3.subgroupSizeControl == VK_TRUE)
		{
			score += 5;
			SEDX_CORE_TRACE_TAG("Device", "Device '{}': Subgroup size control supported (+5)", deviceInfo.name);
		}

		// Vulkan 1.4 features (8 points each)
		if (features.s_Features_1_4.pushDescriptor == VK_TRUE)
		{
			score += 8;
			SEDX_CORE_TRACE_TAG("Device", "Device '{}': Push descriptors supported (+8)", deviceInfo.name);
		}

		SEDX_CORE_TRACE_TAG("Device", "Device '{}' scored: {} points", deviceInfo.name, score);
		return score;
	}

	// -----------------------------------------------------------------

	/**
	 * @brief Device selection candidate with scoring information
	 */
	struct DeviceCandidate
	{
		uint32_t index = UINT32_MAX;
		int32_t featureScore = -1;
		uint32_t memory = 0;
		DeviceType type = DeviceType::MaxEnum;

		/**
		 * @brief Check if this candidate is valid (has a valid index and non-negative feature score)
		 * @return True if valid, false otherwise
		 */
		[[nodiscard]] bool IsValid() const { return index != UINT32_MAX && featureScore >= 0; }

		/**
		 * @brief Compare this candidate to another to determine if it is a better choice based on feature score and memory.
		 * @param other The other candidate to compare against
		 * @return True if this candidate is better than the other, false otherwise
		 */
		[[nodiscard]] bool IsBetterThan(const DeviceCandidate &other) const
		{
			if (!IsValid())
			{
				return false;
			}
			if (!other.IsValid())
			{
				return true;
			}

			// Prioritize feature score, then memory
			if (featureScore != other.featureScore)
			{
				return featureScore > other.featureScore;
			}

			return memory > other.memory;
		}
	};

#pragma endregion

	/**
	 * @brief Choose the best physical device based on feature support and memory, and set it as the active device.
	 * @param instance Vulkan instance handle
	 * @param gpuProps Required instance properties (features, extensions, etc.) to evaluate against each device
	 */
	Device::Device(const VkInstance instance, GraphicsChecks::InstanceProperties gpuProps) : m_InstanceProps(std::move(gpuProps))
	{
		m_Instance = instance;
		SEDX_CORE_ASSERT(m_Instance != VK_NULL_HANDLE, "Vulkan instance is null.");
		PopulatePhysicalDevices(m_Instance);

		// Select best device
		m_PhysicalDevice = Choose();
		SEDX_CORE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "No suitable physical device found");

		// Query properties from the selected device
		VkPhysicalDeviceProperties2 deviceProps{};
		deviceProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		vkGetPhysicalDeviceProperties2(m_PhysicalDevice, &deviceProps);
		m_PhysDeviceProp = deviceProps;
		m_GPUName = m_PhysDeviceProp.properties.deviceName;

		// Check subpar device status
		if (s_SubparDevice)
		{
			const HWDeviceInfo &selectedDeviceInfo = s_PhysicalDevice[m_PhysicalDeviceIndex];
			SEDX_CORE_WARN_TAG("Device", "Selected device '{}' is below minimum recommended requirements: \n", selectedDeviceInfo.name,
							   "- Memory: {} MB (recommended: 4096 MB or higher) \n", selectedDeviceInfo.memory);
			SEDX_CORE_WARN("- Performance may be degraded");
			// TODO: Show warning window to user
		}

		// Detect queue families first (static method, no Device instance needed)
		const QueueManager::QueueFamilyIndices familyIndices = QueueManager::DetectQueueFamilies(m_PhysicalDevice);

		// Store family indices for device creation
		m_FamilyIndices = familyIndices;

		// Create logical device with detected queue families
		m_LogicalDevice = Create();
		volkLoadDevice(m_LogicalDevice);
		SEDX_CORE_ASSERT(m_LogicalDevice != VK_NULL_HANDLE, "Failed to create logical device");

		// Initialize QueueManager after device is created
		QueueManager::QueueConfig config{};
		config.cmdListsPerQueue = 4; // Default value, adjust as needed
		m_QueueManager = CreateRef<QueueManager>(this, config);
		SEDX_CORE_ASSERT(m_QueueManager, "Failed to create QueueManager");

		// Initialize memory allocator
		MemoryAllocator::Init(this);
		SEDX_CORE_TRACE_TAG("Device", "Device initialization complete");
	}

	Device::~Device()
	{
		SEDX_CORE_TRACE_TAG("Device", "Device destructor called");

		if (m_PhysicalDevice != VK_NULL_HANDLE)
		{
			m_PhysicalDevice = VK_NULL_HANDLE;
		}

		if (m_LogicalDevice != VK_NULL_HANDLE)
		{
			vkDestroyDevice(m_LogicalDevice, nullptr);
			m_LogicalDevice = VK_NULL_HANDLE;
			SEDX_CORE_TRACE_TAG("Device", "Logical device destroyed");
		}

		/*if (m_WindowSurface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(m_Instance, m_WindowSurface, nullptr);
			m_WindowSurface = VK_NULL_HANDLE;
		}*/
		// QueueManager cleanup is automatic via smart pointers
		if (m_QueueManager)
		{
			m_QueueManager.Reset();
		}

		m_LogicalDevice = VK_NULL_HANDLE;
		m_PhysicalDevice = VK_NULL_HANDLE;
	}

	void Device::SetVariableRateShading(const CommandList *cmd, const bool enabled)
	{
		if (!GetDeviceStatics().isShadingRateSupported)
			return;

		// set the fragment shading rate state for the current pipeline
		VkExtent2D fragment_size = {.width = 1, .height = 1 };
		VkFragmentShadingRateCombinerOpKHR combinerOps[2];

		// the combiners determine how the different shading rate values for the pipeline, primitives and attachment are combined
		if (enabled)
		{
			// if shading rate from attachment is enabled, we set the combiner, so that the values from the attachment are used
			// combiner for pipeline (a) and primitive (b) - Not used in this sample
			combinerOps[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;
			// combiner for pipeline (a) and attachment (b), replace the pipeline default value (fragment_size) with the fragment sizes stored in the attachment
			combinerOps[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR;
		}
		else
		{
			// if shading rate from attachment is disabled, we keep the value set via the dynamic state
			combinerOps[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;
			combinerOps[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;
		}

		s_FragmentShadingRate(static_cast<VkCommandBuffer>(cmd->GetCommandBuffer()), &fragment_size, combinerOps);
	}

	uint32_t Device::GetPhysicalDeviceCount() { return static_cast<uint32_t>(s_PhysicalDevice.size()); }

	uintptr_t Device::GetPhysicalDeviceHandle(const uint32_t index) { return s_PhysicalDevice[index].guid; }

	HWDeviceInfo Device::GetHWDeviceInfo(const VkPhysicalDevice device)
	{
		SEDX_CORE_ASSERT(device != VK_NULL_HANDLE, "Physical device is null");

		// Find the index of the specified physical device handle in the cached handle list
		auto itHandle = std::ranges::find(s_PhysicalDeviceHandles, device);
		SEDX_CORE_ASSERT(itHandle != s_PhysicalDeviceHandles.end(), "Physical device handle not found in cache");

		const size_t index = static_cast<size_t>(std::distance(s_PhysicalDeviceHandles.begin(), itHandle));
		SEDX_CORE_ASSERT(index < s_PhysicalDevice.size(), "Physical device index out of bounds");

		return s_PhysicalDevice[index];
	}

	const HWDeviceInfo Device::GetChosenHWDeviceInfo()
	{
		SEDX_CORE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Physical device is null");

		SEDX_CORE_ASSERT(m_PhysicalDeviceIndex < s_PhysicalDevice.size(), "Invalid physical device index");
		return s_PhysicalDevice[m_PhysicalDeviceIndex];
	}

	DeviceStatics Device::GetDeviceStatics()
	{
		SEDX_CORE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Physical device is null");
		SEDX_CORE_ASSERT(m_PhysicalDeviceIndex < s_PhysicalDevice.size(), "Invalid physical device index");

		DeviceStatics statics{};

		// Query device properties with extension chain
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProps{};
		rtPipelineProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

		VkPhysicalDeviceAccelerationStructurePropertiesKHR accelStructProps{};
		accelStructProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
		accelStructProps.pNext = &rtPipelineProps;

		VkPhysicalDeviceFragmentShadingRatePropertiesKHR shadingRateProps{};
		shadingRateProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
		shadingRateProps.pNext = &accelStructProps;

		VkPhysicalDeviceProperties2 deviceProps{};
		deviceProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProps.pNext = &shadingRateProps;

		vkGetPhysicalDeviceProperties2(m_PhysicalDevice, &deviceProps);
		const VkPhysicalDeviceLimits &limits = deviceProps.properties.limits;

		// Populate core limits
		statics.timestampPeriod						= limits.timestampPeriod;
		statics.minUniformBufferOffsetAlignment		= limits.minUniformBufferOffsetAlignment;
		statics.minStorageBufferOffsetAlignment		= limits.minStorageBufferOffsetAlignment;
		statics.optimalBufferCopyOffsetAlignment	= limits.optimalBufferCopyOffsetAlignment;
		statics.maxImageDimension1D					= limits.maxImageDimension1D;
		statics.maxImageDimension2D					= limits.maxImageDimension2D;
		statics.maxImageDimension3D					= limits.maxImageDimension3D;
		statics.maxImageDimensionCube				= limits.maxImageDimensionCube;
		statics.maxImageArrayLayers					= limits.maxImageArrayLayers;
		statics.maxPushConstantsSize				= limits.maxPushConstantsSize;

		const DeviceFeatures &features = s_PhysicalDevice[m_PhysicalDeviceIndex].s_SupportedFeatures;

		// Populate ray tracing properties (if supported)
		if (features.s_IsRayTracingSupported)
		{
			statics.shaderGroupHandleSize			= rtPipelineProps.shaderGroupHandleSize;
			statics.shaderGroupHandleAlignment		= rtPipelineProps.shaderGroupHandleAlignment;
			statics.shaderGroupBaseAlignment		= rtPipelineProps.shaderGroupBaseAlignment;
			statics.minAccelBufferOffsetAlignment	= accelStructProps.minAccelerationStructureScratchOffsetAlignment;
		}

		// Populate shading rate properties (if supported)
		if (features.s_IsShadingRateSupported)
		{
			statics.maxShadingRateTexelSizeX = shadingRateProps.maxFragmentShadingRateAttachmentTexelSize.width;
			statics.maxShadingRateTexelSizeY = shadingRateProps.maxFragmentShadingRateAttachmentTexelSize.height;
		}

		// Populate feature flags from cached data
		statics.xessSupported			= features.s_XessSupported;
	 statics.isShadingRateSupported	= features.s_IsShadingRateSupported;
		statics.isRayTracingSupported	= features.s_IsRayTracingSupported;
		statics.isBindlessSupported		= features.s_IsBindlessSupported;
		statics.wideLinesSupported		= features.s_WideLines;

		return statics;
	}

	/**
	 * @brief Retrieve the specifications of a physical device by index
	 * @param index Index of the physical device
	 * @param outDeviceInfo Output parameter to receive the device specifications
	 * @return True if the device info was successfully retrieved, false otherwise
	 */
	bool Device::GetDeviceSpec(const uint32_t index, HWDeviceInfo &outDeviceInfo)
	{
		// Validate cached list
		if (index >= s_PhysicalDevice.size())
		{
			// Fill safe defaults on failure
			std::memset(&outDeviceInfo, 0, sizeof(outDeviceInfo));
			outDeviceInfo.type					= DeviceType::MaxEnum;
			outDeviceInfo.vendorId				= 0;
			outDeviceInfo.memory				= 0;
			outDeviceInfo.guid					= 0;
			outDeviceInfo.data					= nullptr;
			outDeviceInfo.apiVersion[0]			= '\0';
			outDeviceInfo.driverVersion[0]		= '\0';
			outDeviceInfo.name[0]				= '\0';
			outDeviceInfo.vendorName[0]			= '\0';
			outDeviceInfo.surfaceCaps			= {};
			outDeviceInfo.s_SupportedFeatures	= {};
			IsAmd(outDeviceInfo);
			IsArm(outDeviceInfo);
			IsIntel(outDeviceInfo);
			IsNvidia(outDeviceInfo);
			IsQualcomm(outDeviceInfo);
			IsBelowMinimumRequirements(outDeviceInfo);
			return false;
		}

		outDeviceInfo = s_PhysicalDevice[index];
		return true;
	}

	/**
	 * @brief Create a logical device from the selected physical device and specified queue family index.
	 * @return VkDevice handle of the created logical device, or VK_NULL_HANDLE on failure
	 */
	VkDevice Device::Create()
	{
		SEDX_CORE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Physical device not initialized");

		// Build queue create infos from detected family indices
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::vector<float> queuePriorities;
		QueueManager::BuildQueueInfo(m_FamilyIndices, queueCreateInfos, queuePriorities);

		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

		// Get required extensions from instance properties
		std::vector<const char *> deviceExtensions = GraphicsChecks::GetExtensionList(m_InstanceProps.requestedExtensions);

		// Add VK_KHR_swapchain extension which is required for rendering
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

		// Add enabled features from detection
		DeviceFeatures features = s_PhysicalDevice[m_PhysicalDeviceIndex].s_SupportedFeatures;

		// Resolve requested extended feature set and apply it to the selected device feature snapshot.
		// This keeps feature enablement policy in one place (GraphicsChecks) while still validating
		// against the detected hardware capabilities cached in DeviceFeatures.
		GraphicsChecks::RequestedFeaturesEXTState requestedFeaturesExt{};
		GraphicsChecks::EnableRequestedFeaturesEXT(m_InstanceProps, requestedFeaturesExt);

		if (requestedFeaturesExt.enableBindless)
		{
			const bool supportsPartiallyBound = features.s_Features_1_2.descriptorBindingPartiallyBound == VK_TRUE;
			const bool supportsRuntimeArray = features.s_Features_1_2.runtimeDescriptorArray == VK_TRUE;

			features.s_Features_1_2.descriptorBindingPartiallyBound =
				(supportsPartiallyBound && requestedFeaturesExt.indexingFeatures.descriptorBindingPartiallyBound == VK_TRUE) ? VK_TRUE : VK_FALSE;
			features.s_Features_1_2.runtimeDescriptorArray =
				(supportsRuntimeArray && requestedFeaturesExt.indexingFeatures.runtimeDescriptorArray == VK_TRUE) ? VK_TRUE : VK_FALSE;

			features.s_IsBindlessSupported =
				features.s_Features_1_2.descriptorBindingPartiallyBound == VK_TRUE && features.s_Features_1_2.runtimeDescriptorArray == VK_TRUE;

			if (!features.s_IsBindlessSupported)
			{
				SEDX_CORE_WARN_TAG("Device", "Bindless descriptors were requested but are not fully supported by the selected GPU feature set");
			}
		}
		else
		{
			features.s_Features_1_2.descriptorBindingPartiallyBound = VK_FALSE;
			features.s_Features_1_2.runtimeDescriptorArray = VK_FALSE;
			features.s_IsBindlessSupported = false;
		}

		if (requestedFeaturesExt.enableFloat)
		{
		  const bool supportsAtomic = features.s_FeaturesAtomicFloat.shaderBufferFloat32Atomics == VK_TRUE;
			const bool supportsAtomicAdd = features.s_FeaturesAtomicFloat.shaderBufferFloat32AtomicAdd == VK_TRUE;

			features.s_FeaturesAtomicFloat.shaderBufferFloat32Atomics =
				(supportsAtomic && requestedFeaturesExt.atomicFloatFeatures.shaderBufferFloat32Atomics == VK_TRUE) ? VK_TRUE : VK_FALSE;
			features.s_FeaturesAtomicFloat.shaderBufferFloat32AtomicAdd =
				(supportsAtomicAdd && requestedFeaturesExt.atomicFloatFeatures.shaderBufferFloat32AtomicAdd == VK_TRUE) ? VK_TRUE : VK_FALSE;

			if (requestedFeaturesExt.atomicFloatFeatures.shaderBufferFloat32Atomics == VK_TRUE &&
				features.s_FeaturesAtomicFloat.shaderBufferFloat32Atomics != VK_TRUE)
			{
				SEDX_CORE_WARN_TAG("Device", "Requested shaderBufferFloat32Atomics is not supported by the selected GPU");
			}

			if (requestedFeaturesExt.atomicFloatFeatures.shaderBufferFloat32AtomicAdd == VK_TRUE &&
				features.s_FeaturesAtomicFloat.shaderBufferFloat32AtomicAdd != VK_TRUE)
			{
				SEDX_CORE_WARN_TAG("Device", "Requested shaderBufferFloat32AtomicAdd is not supported by the selected GPU");
			}
		}

		if (requestedFeaturesExt.enableFloat2)
		{
			const bool supportsAtomicMinMax = features.s_FeaturesAtomicFloat2.shaderBufferFloat32AtomicMinMax == VK_TRUE;
			features.s_FeaturesAtomicFloat2.shaderBufferFloat32AtomicMinMax =
				(supportsAtomicMinMax && requestedFeaturesExt.atomicFloat2Features.shaderBufferFloat32AtomicMinMax == VK_TRUE) ? VK_TRUE : VK_FALSE;

			if (requestedFeaturesExt.atomicFloat2Features.shaderBufferFloat32AtomicMinMax == VK_TRUE &&
				features.s_FeaturesAtomicFloat2.shaderBufferFloat32AtomicMinMax != VK_TRUE)
			{
				SEDX_CORE_WARN_TAG("Device", "Requested shaderBufferFloat32AtomicMinMax is not supported by the selected GPU");
			}
		}

		if (std::ranges::find(m_InstanceProps.requestedExtensions,
			GraphicsChecks::GPUExtension::EXTENDED_DYNAMIC_STATE) != m_InstanceProps.requestedExtensions.end())
		{
			if (features.s_FeaturesExtendedDynamicState.extendedDynamicState != VK_TRUE)
			{
				SEDX_CORE_WARN_TAG("Device", "Extended dynamic state extension requested, but feature extendedDynamicState is not supported by the selected GPU");
			}
		}

		features.RebuildPNextChain(); // Fix dangling pointers
		deviceCreateInfo.pNext = &features.s_Features;

		VkDevice device = VK_NULL_HANDLE;
		if (VkResult r = vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &device); r != VK_SUCCESS)
		{
			SEDX_CORE_ERROR_TAG("Device", "vkCreateDevice failed: {} (VkResult: {})", ToString(r), static_cast<int>(r));
			return VK_NULL_HANDLE;
		}

		// Load device-level function pointers via volk
		//volkLoadDevice(device);

		// CRITICAL: Validate that device-level functions were loaded correctly
		// If these are null, volk failed to load from the correct Vulkan driver
		if (vkGetDeviceQueue == nullptr || vkCreateCommandPool == nullptr || vkAllocateCommandBuffers == nullptr || vkDestroyDevice == nullptr)
		{
			SEDX_CORE_ERROR_TAG("Device", "volkLoadDevice() failed to load device-level function pointers!");
			SEDX_CORE_ERROR_TAG("Device", "vkGetDeviceQueue: {}", static_cast<void *>(vkGetDeviceQueue));
			SEDX_CORE_ERROR_TAG("Device", "vkCreateCommandPool: {}", static_cast<void *>(vkCreateCommandPool));
			SEDX_CORE_ERROR_TAG("Device", "vkAllocateCommandBuffers: {}", static_cast<void *>(vkAllocateCommandBuffers));
			SEDX_CORE_ERROR_TAG("Device", "vkDestroyDevice: {}", static_cast<void *>(vkDestroyDevice));
			SEDX_CORE_ERROR_TAG("Device", "This usually means the Vulkan loader picked the wrong driver (e.g., OpenGL instead of Vulkan)");

			// Clean up the device we just created
			if (device != VK_NULL_HANDLE)
			{
				vkDestroyDevice(device, nullptr);
			}
			return VK_NULL_HANDLE;
		}

		SEDX_CORE_TRACE_TAG("Device", "Logical device created successfully");
		SEDX_CORE_TRACE_TAG("Device", "Device-level function pointers loaded via volk");
		m_LogicalDevice = device;
		return m_LogicalDevice;
	}

	// TODO: re-enable when debugging is added back
	/*
	void Device::SetResourceName(void *resource, const ResourceType type, const char *name)
	{
		if (Debugging::IsValidationLayerEnabled()) // function pointers are not initialized if validation disabled
		{
			SEDX_CORE_ASSERT(resource != nullptr);
			SEDX_CORE_ASSERT(s_SetDebugObjectName != nullptr);

			VkDebugUtilsObjectNameInfoEXT nameInfo = {};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.pNext = nullptr;
			nameInfo.objectType = OBJECT_TYPE[static_cast<uint32_t>(type)];
			nameInfo.objectHandle = reinterpret_cast<uint64_t>(resource);
			nameInfo.pObjectName = name;

			s_SetDebugObjectName(Device::GetDevice(), &nameInfo);
		}
	}
	*/

	/**
	 * @brief Select the best physical device based on scoring of features, memory, and device type.
	 * @return VkPhysicalDevice handle of the selected device, or VK_NULL_HANDLE if no suitable device is found
	 */
	VkPhysicalDevice Device::Choose()
	{
		SEDX_CORE_TRACE("=== Selecting Physical Device ===");

		// Validate that we have devices to choose from
		SEDX_CORE_ASSERT(!s_PhysicalDevice.empty(), "No physical devices available for selection");
		SEDX_CORE_ASSERT(s_PhysicalDevice.size() == s_PhysicalDeviceHandles.size(), "Device info and handle cache size mismatch");

		if (s_PhysicalDevice.empty())
		{
			SEDX_CORE_ERROR_TAG("Device", "No physical devices found");
			return VK_NULL_HANDLE;
		}

		// Get required features/extensions from instance properties
		const GraphicsChecks::InstanceProperties requiredProps = GraphicsChecks::GetDefaultInstanceProperties();

		// Count devices by type and track best candidates
		uint32_t discreteCount = 0;
		uint32_t integratedCount = 0;
		uint32_t externalCount = 0;
		uint32_t virtualCount = 0;
		uint32_t otherCount = 0;

		DeviceCandidate bestDiscrete;
		DeviceCandidate bestIntegrated;
		DeviceCandidate bestExternal;
		DeviceCandidate bestVirtual;
		DeviceCandidate bestOther;

		SEDX_CORE_TRACE_TAG("Device", "Evaluating {} physical device(s)...", s_PhysicalDevice.size());

		// Iterate through all devices, score them, and track best per category
		for (uint32_t i = 0; i < s_PhysicalDevice.size(); ++i)
		{
			const HWDeviceInfo &deviceInfo = s_PhysicalDevice[i];
			const DeviceType type = deviceInfo.type;
			s_SubparDevice = false;

			// Calculate feature score
			const int32_t featureScore = ScoreDeviceFeatures(deviceInfo, requiredProps);

			// Create candidate
			DeviceCandidate candidate;
			candidate.index = i;
			candidate.featureScore = featureScore;
			candidate.memory = deviceInfo.memory;
			candidate.type = type;

			// Log device evaluation
			if (featureScore < 0)
			{
				SEDX_CORE_WARN_TAG("Device", "[{}] {} - {} ({} MB) - REJECTED (missing required features)", i, deviceInfo.name, deviceInfo.vendorName, deviceInfo.memory);
				continue; // Skip devices that don't meet requirements
			}

			SEDX_CORE_TRACE_TAG("Device", "[{}] {} - {} ({} MB, Score: {})", i, deviceInfo.name, deviceInfo.vendorName, deviceInfo.memory, featureScore);

			// Update best candidate per device type
			switch (type)
			{
				case DeviceType::Discrete:
					discreteCount++;
					if (candidate.IsBetterThan(bestDiscrete))
					{
						bestDiscrete = candidate;
						SEDX_CORE_TRACE_TAG("Device", "New best Discrete GPU");
					}
					break;

				case DeviceType::Integrated:
					integratedCount++;
					if (candidate.IsBetterThan(bestIntegrated))
					{
						bestIntegrated = candidate;
						SEDX_CORE_TRACE_TAG("Device", "New best Integrated GPU");
					}
					break;

				case DeviceType::External:
					externalCount++;
					if (candidate.IsBetterThan(bestExternal))
					{
						bestExternal = candidate;
						SEDX_CORE_TRACE_TAG("Device", "New best External GPU");
					}
					break;

				case DeviceType::Virtual:
					virtualCount++;
					if (candidate.IsBetterThan(bestVirtual))
					{
						bestVirtual = candidate;
						SEDX_CORE_TRACE_TAG("Device", "New best Virtual GPU");
					}
					break;

				case DeviceType::Other:
				case DeviceType::MaxEnum:
				default:
					otherCount++;
					if (candidate.IsBetterThan(bestOther))
					{
						bestOther = candidate;
						SEDX_CORE_TRACE_TAG("DEVICE", "New best Other/Unknown GPU");
					}
					break;
			}
		}

		// Log device type summary
		SEDX_CORE_INFO_TAG("Device", "Device Type Summary:");
		SEDX_CORE_INFO("- Discrete GPUs: {} (best score: {})",		discreteCount,	 bestDiscrete.IsValid() ? bestDiscrete.featureScore : -1);
		SEDX_CORE_INFO("- Integrated GPUs: {} (best score: {})",	integratedCount, bestIntegrated.IsValid() ? bestIntegrated.featureScore : -1);
		SEDX_CORE_INFO("- External GPUs: {} (best score: {})",		externalCount,	 bestExternal.IsValid() ? bestExternal.featureScore : -1);
		SEDX_CORE_INFO("- Virtual GPUs: {} (best score: {})",		virtualCount,	 bestVirtual.IsValid() ? bestVirtual.featureScore : -1);
		SEDX_CORE_INFO("- Other/Unknown: {} (best score: {})",		otherCount,		 bestOther.IsValid() ? bestOther.featureScore : -1);

		// Select best device based on priority: Discrete > Integrated > External > Virtual > Other
		DeviceCandidate selectedCandidate;

		if (bestDiscrete.IsValid())
		{
			selectedCandidate = bestDiscrete;
			SEDX_CORE_TRACE_TAG("Device", "Selected: Discrete GPU (highest priority)");
		}
		else if (bestIntegrated.IsValid())
		{
			selectedCandidate = bestIntegrated;
			SEDX_CORE_WARN_TAG("Device", "No suitable Discrete GPU, using Integrated GPU");
		}
		else if (bestExternal.IsValid())
		{
			selectedCandidate = bestExternal;
			SEDX_CORE_WARN_TAG("Device", "No suitable Discrete/Integrated GPU, using External GPU");
		}
		else if (bestVirtual.IsValid())
		{
			selectedCandidate = bestVirtual;
			SEDX_CORE_WARN_TAG("Device", "No suitable physical GPU, using Virtual GPU");
		}
		else if (bestOther.IsValid())
		{
			selectedCandidate = bestOther;
			SEDX_CORE_WARN_TAG("Device", "Using fallback device (type: Other/Unknown)");
		}
		else
		{
			// No device meets requirements
			SEDX_CORE_ERROR_TAG("Device", "No physical device meets minimum feature requirements!");
			SEDX_CORE_ASSERT(false, "No suitable physical device found");
			return VK_NULL_HANDLE;
		}

		// Validate final selection
		SEDX_CORE_ASSERT(selectedCandidate.IsValid(), "Selected device candidate is invalid");
		SEDX_CORE_ASSERT(selectedCandidate.index < s_PhysicalDevice.size(), "Selected device index out of bounds");
		SEDX_CORE_ASSERT(selectedCandidate.index < s_PhysicalDeviceHandles.size(), "Selected handle index out of bounds");

		// Store the selected device index for later reference
		m_PhysicalDeviceIndex = selectedCandidate.index;

		// Get selected device info and handle
		const HWDeviceInfo &selectedDeviceInfo = s_PhysicalDevice[selectedCandidate.index];
		const VkPhysicalDevice selectedDevice = s_PhysicalDeviceHandles[selectedCandidate.index];

		// Check if device meets minimum requirements
		if (IsBelowMinimumRequirements(selectedDeviceInfo))
		{
			s_SubparDevice = true;
		}

		// Log comprehensive selection details
		SEDX_CORE_INFO_TAG("Device", "=== Selected Physical Device ===");
		SEDX_CORE_INFO("Index: {}",	  selectedCandidate.index);
		SEDX_CORE_INFO("Name: {}",	  selectedDeviceInfo.name);
		SEDX_CORE_INFO("Vendor: {}",  selectedDeviceInfo.vendorName);
		SEDX_CORE_INFO("Type: {}",	  selectedDeviceInfo.type == DeviceType::Discrete ? "Discrete"
									: selectedDeviceInfo.type == DeviceType::Integrated ? "Integrated"
									: selectedDeviceInfo.type == DeviceType::External   ? "External"
									: selectedDeviceInfo.type == DeviceType::Virtual    ? "Virtual" : "Other");
		SEDX_CORE_INFO("Memory: {} MB",		 selectedDeviceInfo.memory);
		SEDX_CORE_INFO("Feature Score: {}",  selectedCandidate.featureScore);
		SEDX_CORE_INFO("Driver Version: {}", selectedDeviceInfo.driverVersion);
		SEDX_CORE_INFO("Vulkan API: {}",	 selectedDeviceInfo.apiVersion);

		// Log supported advanced features
		SEDX_CORE_INFO("Advanced Features:");
		SEDX_CORE_INFO("- Ray Tracing: {}",				selectedDeviceInfo.s_SupportedFeatures.s_IsRayTracingSupported ? "Yes" : "No");
		SEDX_CORE_INFO("- Variable Shading Rate: {}",	selectedDeviceInfo.s_SupportedFeatures.s_IsShadingRateSupported ? "Yes" : "No");
		SEDX_CORE_INFO("- XeSS Support: {}",			selectedDeviceInfo.s_SupportedFeatures.s_XessSupported ? "Yes" : "No");
		SEDX_CORE_INFO("- Bindless Descriptors: {}",	selectedDeviceInfo.s_SupportedFeatures.s_IsBindlessSupported ? "Yes" : "No");

		return selectedDevice;
	}

}

// -----------------------------------------------------------------

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
// ReSharper disable CppInconsistentNaming
#pragma once
#include <SceneryEditorX/renderer/vulkan/enums.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @class GraphicsChecks
	 * @brief Utility class for checking graphics API support and enabling requested features/extensions/layers.
	 */
	class GraphicsChecks
	{
	public:
		/**
		 * @enum GPUExtension
		 * @brief Enum for GPU extensions that can be requested by the application. 
		 * These are not necessarily the same as Vulkan extensions, but rather a higher-level 
		 * abstraction that the application can use to specify what functionality it wants. 
		 * The GraphicsChecks class will then translate these into the appropriate Vulkan extensions and features to enable.
		 */
		enum class GPUExtension
		{
			SWAPCHAIN					= BIT(0),
			ATOMIC_FLOAT				= BIT(1),
			FRAGMENT_SHADING_RATE		= BIT(2),
			EXTENDED_DYNAMIC_STATE		= BIT(3),
			MEMORY_BUDGET				= BIT(4),
			HDR_METADATA				= BIT(5),
			ROBUSTNESS2					= BIT(6),
			SYNCHRONIZATION2			= BIT(7),
			GET_MEMORY_REQUIREMENTS2	= BIT(8),
			MUTABLE_DESCRIPTOR_TYPE		= BIT(9),
		};

		/**
		 * @enum GPULayer
		 * @brief Enum for GPU layers that can be requested by the application.
		 */
		enum class GPULayer
		{
			VALIDATION,
			API_DUMP,
			CRASH_DIAGNOSTIC,
			GFX_RECONSTRUCT,
			SCREENSHOT,
			MONITOR,
			RENDER_DOC
		};

		/**
		 * @enum GPUFeature
		 * @brief Enum for GPU features that can be requested by the application.
		 */
		enum class GPUFeature : uint8_t
		{
			ROBUST_BUFFER_ACCESS,
			FULL_DRAW_INDEX_UINT32,
			IMAGE_CUBE_ARRAY,
			INDEPENDENT_BLEND,
			GEOMETRY_SHADER,
			TESSELLATION_SHADER,
			SAMPLE_RATE_SHADING,
			DUAL_SRC_BLEND,
			LOGIC_OP,
			MULTI_DRAW_INDIRECT,
			DRAW_INDIRECT_FIRST_INSTANCE,
			DEPTH_CLAMP,
			DEPTH_BIAS_CLAMP,
			FILL_MODE_NON_SOLID,
			DEPTH_BOUNDS,
			WIDE_LINES,
			LARGE_POINTS,
			ALPHA_TO_ONE,
			MULTI_VIEWPORT,
			SAMPLER_ANISOTROPY,
			TEXTURE_COMPRESSION_ETC2,
			TEXTURE_COMPRESSION_ASTC_LDR,
			TEXTURE_COMPRESSION_BC,
			OCCLUSION_QUERY_PRECISE,
			PIPELINE_STATISTICS_QUERY,
			VERTEX_PIPELINE_STORES_AND_ATOMICS,
			FRAGMENT_STORES_AND_ATOMICS,
			TESSELLATION_AND_GEOMETRY_POINT_SIZE,
			IMAGE_GATHER_EXTENDED,
			STORAGE_IMAGE_EXTENDED_FORMATS,
			STORAGE_IMAGE_MULTISAMPLE,
			STORAGE_IMAGE_READ_WITHOUT_FORMAT,
			STORAGE_IMAGE_WRITE_WITHOUT_FORMAT,
			UNIFORM_BUFFER_ARRAY_DYNAMIC_INDEXING,
			SAMPLED_IMAGE_ARRAY_DYNAMIC_INDEXING,
			STORAGE_BUFFER_ARRAY_DYNAMIC_INDEXING,
			STORAGE_IMAGE_ARRAY_DYNAMIC_INDEXING,
			SHADER_CLIP_DISTANCE,
			SHADER_CULL_DISTANCE,
			SHADER_FLOAT64,
			SHADER_INT64,
			SHADER_INT16,
			SHADER_DRAW_PARAMS,
			RESOURCE_RESIDENCY,
			RESOURCE_MIN_LOD,
			SPARSE_BINDING,
			SPARSE_RESIDENCY_BUFFER,
			SPARSE_RESIDENCY_IMAGE_2D,
			SPARSE_RESIDENCY_IMAGE_3D,
			SPARSE_RESIDENCY_2_SAMPLES,
			SPARSE_RESIDENCY_4_SAMPLES,
			SPARSE_RESIDENCY_8_SAMPLES,
			SPARSE_RESIDENCY_16_SAMPLES,
			SPARSE_RESIDENCY_ALIASED,
			VARIABLE_MULTISAMPLE_RATE,
			INHERITED_QUERIES
		};

		/**
		 * @enum GPUFeatureEXT
		 * @brief Enum for extended GPU features that can be requested by the application.
		 */
		enum class GPUFeatureEXT : uint8_t
		{
			ATOMICS_FLOAT32,
			ATOMIC_ADD_FLOAT32,
			ATOMIC_MIN_MAX_FLOAT32_2,
			BINDLESS_SUPPORT,
			SHADER_DRAW_PARAMETERS,
		};

		/**
		 * @struct InstanceProperties
		 * @brief Struct for storing requested GPU properties, including extensions, layers, and features.
		 */
		struct InstanceProperties
		{
			std::vector<GPUExtension> requestedExtensions;
			std::vector<GPULayer> requestedLayers;
			std::vector<GPUFeature> requestedFeatures;
			std::vector<GPUFeatureEXT> requestedFeaturesEXT;
		};

		/**
		 * @struct RequestedFeaturesEXTState
		 * @brief Resolved Vulkan feature state generated from requested extended features.
		 */
		struct RequestedFeaturesEXTState
		{
			VkPhysicalDeviceShaderAtomicFloatFeaturesEXT atomicFloatFeatures{};
			VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT atomicFloat2Features{};
			VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
			VkPhysicalDeviceShaderDrawParametersFeatures drawParametersFeatures{};
			VkPhysicalDeviceFragmentShadingRateFeaturesKHR fragShadingRateFeatures{};
			VkPhysicalDeviceFeatures2 features2{};
			VkPhysicalDeviceRobustness2FeaturesEXT robustness2Features{};
			VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptorFeatures{};
			VkPhysicalDeviceVulkan11Features features_1_1{};
			VkPhysicalDeviceVulkan12Features features_1_2{};
			VkPhysicalDeviceVulkan13Features features_1_3{};
			VkPhysicalDeviceVulkan14Features features_1_4{};
			bool enableFloat = false;
			bool enableFloat2 = false;
			bool enableBindless = false;
			bool enableDrawParameters = false;
		};

		/**
		 * @brief Get the default instance properties with commonly used extensions, layers, and features. 
		 * @return The default instance properties.
		 */
		static InstanceProperties GetDefaultInstanceProperties();

		/**
		 * @brief Enable the requested extended GPU features.
		 * @param gpuProps The instance properties containing the requested features.
	  * @param outState Output structure containing resolved Vulkan feature structs and enable flags.
		 */
	 static void EnableRequestedFeaturesEXT(const InstanceProperties &gpuProps, RequestedFeaturesEXTState &outState);

		/**
		 * @brief Check if a Vulkan extension is supported.
		 * @param extension The name of the extension to check.
		 * @param props The list of available Vulkan extension properties.
		 * @return True if the extension is supported, false otherwise.
		 */
		static bool CheckExtension(const char* extension, const std::vector<VkExtensionProperties> &props);

		/**
		 * @brief Get the list of Vulkan extension names for the requested GPU extensions.
		 * @param requestedExtensions The list of requested GPU extensions.
		 * @return A vector of Vulkan extension names.
		 */
		static std::vector<const char*> GetExtensionList(const std::vector<GPUExtension> &requestedExtensions);

		/**
		 * @brief Get the Vulkan API version supported by the instance.
		 * @return The Vulkan API version.
		 */
		static uint32_t GetVulkanAPIVersion();

		/**
		 * @brief Get the name of a Vulkan layer.
		 * @param layer The GPU layer.
		 * @return The name of the Vulkan layer.
		 */
		static const char* GetLayerName(GPULayer layer);

		/**
		 * @brief Get the name of a Vulkan extension.
		 * @param extension The GPU extension.
		 * @return The name of the Vulkan extension.
		 */
		static const char* GetExtensionName(GPUExtension extension);

		/**
		 * @brief Enable a specific GPU feature.
		 * @param deviceFeatures The Vulkan structure for device features.
		 * @param supportedFeatures The Vulkan structure for supported features.
		 * @param feature The GPU feature to enable.
		 */
		static void EnableGPUFeature(VkPhysicalDeviceFeatures &deviceFeatures, const VkPhysicalDeviceFeatures &supportedFeatures, GPUFeature feature);

		/**
		 * @brief Get the list of Vulkan layer names for the requested GPU layers.
		 * @param requestedLayers The list of requested GPU layers.
		 * @return A vector of Vulkan layer names.
		 */
		static std::vector<const char*> GetLayerList(const std::vector<GPULayer> &requestedLayers);

		/**
		 * @brief Check if the Vulkan API version is supported.
		 * @param minVulkanVersion The minimum Vulkan API version required.
		 * @return True if the Vulkan API version is supported, false otherwise.
		 */
		static bool CheckAPIVersion(uint32_t minVulkanVersion);

		/**
		 * @brief Check if the Vulkan validation layers are supported.
		 * @return True if the validation layers are supported, false otherwise.
		 */
		static bool CheckValidationLayerSupport();

		/**
		 * @brief Check if the specified Vulkan layers are supported.
		 * @param layers The list of Vulkan layers to check.
		 */
		static void CheckLayers(const std::vector<const char*> &layers);

		/**
		 * @brief Check if the specified Vulkan extensions are supported.
		 * @param extensions The list of Vulkan extensions to check.
		 */
		static void CheckExtensions(const std::vector<const char*> &extensions);
	
		bool portabilityEnumEnabled = false;
	};
	
} // namespace SceneryEditorX

// -------------------------------------------------------

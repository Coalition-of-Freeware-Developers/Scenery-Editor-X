/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* render_data.h
* -------------------------------------------------------
* Created: 8/4/2025
* -------------------------------------------------------
*/
// ReSharper disable CppVariableCanBeMadeConstexpr
#pragma once
#include <colors.h>
#include "vk_buffers.h"
#include "vk_includes.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /// -------------------------------------------------------

	#define VK_FLAGS_NONE 0
	#define DEFAULT_FENCE_TIMEOUT 100000000000

	using Flags = uint32_t;

	/// ---------------------------------------------------------

    /**
     * @struct VulkanDeviceFeatures
     * @brief Holds Vulkan device features
     *
     * This structure contains a set of boolean flags that indicate
     * the availability of various Vulkan features on the device.
     * It is used to configure the Vulkan device during initialization
     * and to check for feature support.
     *
     * @return VulkanDeviceFeatures
     */
    struct VulkanDeviceFeatures
    {
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// For more information on these features, see the Vulkan specification: https://registry.khronos.org/vulkan/specs/latest/man/html/VkPhysicalDeviceFeatures.html ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        bool robustBufferAccess						 = VK_FALSE; // Accesses to buffers are bounds-checked against the range of the buffer descriptor.
        bool fullDrawIndexUint32					 = VK_FALSE; // Specifies the full 32-bit range of indices is supported for indexed draw.
        bool imageCubeArray							 = VK_FALSE; // Specifies whether image views with a VkImageViewType of VK_IMAGE_VIEW_TYPE_CUBE_ARRAY can be created.
        bool independentBlend						 = VK_TRUE;  // Specifies whether the VkPipelineColorBlendAttachmentState settings are controlled independently per-attachment.
        bool geometryShader							 = VK_TRUE;  // Geometry shader support
        bool tessellationShader						 = VK_FALSE; // Tessellation shader support
        bool sampleRateShading						 = VK_FALSE; // Specifies whether Sample Shading and multi-sample interpolation are supported.
        bool dualSrcBlend							 = VK_FALSE; // Specifies whether blend operations which take two sources are supported.
        bool logicOp								 = VK_TRUE;  // Specifies whether logic operations are supported.
        bool multiDrawIndirect						 = VK_FALSE; // Specifies whether multiple draw indirect is supported.
        bool drawIndirectFirstInstance				 = VK_FALSE; // Specifies whether indirect drawing calls support the firstInstance parameter.
        bool depthClamp								 = VK_TRUE;  // Specifies whether depth clamping is supported.
        bool depthBiasClamp							 = VK_FALSE; // Specifies whether depth bias clamping is supported.
        bool fillModeNonSolid						 = VK_TRUE;  // Enable wireframe and point rendering modes. Specifies whether point and wireframe fill modes are supported.
        bool depthBounds							 = VK_FALSE; // Specifies whether depth bounds tests are supported.
        bool wideLines								 = VK_TRUE;  // Enable wide lines for rendering. Specifies whether lines with width other than 1.0 are supported.
        bool largePoints							 = VK_FALSE; // Specifies whether points with size greater than 1.0 are supported.
        bool alphaToOne								 = VK_FALSE; // Specifies whether the impl is able to replace the alpha value of the fragment shader color output in the Multisample Coverage fragment operation.
        bool multiViewport							 = VK_TRUE;  // Specifies whether more than one viewport is supported.
        bool samplerAnisotropy						 = VK_TRUE;  // Enable anisotropic filtering for textures.
        bool textureCompressionETC2					 = VK_FALSE; // Specifies whether all the ETC2 and EAC compressed texture formats are supported.
        bool textureCompressionASTC_LDR				 = VK_FALSE; // Specifies whether all the ASTC LDR compressed texture formats are supported.
        bool textureCompressionBC					 = VK_FALSE; // Specifies whether all the BC compressed texture formats are supported.
        bool occlusionQueryPrecise					 = VK_FALSE; // Specifies whether occlusion queries returning actual sample counts are supported.
        bool pipelineStatisticsQuery				 = VK_TRUE;  // Specifies whether the pipeline statistics queries are supported.
        bool vertexPipelineStoresAndAtomics			 = VK_FALSE; // Specifies whether storage buffers and images support stores and atomic operations in the vertex, tessellation, and geometry shader stages.
        bool fragmentStoresAndAtomics				 = VK_TRUE;  // Specifies whether storage buffers and images support stores and atomic operations in the fragment shader stage.
        bool shaderTessellationAndGeometryPointSize  = VK_FALSE; // Specifies whether the PointSize built-in decoration is available in the tessellation control, evaluation, and geometry shader stages.
        bool shaderImageGatherExtended               = VK_FALSE; // Specifies whether the extended set of image gather instructions are available in shader code.
        bool shaderStorageImageExtendedFormats		 = VK_FALSE; // Specifies whether all the storage image extended formats are supported.
        bool shaderStorageImageMultisample			 = VK_FALSE; // Specifies whether multi-sampled storage images are supported.
        bool shaderStorageImageReadWithoutFormat	 = VK_TRUE;  // Specifies whether storage images and storage texel buffers require a format qualifier to be specified when reading.
        bool shaderStorageImageWriteWithoutFormat	 = VK_FALSE; // Specifies whether storage images and storage texel buffers require a format qualifier to be specified when writing.
        bool shaderUniformBufferArrayDynamicIndexing = VK_FALSE; // Specifies whether arrays of uniform buffers can be indexed by integer expressions that are dynamically uniform within the subgroup or the invocation group in shader code.
        bool shaderSampledImageArrayDynamicIndexing  = VK_FALSE; // Specifies whether arrays of samplers or sampled images can be indexed by integer expressions that are dynamically uniform within either the subgroup or the invocation group in shader code.
        bool shaderStorageBufferArrayDynamicIndexing = VK_FALSE; // Specifies whether arrays of storage buffers can be indexed by integer expressions that are dynamically uniform within either the subgroup or the invocation group in shader code.
        bool shaderStorageImageArrayDynamicIndexing  = VK_FALSE; // Specifies whether arrays of storage images can be indexed by integer expressions that are dynamically uniform within either the subgroup or the invocation group in shader code.
        bool shaderClipDistance                      = VK_FALSE; // Specifies whether clip distances are supported in shader code.
        bool shaderCullDistance                      = VK_FALSE; // Specifies whether cull distances are supported in shader code.
        bool shaderFloat64							 = VK_FALSE; // Specifies whether 64-bit floats (doubles) are supported in shader code.
        bool shaderInt64							 = VK_FALSE; // Specifies whether 64-bit integers (signed and unsigned) are supported in shader code.
        bool shaderInt16							 = VK_FALSE; // Specifies whether 16-bit integers (signed and unsigned) are supported in shader code.
        bool shaderResourceResidency				 = VK_FALSE; // Specifies whether image operations that return resource residency information are supported in shader code.
        bool shaderResourceMinLod					 = VK_FALSE; // Specifies whether image operations specifying the minimum resource LOD are supported in shader code.
        bool sparseBinding							 = VK_FALSE; // Specifies whether resource memory can be managed at opaque sparse block level instead of at the object level.
        bool sparseResidencyBuffer					 = VK_FALSE; // Specifies whether the device can access partially resident buffers.
        bool sparseResidencyImage2D					 = VK_FALSE; // Specifies whether the device can access partially resident 2D images with 1 sample per pixel.
        bool sparseResidencyImage3D					 = VK_FALSE; // Specifies whether the device can access partially resident 3D images.
        bool sparseResidency2Samples				 = VK_FALSE; // Specifies whether the physical device can access partially resident 2D images with 2 samples per pixel.
        bool sparseResidency4Samples				 = VK_FALSE; // Specifies whether the physical device can access partially resident 2D images with 4 samples per pixel.
        bool sparseResidency8Samples				 = VK_FALSE; // Specifies whether the physical device can access partially resident 2D images with 8 samples per pixel.
        bool sparseResidency16Samples				 = VK_FALSE; // Specifies whether the physical device can access partially resident 2D images with 16 samples per pixel.
        bool sparseResidencyAliased					 = VK_FALSE; // Specifies whether the physical device can correctly access data aliased into multiple locations.
        bool variableMultisampleRate				 = VK_FALSE; // Specifies whether all pipelines that will be bound to a command buffer during a subpass have the same value for VkPipelineMultisampleStateCreateInfo::rasterizationSamples.
        bool inheritedQueries						 = VK_FALSE; // Specifies whether a secondary command buffer may be executed while a query is active.

		/// ------------------------------------------------------------------------------------------------------------------------------------------------

        /// Helper function to initialize VkPhysicalDeviceFeatures from this struct
        [[nodiscard]] VkPhysicalDeviceFeatures GetPhysicalDeviceFeatures() const
        {
            VkPhysicalDeviceFeatures features{};
			features.robustBufferAccess = robustBufferAccess ? VK_TRUE : VK_FALSE;
			features.fullDrawIndexUint32 = fullDrawIndexUint32 ? VK_TRUE : VK_FALSE;
			features.imageCubeArray = imageCubeArray ? VK_TRUE : VK_FALSE;
			features.independentBlend = independentBlend ? VK_TRUE : VK_FALSE;
			features.geometryShader = geometryShader ? VK_TRUE : VK_FALSE;
			features.tessellationShader = tessellationShader ? VK_TRUE : VK_FALSE;
			features.sampleRateShading = sampleRateShading ? VK_TRUE : VK_FALSE;
			features.dualSrcBlend = dualSrcBlend ? VK_TRUE : VK_FALSE;
			features.logicOp = logicOp ? VK_TRUE : VK_FALSE;
			features.multiDrawIndirect = multiDrawIndirect ? VK_TRUE : VK_FALSE;
			features.drawIndirectFirstInstance = drawIndirectFirstInstance ? VK_TRUE : VK_FALSE;
			features.depthClamp = depthClamp ? VK_TRUE : VK_FALSE;
			features.depthBiasClamp = depthBiasClamp ? VK_TRUE : VK_FALSE;
			features.fillModeNonSolid = fillModeNonSolid ? VK_TRUE : VK_FALSE;
			features.depthBounds = depthBounds ? VK_TRUE : VK_FALSE;
			features.wideLines = wideLines ? VK_TRUE : VK_FALSE;
			features.largePoints = largePoints ? VK_TRUE : VK_FALSE;
			features.alphaToOne = alphaToOne ? VK_TRUE : VK_FALSE;
			features.multiViewport = multiViewport ? VK_TRUE : VK_FALSE;
			features.samplerAnisotropy = samplerAnisotropy ? VK_TRUE : VK_FALSE;
			features.textureCompressionETC2 = textureCompressionETC2 ? VK_TRUE : VK_FALSE;
			features.textureCompressionASTC_LDR = textureCompressionASTC_LDR ? VK_TRUE : VK_FALSE;
			features.textureCompressionBC = textureCompressionBC ? VK_TRUE : VK_FALSE;
			features.occlusionQueryPrecise = occlusionQueryPrecise ? VK_TRUE : VK_FALSE;
			features.pipelineStatisticsQuery = pipelineStatisticsQuery ? VK_TRUE : VK_FALSE;
			features.vertexPipelineStoresAndAtomics = vertexPipelineStoresAndAtomics ? VK_TRUE : VK_FALSE;
			features.fragmentStoresAndAtomics = fragmentStoresAndAtomics ? VK_TRUE : VK_FALSE;
			features.shaderTessellationAndGeometryPointSize = shaderTessellationAndGeometryPointSize ? VK_TRUE : VK_FALSE;
			features.shaderImageGatherExtended = shaderImageGatherExtended ? VK_TRUE : VK_FALSE;
			features.shaderStorageImageExtendedFormats = shaderStorageImageExtendedFormats ? VK_TRUE : VK_FALSE;
			features.shaderStorageImageMultisample = shaderStorageImageMultisample ? VK_TRUE : VK_FALSE;
			features.shaderStorageImageReadWithoutFormat = shaderStorageImageReadWithoutFormat ? VK_TRUE : VK_FALSE;
			features.shaderStorageImageWriteWithoutFormat = shaderStorageImageWriteWithoutFormat ? VK_TRUE : VK_FALSE;
			features.shaderUniformBufferArrayDynamicIndexing = shaderUniformBufferArrayDynamicIndexing ? VK_TRUE : VK_FALSE;
			features.shaderSampledImageArrayDynamicIndexing = shaderSampledImageArrayDynamicIndexing ? VK_TRUE : VK_FALSE;
			features.shaderStorageBufferArrayDynamicIndexing = shaderStorageBufferArrayDynamicIndexing ? VK_TRUE : VK_FALSE;
			features.shaderStorageImageArrayDynamicIndexing = shaderStorageImageArrayDynamicIndexing ? VK_TRUE : VK_FALSE;
			features.shaderClipDistance = shaderClipDistance ? VK_TRUE : VK_FALSE;
			features.shaderCullDistance = shaderCullDistance ? VK_TRUE : VK_FALSE;
			features.shaderFloat64 = shaderFloat64 ? VK_TRUE : VK_FALSE;
			features.shaderInt64 = shaderInt64 ? VK_TRUE : VK_FALSE;
			features.shaderInt16 = shaderInt16 ? VK_TRUE : VK_FALSE;
			features.shaderResourceResidency = shaderResourceResidency ? VK_TRUE : VK_FALSE;
			features.shaderResourceMinLod = shaderResourceMinLod ? VK_TRUE : VK_FALSE;
			features.sparseBinding = sparseBinding ? VK_TRUE : VK_FALSE;
			features.sparseResidencyBuffer = sparseResidencyBuffer ? VK_TRUE : VK_FALSE;
			features.sparseResidencyImage2D = sparseResidencyImage2D ? VK_TRUE : VK_FALSE;
			features.sparseResidencyImage3D = sparseResidencyImage3D ? VK_TRUE : VK_FALSE;
			features.sparseResidency2Samples = sparseResidency2Samples ? VK_TRUE : VK_FALSE;
			features.sparseResidency4Samples = sparseResidency4Samples ? VK_TRUE : VK_FALSE;
			features.sparseResidency8Samples = sparseResidency8Samples ? VK_TRUE : VK_FALSE;
			features.sparseResidency16Samples = sparseResidency16Samples ? VK_TRUE : VK_FALSE;
			features.sparseResidencyAliased = sparseResidencyAliased ? VK_TRUE : VK_FALSE;
			features.variableMultisampleRate = variableMultisampleRate ? VK_TRUE : VK_FALSE;
			features.inheritedQueries = inheritedQueries ? VK_TRUE : VK_FALSE;
            return features;
        }
    };

    /// -----------------------------------------------------------

	/**
	* @struct Extensions
	* @brief Manages Vulkan extension requirements and availability
	*
	* This structure contains information about required and available Vulkan extensions.
	* It tracks which extensions are active, which ones are required for the application,
	* and maintains lists of extensions available on the system.
	*/
    struct Extensions
    {
        /**
         * @brief Indicates which extensions are active (true) or inactive (false).
         * @result A vector of booleans where each index corresponds to an extension.
         */
        std::vector<bool> activeExtensions;

        /**
         * @brief List of extension names that are required by the application
	     */
        std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME
        };

        /**
         * @brief List of extensions available on the physical device.
         * @result A vector of VkExtensionProperties containing information about each available extension.
         */
        std::vector<VkExtensionProperties> availableExtensions;

        /**
         * @brief List of extensions available at the instance level.
         *	@result A vector of VkExtensionProperties containing information about each available instance extension.
         */
        std::vector<VkExtensionProperties> instanceExtensions;

        /**
         * @brief Count of available extensions.
         * @result The number of available extensions on the physical device.
         */
        uint32_t extensionCount = 0;
    };

	/**
	 * @struct Layers
	 * @brief Manages Vulkan validation layers for debugging and validation purposes
	 *
	 * This structure contains information about available and active Vulkan validation layers.
	 * Validation layers provide debug information, error checking, and validation during
	 * development to help catch API usage errors and performance issues.
	 */
    struct Layers
    {
        /** @brief Tracks which validation layers are active (true) or inactive (false) */
        std::vector<bool> activeLayers;

        /** @brief Standard validation layer for Vulkan debugging
         *
		 * Includes the Khronos validation layer which contains most validation functionality:
		 *
		 * - Parameter validation
		 * - Object lifetime tracking
		 * - Thread safety validation
		 * - API state validation
		 * - Shader validation
		 */
        std::vector<const char *> validationLayer = {"VK_LAYER_KHRONOS_validation"};

        /** @brief Names of layers that are currently activated in the application */
        std::vector<const char *> activeLayersNames;

        /** @brief Properties of all available Vulkan validation layers on the system */
        std::vector<VkLayerProperties> layers;

        /** @brief Count of available validation layers */
        uint32_t layerCount = 0;
    };

    /// -----------------------------------------------------------

    /**
	 * @struct CommandResources
	 * @brief Holds command buffer and synchronization resources for Vulkan queues
	 *
	 * This structure encapsulates the command buffer, synchronization primitives,
	 * and other resources needed for recording and executing commands on a Vulkan queue.
	 * It is designed to support multiple command buffers and synchronization objects
	 */
    struct CommandResources
    {
        Buffer staging;
        VkFence fence = nullptr;
        uint32_t stagingOffset = 0;
        uint8_t *stagingCpu = nullptr;
        VkQueryPool queryPool;
        VkCommandPool pool = nullptr;
        VkCommandBuffer buffer = nullptr;

        std::vector<uint64_t> timeStamps;
        std::vector<std::string> timeStampNames;
    };

    /**
	 * @struct InternalQueue
	 * @brief Represents a Vulkan queue and its associated command resources.
	 *
	 * InternalQueue encapsulates a Vulkan queue along with its family index and a collection
	 * of associated command resources. It provides the foundation for managing Vulkan command
	 * execution across different queue types (graphics, compute, transfer, etc.).
	 *
	 * Each queue can have multiple associated command resources allowing for parallel command
	 * buffer recording and submission, which is particularly useful when implementing
	 * multithreaded rendering operations.
	 */
    struct InternalQueue
    {
        /** @brief Queue family index to which this queue belongs (-1 indicates uninitialized) */
        int family = -1;

        /** @brief Handle to the Vulkan queue object */
        VkQueue queue = nullptr;

        /**
	     * @brief Collection of command resources associated with this queue.
	     *
	     * Each CommandResources instance can contain command pools, buffers, and synchronization
	     * primitives required for command execution on this queue. Multiple command resources
	     * enable parallel command recording from different threads.
	     */
        std::vector<CommandResources> commands;
    };

    /// -----------------------------------------------------------

	/**
	 * @struct Viewport
	 * @brief Represents a viewport for rendering in the scene editor.
	 *
	 * The Viewport structure encapsulates all data related to a rendering viewport, including
	 * its dimensions, position, aspect ratio, and associated Vulkan resources. This structure
	 * is used to configure and manage separate viewports for scene editing, allowing multiple
	 * views into the same scene with different perspectives or visualization modes.
	 */
    struct ViewportData
    {
        /** @brief Position of the viewport in the scene editor */
        float x = 0.0f;
        float y = 0.0f;

        /** @brief Width and Height of the viewport in pixels */
        float width = 0.0f;
        float height = 0.0f;

        /** @brief minimum and maximum depth value for the viewport (near plane) (far plane) */
        float minDepth = 0.0f;
        float maxDepth = 1.0f;
    };

    /// -------------------------------------------------------

    /**
     * @struct LightingData
     * @brief Stores lighting configuration data for the renderer.
     *
     * The LightingData structure maintains settings related to scene lighting
     * and shadow rendering. It controls how many light sources are active in the scene
     * and the quality settings for shadow maps.
     */
    struct LightingData
    {
        /** @brief Number of active lights in the scene */
        int numLights = 0;

        /** @brief Dimension of shadow maps in pixels (both width and height) */
        int shadowMapSize = 1024;

        /** @brief Number of samples used for shadow map filtering/anti-aliasing */
        int shadowMapSamples = 4;
    };

    /// -------------------------------------------------------

    /**
     * @struct RenderData
	 * @brief Core rendering configuration and state information for the renderer.
	 *
	 * The RenderData structure serves as a central repository for renderer state, configuration,
	 * and capabilities. It maintains information about the rendering surface dimensions,
	 * swap chain configuration, frame timing, hardware capabilities, and various rendering
	 * settings used throughout the rendering pipeline.
	 *
	 * This structure is shared across different components of the renderer to ensure
	 * consistent access to rendering parameters and state.
	 */
    struct RenderData
    {
        [[nodiscard]] uint32_t GetWidth() const { return width; }
        [[nodiscard]] uint32_t GetHeight() const { return height; }
        [[nodiscard]] uint32_t GetImageIndex() const { return imageIndex; }

		/// --------------------------------------------------------

		//const char *defaultValidationLayers[] = {"VK_LAYER_KHRONOS_validation"};

        /**
		 * @brief Minimum supported Vulkan API version.
		 * The application requires at least this Vulkan version to run properly.
		 */
        static inline uint32_t minVulkanVersion = VK_API_VERSION_1_3;

        /**
		 * @brief Maximum supported Vulkan API version.
		 * The application has been tested up to this Vulkan version.
		 */
        static inline uint32_t maxVulkanVersion = VK_API_VERSION_1_4;

        /** @brief Current width of the rendering surface in pixels */
        uint32_t width = 0;

        /** @brief Current height of the rendering surface in pixels */
        uint32_t height = 0;

        /** @brief Number of mipmap levels for textures in the rendering pipeline */
        uint32_t mipLevels = 0;

        /** @brief Index of the current frame being rendered in the application's main loop */
        uint32_t currentFrame = 0;

        /** @brief Global for the current swap chain image being rendered to */
        uint32_t imageIndex = 0;

        /** @brief Global for the current frame index in the frame cycle */
        uint64_t frameIndex = 0;

        /** @brief Maximum number of frames that can be processed simultaneously (triple buffering) */
        mutable uint32_t framesInFlight = 3;

        /** @brief Maximum number of images that can be used in the swap chain */
        uint32_t maxImageCount;

        /** @brief Number of additional images beyond the minimum required by the swap chain */
        uint32_t additionalImages = 0;

        /** @brief Index of the current frame within the swap chain's cycle */
        uint32_t swapChainCurrentFrame = 0;

        /** @brief Hardware vendor name of the GPU device */
        std::string Vendor;

	    /** @brief Driver version information */
        std::string Version;

        /** @brief Name of the GPU device being used */
        std::string Device;

        /** @brief Vulkan API version supported by the device */
        struct apiVersion { int Variant = 0, Major = 0, Minor = 0, Patch = 0; };

        /**
		 * @brief Check if the swap chain needs to be recreated
		 * @return True if the swap chain is marked as dirty and needs rebuilding
		 */
        [[nodiscard]] bool GetSwapChainDirty() const { return swapChainDirty; }

        /** @brief Flag indicating if the swap chain needs to be recreated (e.g., after window resize) */
        bool swapChainDirty = false;

        /** @brief Flag indicating if the framebuffer has been resized and needs updating */
        bool framebufferResized = true;

        /** @brief Format of the swap chain images (e.g., VK_FORMAT_B8G8R8A8_UNORM) */
        VkFormat swapChainImageFormat;

        /** @brief Default sampler used for texture sampling */
        VkSampler baseSampler = VK_NULL_HANDLE;

        /** @brief Current dimensions of the swap chain surface */
        VkExtent2D swapChainExtent;

        /** @brief Supported sample counts for multisampling */
        VkSampleCountFlags sampleCounts;

        /** @brief Current MSAA sample count for rendering */
        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

        /** @brief Maximum MSAA sample count supported by the hardware */
        VkSampleCountFlagBits maxSamples = VK_SAMPLE_COUNT_1_BIT;

        /** @brief Optional index of the graphics queue family */
        std::optional<uint32_t> graphicsFamily;

        /** @brief Optional index of the presentation queue family */
        std::optional<uint32_t> presentFamily;

        /** @brief Number of active cameras in the scene */
        int cameras = 0;

        /** @brief Number of active viewports for rendering */
        int viewports = 0;

        /** @brief Set Vsync true */
        void SetVSync(const bool enabled) { VSync = enabled; }

        /** @brief Flag indicating if vertical synchronization is enabled */
        bool VSync = false;

        /** @brief Flag indicating if temporal anti-aliasing is enabled */
        bool taaEnabled = false;

        /** @brief Flag indicating if temporal anti-aliasing should use reconstruction */
        bool taaReconstruct = false;

        /**
		 * @brief Check if the renderer has all required queue families
		 * @return True if both graphics and present queue families are available
		 */
        [[nodiscard]] bool IsComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }

        /**
		 * @brief Check if a given format includes a stencil component
		 * @param format The Vulkan format to check
		 * @return True if the format includes a stencil component
		 */
        static bool HasStencilComponent(const VkFormat format) { return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT; }

        static uint32_t CalculateMipCount(uint32_t width, uint32_t height) { return (uint32_t)std::floor(std::log2((double)Min(width, height))) + 1; }

    };

    /// -------------------------------------------------------

    /// Taken from the Vulkan 3D Graphics Cookbook Second Edition.
	struct Dimensions
    {
        uint32_t width = 1, height = 1, depth = 1;

        [[nodiscard]] Dimensions divide1D(uint32_t v) const { return {.width = width / v, .height = height, .depth = depth}; }
        [[nodiscard]] Dimensions divide2D(uint32_t v) const { return {.width = width / v, .height = height / v, .depth = depth}; }
        [[nodiscard]] Dimensions divide3D(uint32_t v) const { return {.width = width / v, .height = height / v, .depth = depth / v}; }
        bool operator==(const Dimensions &other) const { return width == other.width && height == other.height && depth == other.depth; }
    };

    struct ScissorRect
    {
        uint32_t x = 0;
        uint32_t y = 0;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    /// -------------------------------------------------------

    /**
     * @brief ImageClearValue is a union that holds clear values for images in Vulkan.
     * @union ImageClearValue
     *
     * This union can store either floating-point values (Vec4) or signed/unsigned integer values (iVec4, UVec4)
     * for clearing image data. It is used to specify the clear color or value when initializing or clearing images.
     *
     * @code
     * ImageClearValue clearValue;
     * clearValue.FloatValues = Vec4{1.0f, 0.0f, 0.0f, 1.0f}; // Red color (float)
     * clearValue.IntValues   = iVec4{255, 0, 0, 255};       // Integer red (signed)
     * clearValue.UIntValues  = UVec4{255u, 0u, 0u, 255u};   // Integer red (unsigned)
     * @endcode
     */
    union ImageClearValue
    {
        Vec4 FloatValues;
        iVec4 IntValues;
        UVec4 UIntValues;
    };

	/// ClearColorValue is more compatible with C-style or raw data copying, practically identical to ImageClearValue
	/*
	union ClearColorValue
    {
        float float32[4];
        int32_t int32[4];
        uint32_t uint32[4];
    };
    */

    /**
     * @brief Offset3D represents a 3D offset in space.
     *
     * This structure holds three integer values (x, y, z) that define an offset in a 3D coordinate system.
     */
    struct Offset3D
    {
        int32_t x = 0,y = 0, z = 0;
    };

    /// -------------------------------------------------------

    // TODO: Refactor and move this when Hash code and serialization is re-implemented
    static uint64_t hash_combine(uint64_t a, uint64_t b) { return a * 31 + b; }

    // shader register slot shifts (required to produce spirv from hlsl)
    // 000-099 is push constant buffer range
    const uint32_t shader_register_shift_u = 100;
    const uint32_t shader_register_shift_b = 200;
    const uint32_t shader_register_shift_t = 300;
    const uint32_t shader_register_shift_s = 400;

    /// -------------------------------------------------------

    const float depth_dont_care = std::numeric_limits<float>::max();
    const float depth_load = std::numeric_limits<float>::infinity();

    const Color color_dont_care = Color(std::numeric_limits<float>::max(), 0.0f, 0.0f, 0.0f);
    const Color color_load = Color(std::numeric_limits<float>::infinity(), 0.0f, 0.0f, 0.0f);

    const uint8_t max_render_target_count = 8;
    const uint8_t max_constant_buffer_count = 8;
    const uint32_t stencil_dont_care = std::numeric_limits<uint32_t>::max();
    const uint32_t stencil_load = std::numeric_limits<uint32_t>::infinity();
    const uint32_t max_array_size = 16384;
    const uint32_t max_descriptor_set_count = 512;
    const uint32_t max_mip_count = 13;
    const uint32_t all_mips = std::numeric_limits<uint32_t>::max();
    const uint32_t dynamic_offset_empty = std::numeric_limits<uint32_t>::max();
    const uint32_t max_buffer_update_size = 65536; // vkCmdUpdateBuffer has a limit of 65536 bytes

    /// -------------------------------------------------------

	static const VkPolygonMode vulkan_polygon_mode[] =
	{
	    VK_POLYGON_MODE_FILL,
	    VK_POLYGON_MODE_LINE,
	    VK_POLYGON_MODE_MAX_ENUM
	};
	
	static const VkCullModeFlags vulkan_cull_mode[] =
	{
	    VK_CULL_MODE_BACK_BIT,
	    VK_CULL_MODE_FRONT_BIT,
	    VK_CULL_MODE_NONE
	};
	
	static const VkPrimitiveTopology vulkan_primitive_topology[] =
	{
	    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	    VK_PRIMITIVE_TOPOLOGY_LINE_LIST
	};
	
	static const VkFormat vulkan_format[] =
	{
	    // R
	    VK_FORMAT_R8_UNORM,
	    VK_FORMAT_R8_UINT,
	    VK_FORMAT_R16_UNORM,
	    VK_FORMAT_R16_UINT,
	    VK_FORMAT_R16_SFLOAT,
	    VK_FORMAT_R32_UINT,
	    VK_FORMAT_R32_SFLOAT,
	    // RG
	    VK_FORMAT_R8G8_UNORM,
	    VK_FORMAT_R16G16_SFLOAT,
	    VK_FORMAT_R32G32_SFLOAT,
	    // RGB
	    VK_FORMAT_B10G11R11_UFLOAT_PACK32,
	    VK_FORMAT_R32G32B32_SFLOAT,
	    // RGBA
	    VK_FORMAT_R8G8B8A8_UNORM,
	    VK_FORMAT_A2B10G10R10_UNORM_PACK32,
	    VK_FORMAT_R16G16B16A16_UNORM,
	    VK_FORMAT_R16G16B16A16_SNORM,
	    VK_FORMAT_R16G16B16A16_SFLOAT,
	    VK_FORMAT_R32G32B32A32_SFLOAT,
	    // Depth
	    VK_FORMAT_D16_UNORM,
	    VK_FORMAT_D32_SFLOAT,
	    VK_FORMAT_D32_SFLOAT_S8_UINT,
	    // Compressed
	    VK_FORMAT_BC1_RGB_UNORM_BLOCK,
	    VK_FORMAT_BC3_UNORM_BLOCK,
	    VK_FORMAT_BC5_UNORM_BLOCK,
	    VK_FORMAT_BC7_UNORM_BLOCK,
	    VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
	    // Surface
	    VK_FORMAT_B8G8R8A8_UNORM,
	    // Unknown
	    VK_FORMAT_UNDEFINED
	};
	
	static const VkObjectType vulkan_object_type[] =
	{
	    VK_OBJECT_TYPE_FENCE,
	    VK_OBJECT_TYPE_SEMAPHORE,
	    VK_OBJECT_TYPE_SHADER_MODULE,
	    VK_OBJECT_TYPE_SAMPLER,
	    VK_OBJECT_TYPE_QUERY_POOL,
	    VK_OBJECT_TYPE_DEVICE_MEMORY,
	    VK_OBJECT_TYPE_BUFFER,
	    VK_OBJECT_TYPE_COMMAND_BUFFER,
	    VK_OBJECT_TYPE_COMMAND_POOL,
	    VK_OBJECT_TYPE_IMAGE,
	    VK_OBJECT_TYPE_IMAGE_VIEW,
	    VK_OBJECT_TYPE_DESCRIPTOR_SET,
	    VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
	    VK_OBJECT_TYPE_PIPELINE,
	    VK_OBJECT_TYPE_PIPELINE_LAYOUT,
	    VK_OBJECT_TYPE_QUEUE,
	    VK_OBJECT_TYPE_UNKNOWN
	};
	
	static const VkSamplerAddressMode vulkan_sampler_address_mode[] =
	{
	    VK_SAMPLER_ADDRESS_MODE_REPEAT,
	    VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
	    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
	    VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
	};
	
	static const VkCompareOp vulkan_compare_operator[] =
	{
	    VK_COMPARE_OP_NEVER,
	    VK_COMPARE_OP_LESS,
	    VK_COMPARE_OP_EQUAL,
	    VK_COMPARE_OP_LESS_OR_EQUAL,
	    VK_COMPARE_OP_GREATER,
	    VK_COMPARE_OP_NOT_EQUAL,
	    VK_COMPARE_OP_GREATER_OR_EQUAL,
	    VK_COMPARE_OP_ALWAYS
	};
	
	static const VkStencilOp vulkan_stencil_operation[] =
	{
	    VK_STENCIL_OP_KEEP,
	    VK_STENCIL_OP_ZERO,
	    VK_STENCIL_OP_REPLACE,
	    VK_STENCIL_OP_INCREMENT_AND_CLAMP,
	    VK_STENCIL_OP_DECREMENT_AND_CLAMP,
	    VK_STENCIL_OP_INVERT,
	    VK_STENCIL_OP_INCREMENT_AND_WRAP,
	    VK_STENCIL_OP_DECREMENT_AND_WRAP
	};
	
	static const VkBlendFactor vulkan_blend_factor[] =
	{
	    VK_BLEND_FACTOR_ZERO,
	    VK_BLEND_FACTOR_ONE,
	    VK_BLEND_FACTOR_SRC_COLOR,
	    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	    VK_BLEND_FACTOR_SRC_ALPHA,
	    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	    VK_BLEND_FACTOR_DST_ALPHA,
	    VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
	    VK_BLEND_FACTOR_DST_COLOR,
	    VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
	    VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
	    VK_BLEND_FACTOR_CONSTANT_COLOR,
	    VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
	    VK_BLEND_FACTOR_SRC1_COLOR,
	    VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
	    VK_BLEND_FACTOR_SRC1_ALPHA,
	    VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA
	};
	
	static const VkBlendOp vulkan_blend_operation[] =
	{
	    VK_BLEND_OP_ADD,
	    VK_BLEND_OP_SUBTRACT,
	    VK_BLEND_OP_REVERSE_SUBTRACT,
	    VK_BLEND_OP_MIN,
	    VK_BLEND_OP_MAX
	};
	
	static const VkFilter vulkan_filter[] =
	{
	    VK_FILTER_NEAREST,
	    VK_FILTER_LINEAR
	};
	
	static const VkSamplerMipmapMode vulkan_mipmap_mode[] =
	{
	    VK_SAMPLER_MIPMAP_MODE_NEAREST,
	    VK_SAMPLER_MIPMAP_MODE_LINEAR
	};
	
	static const VkImageLayout vulkan_image_layout[] =
	{
	    VK_IMAGE_LAYOUT_GENERAL,
	    VK_IMAGE_LAYOUT_PREINITIALIZED,
	    VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
	    VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR,
	    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	    VK_IMAGE_LAYOUT_UNDEFINED
	};
	

}

/// -------------------------------------------------------

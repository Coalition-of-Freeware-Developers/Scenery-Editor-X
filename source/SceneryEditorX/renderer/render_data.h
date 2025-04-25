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
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX
{
    class MemoryAllocator;

	// -------------------------------------------------------

	#define VK_FLAGS_NONE 0
	#define DEFAULT_FENCE_TIMEOUT 100000000000

	using Flags = uint32_t;

    // -------------------------------------------------------

	struct RendererCapabilities
	{
	    std::string Vendor;
	    std::string Device;
	    std::string Version;
        GLOBAL uint32_t apiVersion;
	};

    // Identical to Vulkan's VkAccessFlagBits
    // Note: this is a bitfield
    enum class ResourceAccessFlags
    {
        None = 0,
        IndirectCommandRead = 0x00000001,
        IndexRead = 0x00000002,
        VertexAttributeRead = 0x00000004,
        UniformRead = 0x00000008,
        InputAttachmentRead = 0x00000010,
        ShaderRead = 0x00000020,
        ShaderWrite = 0x00000040,
        ColorAttachmentRead = 0x00000080,
        ColorAttachmentWrite = 0x00000100,
        DepthStencilAttachmentRead = 0x00000200,
        DepthStencilAttachmentWrite = 0x00000400,
        TransferRead = 0x00000800,
        TransferWrite = 0x00001000,
        HostRead = 0x00002000,
        HostWrite = 0x00004000,
        MemoryRead = 0x00008000,
        MemoryWrite = 0x00010000,
    };

    // Identical to Vulkan's VkPipelineStageFlagBits
    // Note: this is a bitfield
    enum class PipelineStage
    {
        None = 0,
        TopOfPipe = 0x00000001,
        DrawIndirect = 0x00000002,
        VertexInput = 0x00000004,
        VertexShader = 0x00000008,
        TesselationControlShader = 0x00000010,
        TesselationEvaluationShader = 0x00000020,
        GeometryShader = 0x00000040,
        FragmentShader = 0x00000080,
        EarlyFragmentTests = 0x00000100,
        LateFragmentTests = 0x00000200,
        ColorAttachmentOutput = 0x00000400,
        ComputeShader = 0x00000800,
        Transfer = 0x00001000,
        BottomOfPipe = 0x00002000,
        Host = 0x00004000,
        AllGraphics = 0x00008000,
        AllCommands = 0x00010000
    };

	// ---------------------------------------------------------

    struct VulkanDeviceFeatures
    {
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// For more information on these features, see the Vulkan specification: https://registry.khronos.org/vulkan/specs/latest/man/html/VkPhysicalDeviceFeatures.html ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        bool robustBufferAccess						 = VK_FALSE; // Accesses to buffers are bounds-checked against the range of the buffer descriptor.
        bool fullDrawIndexUint32					 = VK_FALSE; // Specifies the full 32-bit range of indices is supported for indexed draw.
        bool imageCubeArray							 = VK_FALSE; // Specifies whether image views with a VkImageViewType of VK_IMAGE_VIEW_TYPE_CUBE_ARRAY can be created.
        bool independentBlend						 = VK_TRUE;  // Specifies whether the VkPipelineColorBlendAttachmentState settings are controlled independently per-attachment.
        bool geometryShader							 = VK_TRUE; // Geometry shader support
        bool tessellationShader						 = VK_FALSE; // Tessellation shader support
        bool sampleRateShading						 = VK_FALSE; // Specifies whether Sample Shading and multi-sample interpolation are supported.
        bool dualSrcBlend							 = VK_FALSE; // Specifies whether blend operations which take two sources are supported.
        bool logicOp								 = VK_TRUE; // Specifies whether logic operations are supported.
        bool multiDrawIndirect						 = VK_FALSE; // Specifies whether multiple draw indirect is supported.
        bool drawIndirectFirstInstance				 = VK_FALSE; // Specifies whether indirect drawing calls support the firstInstance parameter.
        bool depthClamp								 = VK_TRUE; // Specifies whether depth clamping is supported.
        bool depthBiasClamp							 = VK_FALSE; // Specifies whether depth bias clamping is supported.
        bool fillModeNonSolid						 = VK_TRUE;  // Enable wireframe and point rendering modes. Specifies whether point and wireframe fill modes are supported.
        bool depthBounds							 = VK_FALSE; // Specifies whether depth bounds tests are supported.
        bool wideLines								 = VK_TRUE;  // Enable wide lines for rendering. Specifies whether lines with width other than 1.0 are supported.
        bool largePoints							 = VK_FALSE; // Specifies whether points with size greater than 1.0 are supported.
        bool alphaToOne								 = VK_FALSE; // Specifies whether the impl is able to replace the alpha value of the fragment shader color output in the Multisample Coverage fragment operation.
        bool multiViewport							 = VK_TRUE; // Specifies whether more than one viewport is supported.
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

		// ------------------------------------------------------------------------------------------------------------------------------------------------

        // Helper function to initialize VkPhysicalDeviceFeatures from this struct
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

    // -----------------------------------------------------------

	struct Extensions
	{
	    std::vector<bool> activeExtensions;
	    std::vector<const char *> activeExtensionsNames;
	    std::vector<const char *> requiredExtensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
				VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
				VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME
	    };
		const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
		std::vector<VkExtensionProperties> availableExtensions;
	    std::vector<VkExtensionProperties> instanceExtensions;

		uint32_t extensionCount = 0;
	};

	struct Layers
	{
		std::vector<bool> activeLayers;
	    std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
	    std::vector<const char *> activeLayersNames;
		std::vector<VkLayerProperties> layers;

	    uint32_t layerCount = 0;
	};

    // -----------------------------------------------------------

    struct BindlessResources
    {
        enum BindlessType : uint8_t
        {
            TEXTURE,
            BUFFER,
            TLAS,
            STORAGE_IMAGE
        };

        VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet bindlessDescriptorSet = VK_NULL_HANDLE;
        VkDescriptorPool bindlessDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout bindlessDescriptorLayout = VK_NULL_HANDLE;

        const uint32_t MAX_STORAGE = 8192;
        const uint32_t MAX_SAMPLED_IMAGES = 8192;
        const uint32_t MAX_STORAGE_IMAGES = 8192;

        std::vector<int32_t> availBufferRID;
        std::vector<int32_t> availImageRID;
    };

    // -----------------------------------------------------------

    enum Queue : uint8_t
    {
        Graphics = 0,
        Compute = 1,
        Transfer = 2,
        Count = 3,
    };

    struct CommandResources
    {
        //Buffer staging;
        VkFence fence = nullptr;
        uint32_t stagingOffset = 0;
        uint8_t *stagingCpu = nullptr;
        VkQueryPool queryPool;
        VkCommandPool pool = nullptr;
        VkCommandBuffer buffer = nullptr;

        std::vector<uint64_t> timeStamps;
        std::vector<std::string> timeStampNames;
    };

	struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        int32_t Graphics = -1;
        int32_t Compute = -1;
        int32_t Transfer = -1;

        [[nodiscard]] bool IsComplete() const
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct InternalQueue
    {
        int family = -1;
        VkQueue queue = nullptr;
        std::vector<CommandResources> commands;
    };

    // -----------------------------------------------------------

	struct Viewport
	{
        glm::ivec2 viewportSize = {64, 64};

	    [[nodiscard]] glm::ivec2 GetViewportSize() const {return viewportSize;}

        uint32_t x = 0;
        uint32_t y = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        float aspectRatio = 0.0f;

        VkImage viewportImage = VK_NULL_HANDLE;
        VkImageView viewportImageView = VK_NULL_HANDLE;
        VkRenderPass viewportRenderPass = VK_NULL_HANDLE;
        VkFramebuffer viewportFramebuffer = VK_NULL_HANDLE;
        VkDeviceMemory viewportImageMemory = VK_NULL_HANDLE;

        bool viewportHovered = false;
        bool viewportResized = false;
	};

    // -------------------------------------------------------

	struct LightingData
	{
        int numLights = 0;
        int shadowMapSize = 1024;
        int shadowMapSamples = 4;

	};

    // -------------------------------------------------------

	struct RenderData
	{
	    uint32_t width = 0;
	    uint32_t height = 0;
        uint32_t mipLevels = 0;
        uint32_t currentFrame = 0;
        GLOBAL const uint32_t imageIndex = 0;
        GLOBAL const uint32_t frameIndex = 0;
        GLOBAL const uint32_t framesInFlight = 3;
        uint32_t additionalImages = 0;
        uint32_t swapChainCurrentFrame = 0;

		[[nodiscard]] bool GetSwapChainDirty() const {return swapChainDirty;}

		bool swapChainDirty = false;
        bool framebufferResized = true;

		VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        VkSampler baseSampler;
        VkSampleCountFlags sampleCounts;
        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
        VkSampleCountFlagBits maxSamples = VK_SAMPLE_COUNT_1_BIT;

		VkAllocationCallbacks *allocator = VK_NULL_HANDLE;

		int cameras = 0;
	    int viewports = 0;
        bool VSync = false;
        bool taaEnabled = false;
        bool taaReconstruct = false;

        GLOBAL bool HasStencilComponent(const VkFormat format)
        {
            return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
        }
	};

} // namespace SceneryEditorX

// -------------------------------------------------------

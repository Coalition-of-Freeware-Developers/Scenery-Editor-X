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
#include <SceneryEditorX/platform/platform_states.h>
#include <vulkan/vulkan_core.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    class MemoryAllocator;

	// -------------------------------------------------------

	#define VK_FLAGS_NONE 0
	#define DEFAULT_FENCE_TIMEOUT 100000000000

	using Flags = uint32_t;

    // -------------------------------------------------------

    /**
     * @enum ResourceAccessFlags
     * @brief Represents the access flags for Vulkan resources.
     *
     * This enum is used to specify the types of access that can be performed on Vulkan resources,
     * such as buffers and images.
     * It is used in synchronization operations to ensure proper access control and memory barriers.
     *
     * @note The values in this enum are bitwise OR'd together to create a bitfield representing multiple access types.
     * @note Identical to Vulkan's VkAccessFlagBits
     */
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

    /**
     * @enum PipelineStage
     * @brief Represents the various stages of a Vulkan pipeline.
     *
     * This enum is used to specify the stages of the pipeline
     * for synchronization purposes, such as when waiting for
     * operations to complete or when setting up barriers.
     *
     * @note The values in this enum are bitwise OR'd together
     * to create a bitfield representing multiple stages.
     * @note Identical to Vulkan's VkPipelineStageFlagBits
     */
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
        /** @brief Indicates which extensions are active (true) or inactive (false) */
        std::vector<bool> activeExtensions;

        /** @brief List of extension names that are required by the application
	     *
	     * These extensions are considered essential for the application to function properly:
	     * - VK_KHR_SWAPCHAIN_EXTENSION_NAME: Required for presenting rendered images to display
	     * - VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME: Required for ray tracing acceleration structures
	     * - VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME: Required for asynchronous operations
	     * - VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME: Required for atomic operations on floating-point values
	     * - VK_EXT_DEBUG_UTILS_EXTENSION_NAME: Required for debugging and validation
	     * - VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME: Required for dynamic vertex input state
	     */
        std::vector<const char *> requiredExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            // VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME
        };

        /** @brief List of device extensions that must be supported
	     *
	     * At minimum, the swapchain extension is required for rendering to the screen
	     */
        std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
        };

        /** @brief List of extensions available on the physical device */
        std::vector<VkExtensionProperties> availableExtensions;

        /** @brief List of extensions available at the instance level */
        std::vector<VkExtensionProperties> instanceExtensions;

        /** @brief Count of available extensions */
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
     * @struct Resource
     * @brief Base class for all render-able resources in the graphics system.
     *
     * The Resource class serves as a foundation for all resources that can be 
     * tracked and managed by the rendering system. It provides basic identification
     * through a name and a unique resource ID.
     *
     * Resources include objects such as textures, buffers, shaders, and other
     * GPU-related assets that need to be managed throughout their lifecycle.
     */
    struct Resource
    {
        /** @brief Descriptive name of the resource for debugging and tracking */
        std::string name;

        /** @brief Unique identifier for the resource (-1 indicates unassigned) */
        int32_t resourceID = -1;

        /** @brief Virtual destructor to ensure proper cleanup of derived resources */
        virtual ~Resource() = default;
    };

    /**
     * @struct BindlessResources
     * @brief Manages bindless resource descriptors for efficient GPU resource access
     * 
     * Bindless resources allow the GPU to access a large number of resources (textures, buffers, etc.)
     * without rebinding descriptor sets between draw calls. This enables more efficient rendering
     * by reducing API overhead and state changes.
     * 
     * This structure maintains the Vulkan descriptor sets and pools required for bindless
     * resource access, as well as defines the maximum number of resources that can be
     * referenced in shaders without rebinding.
     */
    struct BindlessResources
    {
        /**
         * @enum BindlessType
         * @brief Defines the types of resources that can be accessed in a bindless fashion
         * 
         * Used to categorize different resource types that require different descriptor bindings.
         */
        enum BindlessType : uint8_t
        {
            TEXTURE,      ///< Regular texture resources (sampled images)
            BUFFER,       ///< Storage or uniform buffer resources
            TLAS,         ///< Top Level Acceleration Structure for ray tracing
            STORAGE_IMAGE ///< Images that support read/write operations in shaders
        };

        /** @brief Descriptor pool used for ImGui interface elements */
        VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;

        /** @brief The descriptor set containing all bindless resources */
        VkDescriptorSet bindlessDescriptorSet = VK_NULL_HANDLE;

        /** @brief Descriptor pool from which the bindless descriptor set is allocated */
        VkDescriptorPool bindlessDescriptorPool = VK_NULL_HANDLE;

        /** @brief Layout defining the organization of descriptors within the bindless set */
        VkDescriptorSetLayout bindlessDescriptorLayout = VK_NULL_HANDLE;

        /** @brief Maximum number of storage buffers or storage texel buffers that can be accessed */
        uint32_t MAX_STORAGE = 8192;

        /** @brief Maximum number of sampled images (textures) that can be accessed */
        uint32_t MAX_SAMPLED_IMAGES = 8192;

        /** @brief Maximum number of storage images that can be accessed for read/write operations */
        uint32_t MAX_STORAGE_IMAGES = 8192;
    };

    /// -----------------------------------------------------------

    /**
     * @enum Queue
     * @brief Enumeration of Vulkan queue family types used in the rendering system
     * 
     * Vulkan uses different queue families to execute different types of operations.
     * This enum provides a type-safe way to identify and reference these queue families
     * throughout the rendering system.
     */
    enum Queue : uint8_t
    {
        Graphics = 0, ///< Graphics queue family for rendering operations and drawing commands
        Compute  = 1, ///< Compute queue family for compute shader and general computation operations
        Transfer = 2, ///< Transfer queue family dedicated to memory transfer operations
        Count    = 3, ///< Total number of queue families
        Present  = 4, ///< Present queue family for presenting rendered images to the display surface
    };

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
    struct Viewport
    {
        /** @brief Position of the viewport in the scene editor */
        float x, y;

		/**
		 * @brief Constructor for initializing the viewport with default values
		 */
		constexpr Viewport() : x(0.0f), y(0.0f) {}
		constexpr Viewport(const float _x, const float _y) : x(_x), y(_y) {}

		/**
		 * @fn GetViewportPosition
		 * @brief Retrieves the current viewport position
		 * @return The x and y coordinates of the viewport as a float vector
		 */
		[[nodiscard]] Viewport GetViewportPosition() const
		{
            return {x, y};
		}

        /**
         * @fn GetViewportSize
	     * @brief Retrieves the current viewport dimensions
	     * @return The width and height of the viewport as a float vector
	     */
        [[nodiscard]] Viewport GetViewportSize() const 
        {
			return {width, height};
		}

        /** @brief Width of the viewport in pixels */
        float width = 0.0f;

        /** @brief Height of the viewport in pixels */
        float height = 0.0f;

        /** @brief Aspect ratio of the viewport (width/height) for camera projection */
        float aspectRatio = 0.0f;

        /** @brief Vulkan image resource for rendering the viewport contents */
        VkImage viewportImage = VK_NULL_HANDLE;

        /** @brief View into the viewport image resource for shader access */
        VkImageView viewportImageView = VK_NULL_HANDLE;

        /** @brief Vulkan render pass for rendering to this viewport */
        VkRenderPass viewportRenderPass = VK_NULL_HANDLE;

        /** @brief Framebuffer associated with this viewport for rendering */
        VkFramebuffer viewportFramebuffer = VK_NULL_HANDLE;

        /** @brief Device memory allocation for the viewport image */
        VkDeviceMemory viewportImageMemory = VK_NULL_HANDLE;

        /** @brief Flag indicating whether the mouse is currently hovering over the viewport */
        bool viewportHovered = false;

        /** @brief Flag indicating whether the viewport has been resized and needs updating */
        bool viewportResized = false;
    };


    // -------------------------------------------------------

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
        /** @brief Current width of the rendering surface in pixels */
        uint32_t width = 0;

        /** @brief Current height of the rendering surface in pixels */
        uint32_t height = 0;

        /** @brief Number of mipmap levels for textures in the rendering pipeline */
        uint32_t mipLevels = 0;

        /** @brief Index of the current frame being rendered in the application's main loop */
        uint32_t currentFrame = 0;

        /** @brief Global constant for the current swap chain image being rendered to */
        GLOBAL const uint32_t imageIndex = 0;

        /** @brief Global constant for the current frame index in the frame cycle */
        GLOBAL const uint32_t frameIndex = 0;

        /** @brief Maximum number of frames that can be processed simultaneously (triple buffering) */
        GLOBAL const uint32_t framesInFlight = 3;

        /** @brief Maximum number of images that can be used in the swap chain */
        uint32_t maxImageCount;

        /** @brief Number of additional images beyond the minimum required by the swap chain */
        uint32_t additionalImages = 0;

        /** @brief Index of the current frame within the swap chain's cycle */
        uint32_t swapChainCurrentFrame = 0;

        /** @brief Hardware vendor name of the GPU device */
        std::string Vendor;

        /** @brief Name of the GPU device being used */
        std::string Device;

        /** @brief Driver version information */
        std::string Version;

        /** @brief Vulkan API version supported by the device */
        uint32_t apiVersion;

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
        VkSampler baseSampler;

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
        [[nodiscard]] bool IsComplete() const
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }

        /**
		 * @brief Check if a given format includes a stencil component
		 * @param format The Vulkan format to check
		 * @return True if the format includes a stencil component
		 */
        GLOBAL bool HasStencilComponent(const VkFormat format)
        {
            return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
        }
    };


} // namespace SceneryEditorX

// -------------------------------------------------------

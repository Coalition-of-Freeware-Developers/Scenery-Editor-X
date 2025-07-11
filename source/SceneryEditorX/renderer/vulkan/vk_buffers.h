/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_buffers.h
* -------------------------------------------------------
* Created: 5/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/memory.h>
#include <SceneryEditorX/core/pointers.h>
#include <SceneryEditorX/renderer/vulkan/resource.h>
#include <SceneryEditorX/renderer/vulkan/vk_enums.h>
#include <vma/vk_mem_alloc.h>

/// --------------------------------------------

namespace SceneryEditorX
{
	using Flags = uint32_t;

	enum MemoryType : uint8_t
    {
	    GPU = 0x00000001,
	    CPU = 0x00000002 | 0x00000004,
	};
	using MemoryFlags = Flags;

    // ReSharper disable twice IdentifierTypo
    namespace BufferUsage
    { 
		enum BufferUsageFlags
		{
		    TransferSrc					= 0x00000001,
		    TransferDst					= 0x00000002,
		    UniformTexel				= 0x00000004,
		    StorageTexel				= 0x00000008,
		    Uniform						= 0x00000010,
		    Storage						= 0x00000020,
		    Index						= 0x00000040,
		    Vertex						= 0x00000080,
		    Indirect					= 0x00000100,
		    Address						= 0x00020000,
		    VideoDecodeSrc				= 0x00002000,
		    VideoDecodeDst				= 0x00004000,
		    TransformFeedback			= 0x00000800,
		    TransformFeedbackCounter	= 0x00001000,
		    ConditionalRendering		= 0x00000200,
		    AccelerationStructureInput	= 0x00080000,
		    AccelerationStructure		= 0x00100000,
		    ShaderBindingTable			= 0x00000400,
		    SamplerDescriptor			= 0x00200000,
		    ResourceDescriptor			= 0x00400000,
		    PushDescriptors				= 0x04000000,
            MicromapBuildInputReadOnly	= 0x00800000,
		    MicromapStorage				= 0x01000000,
		};

    }
    using BufferUsageFlags = Flags;

    /// -------------------------------------------------------

    /**
     * @brief Safely destroys a Vulkan buffer and frees its associated memory allocation
     * 
     * This utility function handles the clean destruction of a VkBuffer object and its
     * associated VMA allocation. It performs safety checks to ensure valid resources
     * before attempting destruction.
     * 
     * @param buffer The VkBuffer handle to destroy
     * @param allocation The VmaAllocation associated with the buffer to free
     */
    namespace VulkanMemoryUtils
    {
        void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);
    }

    /// -------------------------------------------------------

    /**
     * @struct BufferResource
     * @brief Resource wrapper for Vulkan buffers with memory management
     * 
     * BufferResource encapsulates a Vulkan buffer handle and its associated memory allocation.
     * It inherits from Resource to integrate with the engine's resource management system
     * and provides automatic cleanup through the destructor.
     */
    struct BufferResource : Resource
    {
        /**
         * @brief Vulkan buffer handle
         * 
         * The underlying VkBuffer object used by the graphics API
         */
        VkBuffer buffer = VK_NULL_HANDLE;

        /**
         * @brief VMA memory allocation associated with this buffer
         * 
         * Managed by the Vulkan Memory Allocator (VMA) library
         */
        VmaAllocation allocation;

        /**
         * @brief Native Vulkan device memory handle
         * 
         * Direct reference to the underlying VkDeviceMemory, typically managed by VMA
         */
        VkDeviceMemory memory = VK_NULL_HANDLE;

        /**
         * @brief Destroys the buffer and frees its memory allocation
         *
         * This destructor is responsible for cleaning up the Vulkan buffer and its associated
         * memory allocation. It ensures that resources are released properly to prevent memory leaks.
         *
         * @note This destructor is virtual to allow for polymorphic behavior in derived classes.
		 * @see VulkanMemoryUtils::DestroyBuffer
         */
        virtual ~BufferResource() override
        {
            VulkanMemoryUtils::DestroyBuffer(buffer, allocation);
        }

		/**
		 * @brief Provides descriptor information for the buffer resource
		 * 
		 * This method returns a ResourceDescriptorInfo that contains the necessary
		 * information for binding this buffer resource in a Vulkan pipeline.
		 * 
		 * @return ResourceDescriptorInfo Opaque pointer containing resource-specific
		 *         descriptor information. The actual type and content depend on the
		 *         specific resource implementation (e.g., VkDescriptorBufferInfo).
		 */
        virtual ResourceDescriptorInfo GetDescriptorInfo() const override
        {
            // Provide a valid descriptor info for the buffer
            return nullptr;
        }  

    };

    /// -------------------------------------------------------

    struct Buffer
    {
        void *data = nullptr;               ///< Pointer to the mapped memory region for CPU access, if applicable
        Ref<BufferResource> resource;		///< The Vulkan buffer resource
        uint64_t size = 0;					///< Size of the buffer in bytes
        BufferUsageFlags usage;				///< Usage flags for the buffer (e.g., vertex, index, uniform)
        MemoryFlags memory;					///< Memory type flags indicating where the buffer is allocated (e.g., GPU, CPU)
        [[nodiscard]] uint32_t ID() const;  ///< Unique identifier for the buffer, used for tracking and debugging

		Buffer() = default;
        explicit Buffer(const void* data, const uint64_t size = 0) : data(const_cast<void *>(data)), size(size), usage(0), memory(0) {}

		GLOBAL Buffer Copy(const Buffer& other)
		{
			Buffer buffer;
			buffer.Allocate(other.size);
			memcpy(buffer.data, other.data, other.size);
			return buffer;
		}

		GLOBAL Buffer Copy(const void *data, const uint64_t size)
		{
			Buffer buffer;
			buffer.Allocate(size);
			if(size) memcpy(buffer.data, data, size);
			return buffer;
		}

		void Allocate(const uint64_t size)
		{
			delete[] static_cast<byte *>(data);
			data = nullptr;
            this->size = size;

			if (size == 0)
				return;

			data = hnew byte[size];
		}

		void Release()
		{
			delete[] static_cast<byte *>(data);
			data = nullptr;
			size = 0;
		}

		void ZeroInitialize() const
        {
			if (data)
				memset(data, 0, size);
		}

		template<typename T>
		T& Read(const uint64_t offset = 0)
		{
			return *static_cast<T *>(static_cast<byte *>(data) + offset);
		}

		template<typename T>
		const T& Read(const uint64_t offset = 0) const
		{
			return *static_cast<T *>(static_cast<byte *>(data) + offset);
		}

        [[nodiscard]] byte* ReadBytes(const uint64_t size, const uint64_t offset) const
		{
			SEDX_CORE_ASSERT(offset + size <= this->size, "Buffer overflow!"); /// Fixed this line to use 'this->size'
			byte* buffer = hnew byte[size];
			memcpy(buffer, static_cast<byte *>(data) + offset, size);
			return buffer;
		}
				
		void Write(const void* data, const uint64_t size, const uint64_t offset = 0) const
        {
			SEDX_CORE_ASSERT(offset + size <= this->size, "Buffer overflow!");
			memcpy(static_cast<byte *>(this->data) + offset, data, size);
		}

        explicit operator bool() const
		{
			return static_cast<bool>(data);
		}

		byte& operator[](const int index)
		{
			return static_cast<byte *>(data)[index];
		}

		byte operator[](const int index) const
		{
			return static_cast<byte *>(data)[index];
		}

		template<typename T>
		T* As() const
		{
			return static_cast<T *>(data);
		}

        [[nodiscard]] uint64_t GetSize() const { return size; }
    };
	 
	/// -------------------------------------------------------

    /**
     * @brief Begins a single-use command buffer for immediate execution
     * 
     * @return VkCommandBuffer A command buffer ready to record commands
     * 
     * This function allocates a command buffer from a command pool and begins recording.
     * It's designed for operations that need to be executed immediately and only once,
     * such as resource transfers or one-time initialization tasks.
     * 
     * The command buffer must be ended and submitted using EndSingleTimeCommands()
     * after recording the desired commands.
     * 
     * @see EndSingleTimeCommands() to complete the execution
     */
    VkCommandBuffer BeginCommands();

    /**
     * @brief Creates a Vulkan buffer with specified parameters
     * 
     * @param size Size of the buffer in bytes
     * @param usage Flags specifying how the buffer will be used (vertex, index, uniform, etc.)
     * @param memory Memory type flags indicating where the buffer should be allocated (GPU, CPU, etc.)
     * @param name Optional debug name for the buffer resource
     * @return Buffer A buffer object containing the Vulkan buffer handle and allocation information
     * 
     * This function creates a Vulkan buffer with the specified size and usage flags,
     * allocating it in the appropriate memory type based on the memory parameter.
     * It abstracts away the details of buffer creation and memory allocation in Vulkan.
     */
    Buffer CreateBuffer(uint64_t size, BufferUsageFlags usage, MemoryFlags memory = MemoryType::GPU, const std::string& name = "");
    
    /**
     * @brief Copies data between two Vulkan buffers
     * 
     * @param srcBuffer Source buffer to copy from
     * @param dstBuffer Destination buffer to copy to
     * @param size Number of bytes to copy
     * 
     * This function performs a buffer-to-buffer copy operation through a command buffer,
     * typically used to transfer data from a staging buffer to a device-local buffer.
     * The operation is completed synchronously by creating, executing and destroying
     * a single-use command buffer.
     */
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    
    /**
     * @brief Copies buffer data to an image
     * 
     * @param buffer Source buffer containing image data
     * @param image Destination image to copy data to
     * @param width Width of the image in pixels
     * @param height Height of the image in pixels
     * 
     * This function transfers data from a buffer to an image, typically used when
     * loading texture data. It handles the necessary image layout transitions and
     * submits a copy command to the graphics queue. The image should be in the
     * appropriate layout for a transfer destination operation before calling this function.
     */
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	/**
     * @brief Maps a Vulkan buffer to host-accessible memory
     * 
     * @param buffer The buffer to map to CPU-accessible memory
     * @return void* A pointer to the mapped memory region
     * 
     * This function maps the specified Vulkan buffer to memory that can be
     * accessed by the CPU. It allows direct reading from or writing to the buffer
     * from the host application.
     * 
     * The buffer must have been created with appropriate memory flags that
     * support host access (e.g., MemoryType::CPU_VISIBLE). The function returns
     * a pointer to the mapped memory region which can be used for data transfer.
     * 
     * @note The caller is responsible for ensuring proper synchronization when
     * accessing the mapped memory. The memory should be unmapped when no longer needed.
     * 
     * @see UnmapBuffer() to unmap the memory when operations are complete
     */
    void* MapBuffer(BufferResource &buffer);

	/**
	 * @brief Unmaps a Vulkan buffer from host-accessible memory
	 *
	 * This function unmaps the memory previously mapped by MapBuffer().
	 * It ensures that any modifications made to the mapped memory region are
	 * properly flushed and the buffer is no longer accessible from the CPU.
	 *
	 * The buffer must have been mapped using MapBuffer() and must support host access.
	 * After calling this function, the pointer returned by MapBuffer() becomes invalid.
	 *
	 * @param buffer The buffer resource to unmap from CPU-accessible memory
	 *
	 * @see MapBuffer(BufferResource&) to map the buffer before unmapping
	 */
	void UnmapBuffer(BufferResource &buffer);

    /// ----------------------------------------------------------

    /*
    class Framebuffer;
    class Image2D;

	struct FramebufferTextureSpec
    {
        FramebufferTextureSpec() = default;
        FramebufferTextureSpec(ImageFormat format) : Format(format)
        {
        }

        ImageFormat Format;
        bool blend = true;
        FramebufferBlendMode blendMode = FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha;
        AttachmentLoadOp loadOp = AttachmentLoadOp::Inherit;
        // TODO: filtering/wrap
    };

	struct FramebufferAttachSpec
    {
        FramebufferAttachSpec() = default;
        FramebufferAttachSpec(const std::initializer_list<FramebufferTextureSpec> &attachments)
            : Attachments(attachments)
        {
        }

        std::vector<FramebufferTextureSpec> Attachments;
    };

    struct FramebufferSpec
    {
        float scale = 1.0f;
        uint32_t width = 0;
        uint32_t height = 0;
        glm::vec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        float depthClearValue = 0.0f;
        bool clearColorOnLoad = true;
        bool clearDepthOnLoad = true;

        FramebufferAttachSpec Attachments;
        uint32_t Samples = 1; // multisampling

        // TODO: Temp, needs scale
        bool NoResize = false;

        // Master switch (individual attachments can be disabled in FramebufferTextureSpecification)
        bool Blend = true;
        // None means use BlendMode in FramebufferTextureSpecification
        FramebufferBlendMode BlendMode = FramebufferBlendMode::None;

        // SwapChainTarget = screen buffer (i.e. no framebuffer)
        bool swapChainTarget = false;

        // Will it be used for transfer ops?
        bool transfer = false;

        // Note: these are used to attach multi-layered color/depth images
        Ref<Image2D> ExistingImage;
        std::vector<uint32_t> ExistingImageLayers;

        // Specify existing images to attach instead of creating
        // new images. attachment index -> image
        std::map<uint32_t, Ref<Image2D>> ExistingImages;

        // At the moment this will just create a new render pass
        // with an existing framebuffer
        Ref<Framebuffer> ExistingFramebuffer;

        std::string DebugName;
    };

    /// ----------------------------------------------------------

    class Framebuffer : public RefCounted
    {
    public:
        Framebuffer(const FramebufferSpec &spec);
		virtual ~Framebuffer() override = default;

		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false);
        virtual void AddResizeCallback(const std::function<void(Ref<Framebuffer>)> &func);
        virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0);

        virtual void Bind() const {}
        virtual void Unbind() const {} /// Bind the framebuffer for rendering
		virtual uint32_t GetWidth() const { return fb_width; }
		virtual uint32_t GetHeight() const { return fb_height; }

        virtual RID GetRendererID();

        virtual Ref<Image2D> GetImage(uint32_t attachmentIndex = 0);
        virtual Ref<Image2D> GetDepthImage();

        virtual size_t GetColorAttachmentCount();
        virtual bool HasDepthAttachment();

        VkRenderPass GetRenderPass() const { return renderPass; }
		VkFramebuffer GetFramebuffer() const { return framebuffer; }

        virtual RID GetColorAttachmentRID() const { return 0; }
		virtual RID GetDepthAttachmentRID() const { return 0; }

        static Ref<Framebuffer> Create(const FramebufferSpec &spec);
        virtual const FramebufferSpec &GetSpecification();
    private:
        uint32_t fb_width	= 0;											///< Width of the framebuffer
        uint32_t fb_height = 0;                                             ///< Height of the framebuffer
		FramebufferSpec specification;										///< Specifications for the framebuffer
        std::vector<std::function<void(Ref<Framebuffer>)>> resizeCallbacks; ///< Callbacks for resize events
        std::vector<Ref<Image2D>> attachmentImages;
        Ref<Image2D> depthAttachmentImage;

        std::vector<VkClearValue> clearValues;

        VkRenderPass renderPass = nullptr;
        VkFramebuffer framebuffer = nullptr;
    };
    */

    /// ----------------------------------------------------------

} // namespace SceneryEditorX

/// ----------------------------------------------------------

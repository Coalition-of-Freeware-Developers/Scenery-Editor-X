/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* texture.h
* -------------------------------------------------------
* Created: 8/7/2025
* -------------------------------------------------------
*/
#pragma once
//#include "SceneryEditorX/asset/asset.h"
#include "viewport.h"
#include "vulkan/vk_buffers.h"
#include "vulkan/vk_enums.h"
#include "vulkan/vk_image.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /// Forward declarations
    //class Image2D;

	enum class TextureType : uint8_t
	{
	    None = 0,
	    Texture2D,
	    Texture2DArray,
		Texture3D,
	    TextureCube,
		MaxEnum
	};

	enum TextureFlags : uint32_t
	{
	    Texture_Srv		= 1U << 0,
	    Texture_Uav		= 1U << 1,
	    Texture_Rtv		= 1U << 2,
	    Texture_Vrs		= 1U << 3,
	    ClearBlit		= 1U << 4,
	    PerMipViews		= 1U << 5,
	    Greyscale		= 1U << 6,
	    Transparent		= 1U << 7,
	    SRGB			= 1U << 8,
	    Mappable		= 1U << 9,
	    Compress		= 1U << 10,
	    ExternalMemory	= 1U << 11,
	    DontPrepForGpu	= 1U << 12,
	    Thumbnail		= 1U << 13
	};

    struct TextureMip
    {
        std::vector<std::byte> bytes;
    };

    struct TextureSlice
    {
        std::vector<TextureMip> mips;
        uint32_t GetMipCount() { return static_cast<uint32_t>(mips.size()); }
    };

	struct TextureSpecification
	{
        TextureType type;
		VkFormat format;
		uint32_t width;
		uint32_t height;
        uint32_t depth;
        uint32_t mipCount;
        uint32_t flags;
		SamplerWrap samplerWrap = SamplerWrap::Repeat;
		SamplerFilter samplerFilter = SamplerFilter::Linear;
        std::vector<TextureSlice> data = {};
        std::string debugName;

		bool generateMips = true;
		bool storage = false;
		bool storeLocally = false;
	};

    /// -------------------------------------------------------

	class Texture : public Resource
	{
	public:
        //AssetHandle handle = AssetHandle(0);

		virtual ~Texture() override = default;
        virtual void Bind(uint32_t slot = 0) const = 0;

		virtual VkFormat GetFormat() const = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual UVec2 GetSize() const = 0;

		virtual uint32_t GetMipLevelCount() const = 0;
		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const = 0;

		virtual uint64_t GetHash() const = 0;
		virtual TextureType GetType() const = 0;

		// Bindless indices (optional; valid after first Invalidate when bindless manager active)
		virtual int32_t GetBindlessImageIndex() const { return -1; }
		virtual int32_t GetBindlessSamplerIndex() const { return -1; }
	};

    /// -------------------------------------------------------

	class Texture2D : public Texture
	{
	public:
        Texture2D() = default;

        explicit Texture2D(const TextureSpecification &specification);
        Texture2D(const TextureSpecification &specification, const std::filesystem::path &filePath);
        Texture2D(const TextureSpecification &specification, const Buffer &imageData = Buffer());

		//static Ref<Texture2D> Create(const TextureSpecification &specification);
		//static Ref<Texture2D> Create(const TextureSpecification &specification, const std::filesystem::path& filePath);
        //static Ref<Texture2D> Create(const TextureSpecification &specification, const Buffer &imageData = Buffer());

		///< reinterpret the given texture's data as if it was sRGB
		static Ref<Texture2D> CreateFromSRGB(const Ref<Texture2D> &texture);

		void CreateFromFile(const TextureSpecification &specification, const std::filesystem::path& filePath);
        void ReplaceFromFile(const TextureSpecification &specification, const std::filesystem::path &filePath);
        //virtual void CreateFromBuffer(const TextureSpecification &specification, Buffer data = Buffer());

		void CreateFromBuffer(const TextureSpecification &specification, const Buffer &data);

        void Resize(const UVec2 &size);
        void Resize(uint32_t width, uint32_t height);

		void Invalidate();

		virtual VkFormat GetFormat() const override { return m_Specification.format; }
		virtual uint32_t GetWidth() const override { return m_Specification.width; }
		virtual uint32_t GetHeight() const override { return m_Specification.height; }
		virtual UVec2 GetSize() const override { return { m_Specification.width, m_Specification.height }; }

		virtual void Bind(uint32_t slot = 0) const override;

		virtual Ref<Image2D> GetImage() const { return m_Image; }
		virtual ResourceDescriptorInfo GetDescriptorInfo() const override;
		const VkDescriptorImageInfo& GetDescriptorInfoVulkan() const;

        void ClearData();
        void PrepareForGpu();
        void SaveAsImage(const std::string &file_path);

	    void SetLayout(const Layout::ImageLayout layout, CommandList *cmd_list,  uint32_t mip_index = all_mips, uint32_t mip_range = 0);
        Layout::ImageLayout GetLayout(const uint32_t mip) const;
        std::array<Layout::ImageLayout, max_mip_count> GetLayouts();

		void Lock();
		void Unlock();

		Buffer GetWriteableBuffer() const;
		bool Loaded() const;
		const std::filesystem::path& GetPath() const;
        virtual uint32_t GetMipLevelCount() const override;
		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

	    TextureMip GetMip(uint32_t array_index, uint32_t mip_index);
        TextureSlice &GetSlice(uint32_t array_index);
        void GenerateMips();
        void AllocateMip();
        static size_t CalculateMipSize(const TextureSpecification &spec, uint32_t bits_per_channel, uint32_t channel_count);

		virtual uint64_t GetHash() const override;
        void CopyToHostBuffer(Buffer &buffer) const;
        virtual TextureType GetType() const override { return TextureType::Texture2D; }

		virtual int32_t GetBindlessImageIndex() const override { return m_BindlessImageIndex; }
		virtual int32_t GetBindlessSamplerIndex() const override { return m_BindlessSamplerIndex; }

        const auto& GetViewport() const { return m_viewport; }

    private:
        void SetData(const Buffer &buffer);

        TextureSpecification m_Specification;
        std::filesystem::path m_Path;
        Buffer m_ImageData = {};
        Ref<Image2D> m_Image;
	    int32_t m_BindlessImageIndex = -1;
	    int32_t m_BindlessSamplerIndex = -1;

	protected:
        bool CreateResource();

        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_depth = 0;
        uint32_t m_mip_count = 0;
        uint32_t m_bits_per_channel = 0;
        uint32_t m_channel_count = 0;
        VkFormat m_format = VK_FORMAT_MAX_ENUM;
        TextureType m_type = TextureType::MaxEnum;
        Viewport m_viewport;
        std::vector<TextureSlice> m_slices;

	    void* m_srv = nullptr;										// an srv with all mips
        std::array<void*, max_mip_count> m_srv_mips = {nullptr};	// an srv for each mip
        std::array<void*, max_render_target_count> m_rtv = {nullptr};
        std::array<void*, max_render_target_count> m_dsv = {nullptr};
        void* m_resource = nullptr;
        void* m_externalMemory = nullptr;
	    void* m_mappedData = nullptr;
    };

    /// -------------------------------------------------------

	class TextureCube : public Texture
	{
	public:
        TextureCube(TextureSpecification specification, const Buffer &data);
		virtual ~TextureCube() override;
		void Release();

		virtual void Bind(uint32_t slot = 0) const override {}
		virtual VkFormat GetFormat() const override { return m_Specification.format; }
		virtual uint32_t GetWidth() const override{ return m_Specification.width; }
		virtual uint32_t GetHeight() const override { return m_Specification.height; }
		virtual UVec2 GetSize() const override { return { m_Specification.width, m_Specification.height}; }
	    virtual TextureType GetType() const override { return TextureType::TextureCube; }
		virtual uint32_t GetMipLevelCount() const override;
		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

		virtual uint64_t GetHash() const override { return (uint64_t)m_Image; }

		virtual ResourceDescriptorInfo GetDescriptorInfo() const override { return (ResourceDescriptorInfo)&m_DescriptorImageInfo; }
		const VkDescriptorImageInfo& GetDescriptorInfoVulkan() const { return *(VkDescriptorImageInfo*)GetDescriptorInfo(); }

		VkImageView CreateImageViewSingleMip(uint32_t mip);

		void GenerateMips(bool readonly = false);
        void CopyToHostBuffer(Buffer &buffer) const;
        void CopyFromBuffer(const Buffer &buffer, uint32_t mips) const;
	private:
		void Invalidate();
		TextureSpecification m_Specification;

		bool m_MipsGenerated = false;

		Buffer m_LocalStorage;
		VmaAllocation m_MemoryAlloc;
		uint64_t m_GPUAllocationSize = 0;
		VkImage m_Image { nullptr };
		VkDescriptorImageInfo m_DescriptorImageInfo = {};
	};

}

/// -------------------------------------------------------

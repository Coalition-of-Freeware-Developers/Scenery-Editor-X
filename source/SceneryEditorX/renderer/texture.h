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
#include <filesystem>
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/renderer/vulkan/vk_image.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	enum class TextureType : uint8_t
	{
	    None = 0,
	    Texture2D,
	    TextureCube
	};

	struct TextureSpecification
	{
		VkFormat format;
		uint32_t width = 1;
		uint32_t height = 1;
		SamplerWrap samplerWrap = SamplerWrap::Repeat;
		SamplerFilter samplerFilter = SamplerFilter::Linear;

		bool generateMips = true;
		bool storage = false;
		bool storeLocally = false;

		std::string debugName;
	};

    /// -------------------------------------------------------

	class Texture : public Resource
	{
	public:
        AssetHandle handle = AssetHandle(0);

		virtual ~Texture() override = default;
        virtual void Bind(uint32_t slot = 0) const = 0;

		virtual VkFormat GetFormat() const = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual glm::uvec2 GetSize() const = 0;

		virtual uint32_t GetMipLevelCount() const = 0;
		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const = 0;

		virtual uint64_t GetHash() const = 0;
		//virtual TextureType GetType() const = 0;
	};

    /// -------------------------------------------------------

	class Texture2D : public Texture
	{
	public:
        Texture2D() = default;

        explicit Texture2D(const TextureSpecification &specification);
        Texture2D(const TextureSpecification &specification, const std::filesystem::path &filepath);
        Texture2D(const TextureSpecification &specification, const Buffer &imagedata);
        
		static Ref<Texture2D> Create(const TextureSpecification &specification);
		static Ref<Texture2D> Create(const TextureSpecification &specification, const std::filesystem::path& filepath);
        static Ref<Texture2D> Create(const TextureSpecification &specification, const Buffer &imagedata = Buffer());

		///< reinterpret the given texture's data as if it was sRGB
		static Ref<Texture2D> CreateFromSRGB(const Ref<Texture2D> &texture);

		virtual void CreateFromFile(const TextureSpecification &specification, const std::filesystem::path& filepath);
        virtual void ReplaceFromFile(const TextureSpecification &specification, const std::filesystem::path &filepath);
        virtual void CreateFromBuffer(const TextureSpecification &specification, Buffer data = Buffer());

		//void CreateFromBuffer(const TextureSpecification &specification, const Buffer &data);

        virtual void Resize(const glm::uvec2 &size);
        virtual void Resize(uint32_t width, uint32_t height);

		void Invalidate();

		virtual VkFormat GetFormat() const override { return m_Specification.format; }
		virtual uint32_t GetWidth() const override { return m_Specification.width; }
		virtual uint32_t GetHeight() const override { return m_Specification.height; }
		virtual glm::uvec2 GetSize() const override { return { m_Specification.width, m_Specification.height }; }

		virtual void Bind(uint32_t slot = 0) const override;

		virtual Ref<Image2D> GetImage() const { return m_Image; }
		virtual ResourceDescriptorInfo GetDescriptorInfo() const override { return m_Image.As<Image2D>()->GetDescriptorInfo(); }
		const VkDescriptorImageInfo& GetDescriptorInfoVulkan() const { return *(VkDescriptorImageInfo*)GetDescriptorInfo(); }

		void Lock();
		void Unlock();

		Buffer GetWriteableBuffer() const;
		bool Loaded() const { return m_Image && m_Image->IsValid(); }
		const std::filesystem::path& GetPath() const;
        virtual uint32_t GetMipLevelCount() const override;
		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

		void GenerateMips();
		virtual uint64_t GetHash() const override { return (uint64_t)m_Image.As<Image2D>()->GetDescriptorInfoVulkan().imageView; }
        void CopyToHostBuffer(Buffer &buffer) const;

    private:
        void SetData(const Buffer &buffer);

        TextureSpecification m_Specification;
        std::filesystem::path m_Path;
        Buffer m_ImageData = {};
        Ref<Image2D> m_Image;

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
		virtual glm::uvec2 GetSize() const override { return { m_Specification.width, m_Specification.height}; }

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

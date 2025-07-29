/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_image.h
* -------------------------------------------------------
* Created: 27/6/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/renderer/image_data.h>

/// -----------------------------------------------------------

namespace SceneryEditorX
{

	struct ImageSpecification
    {
        std::string debugName;
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        ImageUsageFlags usage;
        bool transfer = false; ///< Will it be used for transfer ops?
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t mips = 1;
        uint32_t layers = 1;
        bool createSampler = true;
    };

    /// -----------------------------------------------------------

    class Image2D : public Resource
    {
    public:
        Image2D() = default;
        Image2D(const ImageSpecification &specification);
        //Image2D(const Image &specification);
        virtual ~Image2D() override;

        GLOBAL Ref<Image2D> Create(const ImageDescriptions &desc, const std::string &name = "Image2D");
        //[[nodiscard]] virtual bool Valid() const = 0;

        virtual void Resize(const glm::uvec2& size)
        {
			Resize(size.x, size.y);
		}

		virtual void Resize(const uint32_t width, const uint32_t height)
        {
			m_Specification.width = width;
			m_Specification.height = height;
			Invalidate();
		}

        virtual void Invalidate();
        virtual void Release();
        [[nodiscard]] virtual bool IsValid() const { return m_DescriptorImageInfo.imageView != nullptr; }
        [[nodiscard]] virtual uint32_t GetWidth() const { return m_Specification.width; }
		[[nodiscard]] virtual uint32_t GetHeight() const { return m_Specification.height; }
		[[nodiscard]] virtual glm::uvec2 GetSize() const { return { m_Specification.width, m_Specification.height };}
		[[nodiscard]] virtual bool HasMips() const { return m_Specification.mips > 1; }
		[[nodiscard]] virtual float GetAspectRatio() const { return (float)m_Specification.width / (float)m_Specification.height; }
		[[nodiscard]] int GetClosestMipLevel(uint32_t width, uint32_t height) const;
        [[nodiscard]] std::pair<uint32_t, uint32_t> GetMipLevelSize(int mipLevel) const;
		[[nodiscard]] virtual const ImageSpecification& GetSpecification() const { return m_Specification; }

        virtual ImageSpecification& GetSpecification() { return m_Specification; }
		void Invalidate_RenderThread();

        virtual void CreatePerLayerImageViews();
        void CreatePerLayerImageViews_RenderThread();
        void CreatePerSpecificLayerImageViews_RenderThread(const std::vector<uint32_t> &layerIndices);

		virtual VkImageView GetLayerImageView(const uint32_t layer)
		{
			SEDX_CORE_ASSERT(layer < m_PerLayerImageViews.size());
			return m_PerLayerImageViews[layer];
		}

		VkImageView GetMipImageView(uint32_t mip);
        VkImageView GetRenderThreadMipImageView(uint32_t mip);

		ImageResource &GetImageInfo() { return m_Info; }
		[[nodiscard]] const ImageResource &GetImageInfo() const { return m_Info; }

		[[nodiscard]] virtual ResourceDescriptorInfo GetDescriptorInfo() const override { return (ResourceDescriptorInfo)&m_DescriptorImageInfo; }
		[[nodiscard]] const VkDescriptorImageInfo& GetDescriptorInfoVulkan() const { return *(VkDescriptorImageInfo*)GetDescriptorInfo(); }

        [[nodiscard]] virtual Buffer GetBuffer() const { return m_ImageData; }
		[[nodiscard]] virtual Buffer &GetBuffer() { return m_ImageData; }

		[[nodiscard]] virtual uint64_t GetGPUMemoryUsage() const { return m_GPUAllocationSize; }
		[[nodiscard]] virtual uint64_t GetHash() const { return (uint64_t)m_Info.image; }

		void UpdateDescriptor();

		/// Debug
		static const std::map<VkImage, WeakRef<Image2D>>& GetImageRefs();
		virtual void SetData(Buffer buffer);
        virtual void CopyToHostBuffer(Buffer &buffer) const;
    private:
        ImageSpecification m_Specification;
        Buffer m_ImageData;
        ImageResource m_Info;
        VkDeviceSize m_GPUAllocationSize = 0;

        std::vector<VkImageView> m_PerLayerImageViews;
        std::map<uint32_t, VkImageView> m_PerMipImageViews;
        VkDescriptorImageInfo m_DescriptorImageInfo = {};
    };

    /// -----------------------------------------------------------

    struct ImageViewData
    {
        Ref<Image2D> image;
        uint32_t mip = 0;
        std::string debugName;
    };

	class ImageView : Resource
	{
	public:
        ImageView(ImageViewData spec);
        virtual ~ImageView();
		
		void Invalidate();
		void Invalidate_RenderThread();

		[[nodiscard]] VkImageView GetImageView() const { return m_ImageView; }

		[[nodiscard]] virtual ResourceDescriptorInfo GetDescriptorInfo() const override { return (ResourceDescriptorInfo)&m_DescriptorImageInfo; }
		[[nodiscard]] const VkDescriptorImageInfo &GetDescriptorInfoVulkan() const { return *(VkDescriptorImageInfo*)GetDescriptorInfo(); }
	private:
		ImageViewData m_Specification;
		VkImageView m_ImageView = nullptr;
		VkDescriptorImageInfo m_DescriptorImageInfo = {};
	};

}

/// -----------------------------------------------------------

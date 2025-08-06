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
        explicit Image2D(const ImageSpecification &specification);
        virtual ~Image2D();

        //GLOBAL Ref<Image2D> Create(const ImageDescriptions &desc, const std::string &name = "Image2D");

        void Resize(const glm::uvec2& size)
        {
			Resize(size.x, size.y);
		}

        void Resize(const uint32_t width, const uint32_t height)
        {
			m_Specification.width = width;
			m_Specification.height = height;
			Invalidate();
		}

        void Invalidate();
        void Release();

        [[nodiscard]] bool IsValid() const { return m_DescriptorImageInfo.imageView != nullptr; }
        [[nodiscard]] uint32_t GetWidth() const { return m_Specification.width; }
		[[nodiscard]] uint32_t GetHeight() const { return m_Specification.height; }
		[[nodiscard]] glm::uvec2 GetSize() const { return { m_Specification.width, m_Specification.height };}
		[[nodiscard]] bool HasMips() const { return m_Specification.mips > 1; }
		[[nodiscard]] float GetAspectRatio() const { return (float)m_Specification.width / (float)m_Specification.height; }
		[[nodiscard]] int GetClosestMipLevel(uint32_t width, uint32_t height) const;
        [[nodiscard]] std::pair<uint32_t, uint32_t> GetMipLevelSize(int mipLevel) const;

        ImageSpecification& GetSpecification() { return m_Specification; }
		const ImageSpecification& GetSpecification() const { return m_Specification; }


        void Invalidate_RenderThread();
        void CreatePerLayerImageViews();
        void CreatePerLayerImageViews_RenderThread();
        void CreatePerSpecificLayerImageViews_RenderThread(const std::vector<uint32_t> &layerIndices);

		VkImageView GetLayerImageView(uint32_t layer) const
        {
			SEDX_CORE_ASSERT(layer < m_PerLayerImageViews.size());
			return m_PerLayerImageViews[layer];
		}

		VkImageView GetMipImageView(uint32_t mip);
        VkImageView GetRenderThreadMipImageView(uint32_t mip);

		ImageResource &GetImageInfo() { return m_Info; }
		[[nodiscard]] const ImageResource &GetImageInfo() const { return m_Info; }
		[[nodiscard]] virtual ResourceDescriptorInfo GetDescriptorInfo() const override { return (ResourceDescriptorInfo)&m_DescriptorImageInfo; }
		[[nodiscard]] const VkDescriptorImageInfo &GetDescriptorInfoVulkan() const { return *(VkDescriptorImageInfo*)GetDescriptorInfo(); }

        [[nodiscard]] Buffer GetBuffer() const { return m_ImageData; }
		[[nodiscard]] Buffer &GetBuffer() { return m_ImageData; }

		[[nodiscard]] uint64_t GetGPUMemoryUsage() const { return m_GPUAllocationSize; }
		[[nodiscard]] uint64_t GetHash() const { return (uint64_t)m_Info.image; }

		void UpdateDescriptor();

		/// Debug
		GLOBAL const std::map<VkImage, WeakRef<Image2D>> &GetImageRefs();
		void SetData(Buffer buffer);
        void CopyToHostBuffer(Buffer &buffer) const;
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

	class ImageView : public Resource
	{
	public:
        ImageView(const ImageViewData &spec);
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

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_image_view.h
* -------------------------------------------------------
* Created: 8/8/2025
* -------------------------------------------------------
*/
#pragma once
#include "vk_image.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

    struct ImageViewData
    {
        Ref<Image2D> image;
        uint32_t mip = 0;
        std::string debugName;
    };

    /// -----------------------------------------------------------

	class ImageView : public Resource
	{
	public:
        explicit ImageView(const ImageViewData &spec);
        virtual ~ImageView() override;
		
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

/// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* descriptor_set.cpp
* -------------------------------------------------------
* Created: 13/12/2025
* -------------------------------------------------------
*/
#include "render_context.h"
#include "descriptors.h"
#include "descriptor_set.h"
#include "renderer.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
    void DescriptorSet::Update(const std::vector<Descriptors>& descriptors)
    {
        m_Descriptors = descriptors;
        auto device = RenderContext::GetCurrentDevice()->GetDevice();

        // Validate descriptor set
        SEDX_ASSERT(m_Resource != nullptr);

        const uint32_t descriptor_count = 256;
        std::array<VkWriteDescriptorSet, descriptor_count> descriptor_sets;

        std::vector<VkDescriptorImageInfo> info_images;
        info_images.resize(descriptor_count);
        info_images.reserve(descriptor_count);
        int image_index = -1;

        std::vector<VkDescriptorBufferInfo> info_buffers;
        info_buffers.resize(descriptor_count);
        info_buffers.reserve(descriptor_count);

        std::vector<VkWriteDescriptorSetAccelerationStructureKHR> info_accel_structs;
        info_accel_structs.resize(descriptor_count);
        info_accel_structs.reserve(descriptor_count);
        std::vector<VkAccelerationStructureKHR> accel_struct_handles;
        accel_struct_handles.reserve(descriptor_count);
        int accel_index = -1;

        uint32_t index = 0;
        descriptor_sets = {};
        for (const Descriptors& descriptor : descriptors)
        {
            // in case of a null texture (which is legal), don't skip it
            // set a checkerboard texture instead, this way if it's sampled (which is wrong), we'll see it
            if (!descriptor.data && descriptor.type != DescriptorType::Image)
                continue;

            // the bindless texture array has its own descriptor
            bool bindless_array = descriptor.as_array && descriptor.array_length == max_array_size;
            if (bindless_array)
                continue;

            uint32_t descriptor_index_start = 0;
            uint32_t descriptor_cnt       = 1;

            if (descriptor.type == DescriptorType::Image || descriptor.type == DescriptorType::TextureStorage)
            {
                Texture* texture     = static_cast<Texture*>(descriptor.data);
                const bool mip_specified = descriptor.mip != all_mips;
                uint32_t mip_start       = mip_specified ? descriptor.mip : 0;

                // get texture, if unable to do so, fallback to a checkerboard texture, so we can spot it by eye
                void* srv_fallback = nullptr;
                if (Texture* texture_it = Renderer::GetStandardTexture(Renderer_StandardTexture::Checkerboard))
                {
                    void* srv_fallback = texture_it->GetRhiSrv();
                }

                if (!descriptor.as_array)
                {
                    image_index++;

                    // get texture, if unable to do so, fallback to a checkerboard texture, so we can spot it by eye
                    void* resource = texture ? (mip_specified ? texture->GetSrvMip(descriptor.mip) : texture->GetSrv()) : nullptr;
                    ImageLayout layout = texture ? texture->GetLayout(mip_start) : ImageLayout::Max;
                    if (descriptor.type == DescriptorType::Image && descriptor.data == nullptr)
                    {
                        resource = srv_fallback;
                        layout   = ImageLayout::Shader_Read;
                    }

                    info_images[image_index].sampler     = nullptr;
                    info_images[image_index].imageView   = static_cast<VkImageView>(resource);
                    info_images[image_index].imageLayout = vulkan_image_layout[static_cast<uint8_t>(layout)];

                    descriptor_index_start = image_index;
                }
                else // bind mips as an array of textures (not a Texture2DArray)
                {
                    for (uint32_t mip_index = mip_start; mip_index < mip_start + descriptor.mip_range; mip_index++)
                    {
                        image_index++;

                        // get texture, if unable to do so, fallback to a checkerboard texture, so we can spot it by eye
                        void* resource = texture ? texture->GetSrvMip(mip_index) : nullptr;
                        ImageLayout layout = texture ? texture->GetLayout(mip_index) : ImageLayout::Max;
                        if (descriptor.type == DescriptorType::Image && descriptor.data == nullptr)
                        {
                            resource = srv_fallback;
                            layout   = ImageLayout::Shader_Read;
                        }

                        info_images[image_index].sampler     = nullptr;
                        info_images[image_index].imageView   = static_cast<VkImageView>(resource);
                        info_images[image_index].imageLayout = vulkan_image_layout[static_cast<uint8_t>(layout)];

                        if (mip_index == descriptor.mip)
                        {
                            descriptor_index_start = image_index;
                        }
                    }

                    descriptor_cnt = descriptor.mip_range != 0 ? descriptor.mip_range : descriptor_cnt;
                }
            }
            else if (descriptor.type == DescriptorType::AccelerationStructure)
            {
                AccelerationStructure* tlas = static_cast<AccelerationStructure*>(descriptor.data);

                if (!tlas || !tlas->GetResource())
                {
                    SEDX_CORE_WARNING("Acceleration structure is null or invalid, skipping descriptor update");
                    continue;
                }

                accel_index++;
                // Store handle in persistent vector so pointer remains valid
                accel_struct_handles.push_back(static_cast<VkAccelerationStructureKHR>(tlas->GetResource()));
                info_accel_structs[accel_index].sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
                info_accel_structs[accel_index].accelerationStructureCount = 1;
                info_accel_structs[accel_index].pAccelerationStructures    = &accel_struct_handles.back();
                descriptor_index_start                                     = accel_index;
                descriptor_cnt                                             = 1;
            }
            else if (descriptor.type == DescriptorType::ConstantBuffer || descriptor.type == DescriptorType::StructuredBuffer)
            {
                info_buffers[index].buffer = static_cast<VkBuffer>(static_cast<Buffer*>(descriptor.data)->GetRhiResource());
                info_buffers[index].offset = 0;
                info_buffers[index].range  = descriptor.range;

                descriptor_index_start = index;
            }
            else
            {
                SEDX_ASSERT_MSG(false, "Unhandled descriptor type");
            }

            // Write descriptor set
            descriptor_sets[index].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_sets[index].pNext            = (descriptor.type == DescriptorType::AccelerationStructure) ? &info_accel_structs[descriptor_index_start] : nullptr;
            descriptor_sets[index].dstSet           = static_cast<VkDescriptorSet>(m_resource);
            descriptor_sets[index].dstBinding       = descriptor.slot;
            descriptor_sets[index].dstArrayElement  = 0; // starting element in that array
            descriptor_sets[index].descriptorCount  = descriptor_cnt;
            descriptor_sets[index].descriptorType   = static_cast<VkDescriptorType>(VulkanDevice::GetDescriptorType(descriptor));
            descriptor_sets[index].pImageInfo       = &info_images[descriptor_index_start];
            descriptor_sets[index].pBufferInfo      = &info_buffers[descriptor_index_start];
            descriptor_sets[index].pTexelBufferView = nullptr;

            SEDX_ASSERT(descriptor_sets[index].dstSet != nullptr);

            index++;
        }

        vkUpdateDescriptorSets(
            device,    				// device
            index,                  // descriptorWriteCount
            descriptor_sets.data(), // pDescriptorWrites
            0,                      // descriptorCopyCount
            nullptr                 // pDescriptorCopies
        );
    }
}

// -------------------------------------------------------

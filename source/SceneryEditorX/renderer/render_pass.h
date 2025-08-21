/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* render_pass.h
* -------------------------------------------------------
* Created: 18/8/2025
* -------------------------------------------------------
*/
#pragma once
#include "texture.h"
#include "buffers/storage_buffer.h"
#include "buffers/uniform_buffer.h"
#include "vulkan/vk_enums.h"
#include "vulkan/vk_image_view.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	
    struct RenderPassInput
    {
        ResourceType type = ResourceType::None;
        std::vector<Ref<RefCounted>> input;

        RenderPassInput() = default;
    	RenderPassInput(const Ref<UniformBuffer>& uniformBuffer) : type(ResourceType::UniformBuffer) { input.reserve(1); input.push_back(uniformBuffer.As<RefCounted>()); }
    	RenderPassInput(const Ref<UniformBufferSet>& uniformBufferSet) : type(ResourceType::UniformSet) { input.reserve(1); input.push_back(uniformBufferSet.As<RefCounted>()); }
    	RenderPassInput(const Ref<StorageBuffer>& storageBuffer) : type(ResourceType::StorageBuffer) { input.reserve(1); input.push_back(storageBuffer.As<RefCounted>()); }
    	RenderPassInput(const Ref<StorageBufferSet>& storageBufferSet) : type(ResourceType::StorageSet) { input.reserve(1); input.push_back(storageBufferSet.As<RefCounted>()); }
    	RenderPassInput(const Ref<Texture2D>& texture) : type(ResourceType::Texture2D) { input.reserve(1); input.push_back(texture.As<RefCounted>()); }
    	RenderPassInput(const Ref<TextureCube>& texture) : type(ResourceType::TextureCube) { input.reserve(1); input.push_back(texture.As<RefCounted>()); }
    	RenderPassInput(const Ref<Image2D>& image) : type(ResourceType::Image2D) { input.reserve(1); input.push_back(image.As<RefCounted>()); }

    	void Set(const Ref<UniformBuffer>& uniformBuffer, uint32_t index = 0) { type = ResourceType::UniformBuffer; if (input.size() <= index) input.resize(index+1); input[index] = uniformBuffer.As<RefCounted>(); }
    	void Set(const Ref<UniformBufferSet>& uniformBufferSet, uint32_t index = 0) { type = ResourceType::UniformSet; if (input.size() <= index) input.resize(index+1); input[index] = uniformBufferSet.As<RefCounted>(); }
    	void Set(const Ref<StorageBuffer>& storageBuffer, uint32_t index = 0) { type = ResourceType::StorageBuffer; if (input.size() <= index) input.resize(index+1); input[index] = storageBuffer.As<RefCounted>(); }
    	void Set(const Ref<StorageBufferSet>& storageBufferSet, uint32_t index = 0) { type = ResourceType::StorageSet; if (input.size() <= index) input.resize(index+1); input[index] = storageBufferSet.As<RefCounted>(); }
    	void Set(const Ref<Texture2D>& texture, uint32_t index = 0) { type = ResourceType::Texture2D; if (input.size() <= index) input.resize(index+1); input[index] = texture.As<RefCounted>(); }
    	void Set(const Ref<TextureCube>& texture, uint32_t index = 0) { type = ResourceType::TextureCube; if (input.size() <= index) input.resize(index+1); input[index] = texture.As<RefCounted>(); }
    	void Set(const Ref<Image2D>& image, uint32_t index = 0) { type = ResourceType::Image2D; if (input.size() <= index) input.resize(index+1); input[index] = image.As<RefCounted>(); }
    	void Set(const Ref<ImageView>& image, uint32_t index = 0) { type = ResourceType::Image2D; if (input.size() <= index) input.resize(index+1); input[index] = image.As<RefCounted>(); }
    };

    struct RenderPassInputDeclaration
    {
        ResourceInputType type = ResourceInputType::None;
        uint32_t set = 0;
        uint32_t binding = 0;
        uint32_t count = 0;
        std::string name;
    };


}

/// -------------------------------------------------------

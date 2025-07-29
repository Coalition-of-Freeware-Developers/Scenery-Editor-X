/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_descriptor_set_manager.cpp
* -------------------------------------------------------
* Created: 26/7/2025
* -------------------------------------------------------
*/
//#include <SceneryEditorX/core/time/timer.h>
//#include <SceneryEditorX/logging/profiler.hpp>
//#include <SceneryEditorX/renderer/vulkan/vk_descriptor_set_manager.h>
//#include <SceneryEditorX/renderer/vulkan/vk_util.h>
//#include <SceneryEditorX/renderer/shaders/shader_resource.h>
//#include <SceneryEditorX/renderer/vulkan/vk_sampler.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	
	/*
	namespace Utils
    {

		inline ResourceType GetDefaultResourceType(VkDescriptorType descriptorType)
		{
			switch (descriptorType)
			{
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    return ResourceType::Texture2D;
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    return ResourceType::Image2D;
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    return ResourceType::UniformBuffer;
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    return ResourceType::StorageBuffer;
			}

			SEDX_CORE_ASSERT(false);
			return ResourceType::None;
		}

	}

	DescriptorSetManager::DescriptorSetManager(const DescriptorSetManagerSpecification& specification) : m_Specification(specification)
	{
		Init();
	}

	DescriptorSetManager::DescriptorSetManager(const DescriptorSetManager& other) : m_Specification(other.m_Specification)
	{
		Init();
		inputResources = other.inputResources;
		Bake();
	}

	DescriptorSetManager DescriptorSetManager::Copy(const DescriptorSetManager& other)
	{
		DescriptorSetManager result(other);
		return result;
	}

	void DescriptorSetManager::Init()
	{
		const auto& shaderDescriptorSets = m_Specification.shader->GetShaderDescriptorSets();
		uint32_t framesInFlight = Renderer::GetRenderData().framesInFlight;
		writeDescriptorMap.resize(framesInFlight);

		for (uint32_t set = m_Specification.startSet; set <= m_Specification.endSet; set++)
		{
			if (set >= shaderDescriptorSets.size())
				break;

            for (const auto& shaderDescriptor = shaderDescriptorSets[set]; auto&& [bname, wd] : shaderDescriptor.writeDescriptorSets)
			{
				/// HACK: This is a hack to fix a bad input decl name Coming from somewhere.
				const char* broken = strrchr(bname.c_str(), '.');
				std::string name = broken ? broken + 1 : bname;

				uint32_t binding = wd.dstBinding;
				RenderPassInputDeclaration& inputDecl = inputDeclarations[name];
				inputDecl.type = RenderPassInputTypeFromVulkanDescriptorType(wd.descriptorType);
				inputDecl.set = set;
				inputDecl.binding = binding;
				inputDecl.name = name;
				inputDecl.count = wd.descriptorCount;

				/// Insert default resources (useful for materials)
				if (m_Specification.DefaultResources || true)
				{
					/// Create RenderPassInput
					RenderPassInput& input = inputResources[set][binding];
					input.input.resize(wd.descriptorCount);
					input.type = Utils::GetDefaultResourceType(wd.descriptorType);

					/// Set default textures
                    if (inputDecl.type == ResourceInputType::ImageSampler2D)
					{
						for (auto &i : input.input)
                            i = Renderer::GetWhiteTexture();
                    }
                    else if (inputDecl.type == ResourceInputType::ImageSampler3D)
					{
						for (auto &i : input.input)
                            i = Renderer::GetBlackCubeTexture();
                    }
				}

				for (uint32_t frameIndex = 0; frameIndex < framesInFlight; frameIndex++)
					writeDescriptorMap[frameIndex][set][binding] = {
				    .writeDescriptorSet = wd,
				    .resourceHandles = std::vector<void*>(wd.descriptorCount)
				};

				if (shaderDescriptor.imageSamplers.contains(binding))
				{
					auto& imageSampler = shaderDescriptor.imageSamplers.at(binding);
					uint32_t dimension = imageSampler.Dimension;
					if (wd.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || wd.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
					{
						switch (dimension)
						{
							case 1: inputDecl.type = ResourceInputType::ImageSampler1D;
								break;
							case 2: inputDecl.type = ResourceInputType::ImageSampler2D;
								break;
							case 3: inputDecl.type = ResourceInputType::ImageSampler3D;
								break;
						}
					}
					else if (wd.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
					{
						switch (dimension)
						{
							case 1: inputDecl.type = ResourceInputType::StorageImage1D;
								break;
							case 2: inputDecl.type = ResourceInputType::StorageImage2D;
								break;
							case 3: inputDecl.type = ResourceInputType::StorageImage3D;
								break;
						}

					}
				}
			}
		}
	}

	void DescriptorSetManager::AddInput(std::string_view name, const Ref<UniformBufferSet> &uniformBufferSet)
	{
        if (const RenderPassInputDeclaration* decl = GetInputDeclaration(name))
			inputResources.at(decl->set).at(decl->binding).Set(uniformBufferSet);
		else
			SEDX_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.debugName, name);
	}

	void DescriptorSetManager::AddInput(std::string_view name, const Ref<UniformBuffer> &uniformBuffer)
	{
        if (const RenderPassInputDeclaration* decl = GetInputDeclaration(name))
			inputResources.at(decl->set).at(decl->binding).Set(uniformBuffer);
		else
			SEDX_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.debugName, name);
	}

	void DescriptorSetManager::AddInput(std::string_view name, const Ref<StorageBufferSet> &storageBufferSet)
	{
        if (const RenderPassInputDeclaration* decl = GetInputDeclaration(name))
			inputResources.at(decl->set).at(decl->binding).Set(storageBufferSet);
		else
			SEDX_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.debugName, name);
	}

	void DescriptorSetManager::AddInput(std::string_view name, const Ref<StorageBuffer> &storageBuffer)
	{
        if (const RenderPassInputDeclaration* decl = GetInputDeclaration(name))
			inputResources.at(decl->set).at(decl->binding).Set(storageBuffer);
		else
			SEDX_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.debugName, name);
	}

	void DescriptorSetManager::AddInput(std::string_view name, const Ref<Texture2D> &texture, uint32_t index)
	{
		const RenderPassInputDeclaration* decl = GetInputDeclaration(name);
		SEDX_CORE_VERIFY(index < decl->count);
		if (decl)
			inputResources.at(decl->set).at(decl->binding).Set(texture, index);
		else
			SEDX_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.debugName, name);
	}

	void DescriptorSetManager::AddInput(std::string_view name, const Ref<TextureCube> &textureCube)
	{
        if (const RenderPassInputDeclaration* decl = GetInputDeclaration(name))
			inputResources.at(decl->set).at(decl->binding).Set(textureCube);
		else
			SEDX_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.debugName, name);
	}

	void DescriptorSetManager::AddInput(std::string_view name, const Ref<Image2D> &image)
	{
        if (const RenderPassInputDeclaration* decl = GetInputDeclaration(name))
			inputResources.at(decl->set).at(decl->binding).Set(image);
		else
			SEDX_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.debugName, name);
	}

	void DescriptorSetManager::AddInput(std::string_view name, const Ref<ImageView> &image)
	{
        if (const RenderPassInputDeclaration* decl = GetInputDeclaration(name))
			inputResources.at(decl->set).at(decl->binding).Set(image);
		else
			SEDX_CORE_WARN_TAG("Renderer", "[RenderPass ({})] Input {} not found", m_Specification.debugName, name);
	}

	bool DescriptorSetManager::IsInvalidated(uint32_t set, uint32_t binding) const
	{
		if (invalidatedInputResources.contains(set))
		{
			const auto& resources = invalidatedInputResources.at(set);
			return resources.contains(binding);
		}

		return false;
	}

	

	std::set<uint32_t> DescriptorSetManager::HasBufferSets() const
	{
		/// Find all descriptor sets that have either UniformBufferSet or StorageBufferSet descriptors
		std::set<uint32_t> sets;

		for (const auto& [set, resources] : inputResources)
		{
			for (const auto &input : resources | std::views::values)
			{
				if (input.type == ResourceType::UniformBufferSet || input.type == ResourceType::StorageBufferSet)
				{
					sets.insert(set);
					break;
				}
			}
		}
		return sets;
	}


	bool DescriptorSetManager::Validate()
	{
		/// Go through pipeline requirements to make sure we have all required resource
		const auto& shaderDescriptorSets = m_Specification.shader->GetShaderDescriptorSets();

		/// Nothing to validate, pipeline only contains material inputs
		///if (shaderDescriptorSets.size() < 2)
		///	return true;

		for (uint32_t set = m_Specification.startSet; set <= m_Specification.endSet; set++)
		{
			if (set >= shaderDescriptorSets.size())
				break;

			/// No descriptors in this set
			if (!shaderDescriptorSets[set])
				continue;

			if (!inputResources.contains(set))
			{
				SEDX_CORE_ERROR_TAG("Renderer", "[RenderPass ({})] No input resources for Set {}", m_Specification.debugName, set);
				return false;
			}

			const auto& setInputResources = inputResources.at(set);

            for (const auto& shaderDescriptor = shaderDescriptorSets[set]; auto&& [name, wd] : shaderDescriptor.writeDescriptorSets)
			{
				uint32_t binding = wd.dstBinding;
				if (!setInputResources.contains(binding))
				{
					SEDX_CORE_ERROR_TAG("Renderer", "[RenderPass ({})] No input resource for {}.{}", m_Specification.debugName, set, binding);
					SEDX_CORE_ERROR_TAG("Renderer", "[RenderPass ({})] Required resource is {} ({})", m_Specification.debugName, name, (int)wd.descriptorType);
					return false;
				}

				const auto& resource = setInputResources.at(binding);
				if (!IsCompatibleInput(resource.type, wd.descriptorType))
				{
					SEDX_CORE_ERROR_TAG("Renderer", "[RenderPass ({})] Required resource is wrong type! {} but needs {}", m_Specification.debugName, (uint16_t)resource.type, (int)wd.descriptorType);
					return false;
				}

				if (resource.type != ResourceType::Image2D && resource.input[0] == nullptr)
				{
					SEDX_CORE_ERROR_TAG("Renderer", "[RenderPass ({})] Resource is null! {} ({}.{})", m_Specification.debugName, name, set, binding);
					return false;
				}
			}
		}

		/// All resources present
		return true;
	}

	void DescriptorSetManager::Bake()
	{
		/// Make sure all resources are present and we can properly bake
		if (!Validate())
		{
			SEDX_CORE_ERROR_TAG("Renderer", "[RenderPass] Bake - Validate failed! {}", m_Specification.debugName);
			return;
		}
		
		/// If valid, we can create descriptor sets

		/// Create Descriptor Pool
		VkDescriptorPoolSize poolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 10 * 3; /// frames in flight should partially determine this
		poolInfo.poolSizeCount = 10;
		poolInfo.pPoolSizes = poolSizes;

		const VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool))

		auto bufferSets = HasBufferSets();
		bool perFrameInFlight = !bufferSets.empty();
		perFrameInFlight = true; // always
		uint32_t descriptorSetCount = Renderer::GetRenderData().framesInFlight;
		if (!perFrameInFlight)
			descriptorSetCount = 1;

		if (m_DescriptorSets.empty())
		{
			for (uint32_t i = 0; i < descriptorSetCount; i++)
				m_DescriptorSets.emplace_back();
		}

		for (auto& descriptorSet : m_DescriptorSets)
			descriptorSet.clear();

		for (const auto& [set, setData] : inputResources)
		{
			uint32_t descriptorCountInSet = bufferSets.contains(set) ? descriptorSetCount : 1;
			for (uint32_t frameIndex = 0; frameIndex < descriptorSetCount; frameIndex++)
			{
				VkDescriptorSetLayout dsl = m_Specification.shader->GetDescriptorSetLayout(set);
				VkDescriptorSetAllocateInfo descriptorSetAllocInfo = DescriptorSetAllocInfo(&dsl);
				descriptorSetAllocInfo.descriptorPool = m_DescriptorPool;
				VkDescriptorSet descriptorSet = nullptr;
				VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSet));

				m_DescriptorSets[frameIndex].emplace_back(descriptorSet);

				auto& currentWriteDescriptorMap = writeDescriptorMap[frameIndex].at(set);
				std::vector<std::vector<VkDescriptorImageInfo>> imageInfoStorage;
				uint32_t imageInfoStorageIndex = 0;

				for (const auto& [binding, input] : setData)
				{
					auto& storedWriteDescriptor = currentWriteDescriptorMap.at(binding);

					VkWriteDescriptorSet& writeDescriptor = storedWriteDescriptor.writeDescriptorSet;
					writeDescriptor.dstSet = descriptorSet;

					switch (input.type)
					{
						case ResourceType::UniformBuffer:
						{
							Ref<UniformBuffer> buffer = input.input[0].As<UniformBuffer>();
							writeDescriptor.pBufferInfo = &buffer->GetDescriptorBufferInfo();
							storedWriteDescriptor.resourceHandles[0] = writeDescriptor.pBufferInfo->buffer;

							// Defer if resource doesn't exist
							if (writeDescriptor.pBufferInfo->buffer == nullptr)
								invalidatedInputResources[set][binding] = input;

							break;
						}
						case ResourceType::UniformBufferSet:
						{
							Ref<UniformBufferSet> buffer = input.input[0].As<UniformBufferSet>();
							// TODO: replace 0 with current frame in flight (i.e. create bindings for all frames)
							writeDescriptor.pBufferInfo = &buffer->Get(frameIndex).As<UniformBuffer>()->GetDescriptorBufferInfo();
							storedWriteDescriptor.resourceHandles[0] = writeDescriptor.pBufferInfo->buffer;

							// Defer if resource doesn't exist
							if (writeDescriptor.pBufferInfo->buffer == nullptr)
								invalidatedInputResources[set][binding] = input;

							break;
						}
						case ResourceType::StorageBuffer:
						{
							Ref<StorageBuffer> buffer = input.input[0].As<StorageBuffer>();
							writeDescriptor.pBufferInfo = &buffer->GetDescriptorBufferInfo();
							storedWriteDescriptor.resourceHandles[0] = writeDescriptor.pBufferInfo->buffer;

							// Defer if resource doesn't exist
							if (writeDescriptor.pBufferInfo->buffer == nullptr)
								invalidatedInputResources[set][binding] = input;

							break;
						}
						case ResourceType::StorageBufferSet:
						{
							Ref<StorageBufferSet> buffer = input.input[0].As<StorageBufferSet>();
							// TODO: replace 0 with current frame in flight (i.e. create bindings for all frames)
							writeDescriptor.pBufferInfo = &buffer->Get(frameIndex).As<StorageBuffer>()->GetDescriptorBufferInfo();
							storedWriteDescriptor.resourceHandles[0] = writeDescriptor.pBufferInfo->buffer;

							// Defer if resource doesn't exist
							if (writeDescriptor.pBufferInfo->buffer == nullptr)
								invalidatedInputResources[set][binding] = input;

							break;
						}
						case ResourceType::Texture2D:
						{
							if (input.input.size() > 1)
							{
								imageInfoStorage.emplace_back(input.input.size());
								for (size_t i = 0; i < input.input.size(); i++)
								{
									Ref<Texture2D> texture = input.input[i].As<Texture2D>();
									imageInfoStorage[imageInfoStorageIndex][i] = texture->GetDescriptorInfoVulkan();

								}
								writeDescriptor.pImageInfo = imageInfoStorage[imageInfoStorageIndex].data();
								imageInfoStorageIndex++;
							}
							else
							{
								Ref<Texture2D> texture = input.input[0].As<Texture2D>();
								writeDescriptor.pImageInfo = &texture->GetDescriptorInfoVulkan();
							}
							storedWriteDescriptor.resourceHandles[0] = writeDescriptor.pImageInfo->imageView;

							// Defer if resource doesn't exist
							if (writeDescriptor.pImageInfo->imageView == nullptr)
								invalidatedInputResources[set][binding] = input;

							break;
						}
						case ResourceType::TextureCube:
						{
							Ref<TextureCube> texture = input.input[0].As<TextureCube>();
							writeDescriptor.pImageInfo = &texture->GetDescriptorInfoVulkan();
							storedWriteDescriptor.resourceHandles[0] = writeDescriptor.pImageInfo->imageView;

							// Defer if resource doesn't exist
							if (writeDescriptor.pImageInfo->imageView == nullptr)
								invalidatedInputResources[set][binding] = input;

							break;
						}
						case ResourceType::Image2D:
						{
							Ref<Resource> image = input.input[0].As<Resource>();
							// Defer if resource doesn't exist
							if (image == nullptr)
							{
								invalidatedInputResources[set][binding] = input;
								break;
							}

							writeDescriptor.pImageInfo = (VkDescriptorImageInfo*)image->GetDescriptorInfo();
							storedWriteDescriptor.resourceHandles[0] = writeDescriptor.pImageInfo->imageView;

							// Defer if resource doesn't exist
							if (writeDescriptor.pImageInfo->imageView == nullptr)
								invalidatedInputResources[set][binding] = input;

							break;
						}
					}
				}

				std::vector<VkWriteDescriptorSet> writeDescriptors;
				for (auto&& [binding, writeDescriptor] : currentWriteDescriptorMap)
				{
					// Include if valid, otherwise defer (these will be resolved if possible at Prepare stage)
					if (!IsInvalidated(set, binding))
						writeDescriptors.emplace_back(writeDescriptor.writeDescriptorSet);
				}

				if (!writeDescriptors.empty())
				{
					SEDX_CORE_INFO_TAG("Renderer", "Render pass update {} descriptors in set {}", writeDescriptors.size(), set);
					vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
				}
			}
		}

	}

	void DescriptorSetManager::InvalidateAndUpdate()
	{
		SEDX_PROFILE_FUNC();
		//SEDX_SCOPE_PERF("DescriptorSetManager::InvalidateAndUpdate")

		uint32_t currentFrameIndex = Renderer::GetCurrentRenderThreadFrameIndex();

		///< Check for invalidated resources
		for (const auto& [set, inputs] : inputResources)
		{
			for (const auto& [binding, input] : inputs)
			{
				switch (input.type)
				{
					case ResourceType::UniformBuffer:
					{
						//for (uint32_t frameIndex = 0; frameIndex < (uint32_t)writeDescriptorMap.size(); frameIndex++)
						{
							const VkDescriptorBufferInfo& bufferInfo = input.input[0].As<UniformBuffer>()->GetDescriptorBufferInfo();
							if (bufferInfo.buffer != writeDescriptorMap[currentFrameIndex].at(set).at(binding).resourceHandles[0])
							{
								invalidatedInputResources[set][binding] = input;
								break;
							}
						}
						break;
					}
					case ResourceType::UniformBufferSet:
					{
						//for (uint32_t frameIndex = 0; frameIndex < (uint32_t)writeDescriptorMap.size(); frameIndex++)
						{
							const VkDescriptorBufferInfo& bufferInfo = input.input[0].As<UniformBufferSet>()->Get(currentFrameIndex).As<UniformBuffer>()->GetDescriptorBufferInfo();
							if (bufferInfo.buffer != writeDescriptorMap[currentFrameIndex].at(set).at(binding).resourceHandles[0])
							{
								invalidatedInputResources[set][binding] = input;
								break;
							}
						}
						break;
					}
					case ResourceType::StorageBuffer:
					{

						//for (uint32_t frameIndex = 0; frameIndex < (uint32_t)writeDescriptorMap.size(); frameIndex++)
						{
							const VkDescriptorBufferInfo& bufferInfo = input.input[0].As<StorageBuffer>()->GetDescriptorBufferInfo();
							if (bufferInfo.buffer != writeDescriptorMap[currentFrameIndex].at(set).at(binding).resourceHandles[0])
							{
								invalidatedInputResources[set][binding] = input;
								break;
							}
						}
						break;
					}
					case ResourceType::StorageBufferSet:
					{
						//for (uint32_t frameIndex = 0; frameIndex < (uint32_t)writeDescriptorMap.size(); frameIndex++)
						{
							const VkDescriptorBufferInfo& bufferInfo = input.input[0].As<StorageBufferSet>()->Get(currentFrameIndex).As<StorageBuffer>()->GetDescriptorBufferInfo();
							if (bufferInfo.buffer != writeDescriptorMap[currentFrameIndex].at(set).at(binding).resourceHandles[0])
							{
								invalidatedInputResources[set][binding] = input;
								break;
							}
						}
						break;
					}
					case ResourceType::Texture2D:
					{
						for (size_t i = 0; i < input.input.size(); i++)
						{
							Ref<Texture2D> vulkanTexture = input.input[i].As<Texture2D>();
							if (vulkanTexture == nullptr)
								vulkanTexture = Renderer::GetWhiteTexture().As<Texture2D>(); // TODO(Yan): error texture

							const VkDescriptorImageInfo& imageInfo = vulkanTexture->GetDescriptorInfoVulkan();
							if (imageInfo.imageView != writeDescriptorMap[currentFrameIndex].at(set).at(binding).resourceHandles[i])
							{
								invalidatedInputResources[set][binding] = input;
								break;
							}
						}
						break;
					}
					case ResourceType::TextureCube:
					{
						//for (uint32_t frameIndex = 0; frameIndex < (uint32_t)writeDescriptorMap.size(); frameIndex++)
						{
							const VkDescriptorImageInfo& imageInfo = input.input[0].As<TextureCube>()->GetDescriptorInfoVulkan();
							if (imageInfo.imageView != writeDescriptorMap[currentFrameIndex].at(set).at(binding).resourceHandles[0])
							{
								invalidatedInputResources[set][binding] = input;
								break;
							}
						}
						break;
					}
					case ResourceType::Image2D:
					{
						//for (uint32_t frameIndex = 0; frameIndex < (uint32_t)writeDescriptorMap.size(); frameIndex++)
						{
							const VkDescriptorImageInfo& imageInfo = *(VkDescriptorImageInfo*)input.input[0].As<Resource>()->GetDescriptorInfo();
							if (imageInfo.imageView != writeDescriptorMap[currentFrameIndex].at(set).at(binding).resourceHandles[0])
							{
								invalidatedInputResources[set][binding] = input;
								break;
							}
						}
						break;
					}
				}
			}
		}

		/// Nothing to do
		if (invalidatedInputResources.empty())
			return;

		auto bufferSets = HasBufferSets();
		bool perFrameInFlight = !bufferSets.empty();
		perFrameInFlight = true; /// always
		uint32_t descriptorSetCount = Renderer::GetRenderData().framesInFlight;
		if (!perFrameInFlight)
			descriptorSetCount = 1;


		// TODO: handle these if they fail (although Vulkan will probably give us a validation error if they do anyway)
		for (const auto& [set, setData] : invalidatedInputResources)
		{
			uint32_t descriptorCountInSet = bufferSets.contains(set) ? descriptorSetCount : 1;
			//for (uint32_t frameIndex = currentFrameIndex; frameIndex < descriptorSetCount; frameIndex++)
            {
                uint32_t frameIndex = perFrameInFlight ? currentFrameIndex : 0;
                // Go through every resource here and call vkUpdateDescriptorSets with write descriptors
				// If we don't have valid buffers/images to bind to here, that's an error and needs to be
				// probably handled by putting in some error resources, otherwise we'll crash
				std::vector<VkWriteDescriptorSet> writeDescriptorsToUpdate;
				writeDescriptorsToUpdate.reserve(setData.size());
				std::vector<std::vector<VkDescriptorImageInfo>> imageInfoStorage;
				uint32_t imageInfoStorageIndex = 0;
				for (const auto& [binding, input] : setData)
				{
					// Update stored write descriptor
					auto& wd = writeDescriptorMap[frameIndex].at(set).at(binding);
					VkWriteDescriptorSet& writeDescriptor = wd.writeDescriptorSet;
					switch (input.type)
					{
						case ResourceType::UniformBuffer:
						{
							Ref<UniformBuffer> buffer = input.input[0].As<UniformBuffer>();
							writeDescriptor.pBufferInfo = &buffer->GetDescriptorBufferInfo();
							wd.resourceHandles[0] = writeDescriptor.pBufferInfo->buffer;
							break;
						}
						case ResourceType::UniformBufferSet:
						{
							Ref<UniformBufferSet> buffer = input.input[0].As<UniformBufferSet>();
							writeDescriptor.pBufferInfo = &buffer->Get(frameIndex).As<UniformBuffer>()->GetDescriptorBufferInfo();
							wd.resourceHandles[0] = writeDescriptor.pBufferInfo->buffer;
							break;
						}
						case ResourceType::StorageBuffer:
						{
							Ref<StorageBuffer> buffer = input.input[0].As<StorageBuffer>();
							writeDescriptor.pBufferInfo = &buffer->GetDescriptorBufferInfo();
							wd.resourceHandles[0] = writeDescriptor.pBufferInfo->buffer;
							break;
						}
						case ResourceType::StorageBufferSet:
						{
							Ref<StorageBufferSet> buffer = input.input[0].As<StorageBufferSet>();
							writeDescriptor.pBufferInfo = &buffer->Get(frameIndex).As<StorageBuffer>()->GetDescriptorBufferInfo();
							wd.resourceHandles[0] = writeDescriptor.pBufferInfo->buffer;
							break;
						}
						case ResourceType::Texture2D:
						{

							if (input.input.size() > 1)
							{
								imageInfoStorage.emplace_back(input.input.size());
								for (size_t i = 0; i < input.input.size(); i++)
								{
									Ref<Texture2D> texture = input.input[i].As<Texture2D>();
									imageInfoStorage[imageInfoStorageIndex][i] = texture->GetDescriptorInfoVulkan();
									wd.resourceHandles[i] = imageInfoStorage[imageInfoStorageIndex][i].imageView;
								}
								writeDescriptor.pImageInfo = imageInfoStorage[imageInfoStorageIndex].data();
								imageInfoStorageIndex++;
							}
							else
							{
								Ref<Texture2D> texture = input.input[0].As<Texture2D>();
								writeDescriptor.pImageInfo = &texture->GetDescriptorInfoVulkan();
								wd.resourceHandles[0] = writeDescriptor.pImageInfo->imageView;
							}

							break;
						}
						case ResourceType::TextureCube:
						{
							Ref<TextureCube> texture = input.input[0].As<TextureCube>();
							writeDescriptor.pImageInfo = &texture->GetDescriptorInfoVulkan();
							wd.resourceHandles[0] = writeDescriptor.pImageInfo->imageView;
							break;
						}
						case ResourceType::Image2D:
						{
							Ref<Resource> image = input.input[0].As<Resource>();
							writeDescriptor.pImageInfo = (VkDescriptorImageInfo*)image->GetDescriptorInfo();
							SEDX_CORE_VERIFY(writeDescriptor.pImageInfo->imageView);
							wd.resourceHandles[0] = writeDescriptor.pImageInfo->imageView;
							break;
						}
					}
					writeDescriptorsToUpdate.emplace_back(writeDescriptor);
				}
				// SEDX_CORE_INFO_TAG("Renderer", "RenderPass::Prepare ({}) - updating {} descriptors in set {} (frameIndex={})", m_Specification.DebugName, writeDescriptorsToUpdate.size(), set, frameIndex);
				SEDX_CORE_INFO_TAG("Renderer", "DescriptorSetManager::InvalidateAndUpdate ({}) - updating {} descriptors in set {} (frameIndex={})", m_Specification.debugName, writeDescriptorsToUpdate.size(), set, frameIndex);
				const VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();
				vkUpdateDescriptorSets(device, (uint32_t)writeDescriptorsToUpdate.size(), writeDescriptorsToUpdate.data(), 0, nullptr);
			}
		}

		invalidatedInputResources.clear();
	}

	bool DescriptorSetManager::HasDescriptorSets() const
	{
		return !m_DescriptorSets.empty() && !m_DescriptorSets[0].empty();
	}

	uint32_t DescriptorSetManager::GetFirstSetIndex() const
	{
		if (inputResources.empty())
			return UINT32_MAX;

		/// Return first key (key == descriptor set index)
		return inputResources.begin()->first;
	}

	const std::vector<VkDescriptorSet>& DescriptorSetManager::GetDescriptorSets(uint32_t frameIndex) const
	{
		SEDX_CORE_ASSERT(!m_DescriptorSets.empty());

		if (frameIndex > 0 && m_DescriptorSets.size() == 1)
			return m_DescriptorSets[0]; /// Frame index is irrelevant for this type of render pass

		return m_DescriptorSets[frameIndex];
	}

	bool DescriptorSetManager::IsInputValid(std::string_view name) const
	{
		std::string nameStr(name);
		return inputDeclarations.contains(nameStr);
	}

	const RenderPassInputDeclaration* DescriptorSetManager::GetInputDeclaration(std::string_view name) const
	{
		std::string nameStr(name);
		if (!inputDeclarations.contains(nameStr))
			return nullptr;

		const RenderPassInputDeclaration& decl = inputDeclarations.at(nameStr);
		return &decl;
	}
	*/

}

/// -------------------------------------------------------

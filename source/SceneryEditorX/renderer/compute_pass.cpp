/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* compute_pass.cpp
* -------------------------------------------------------
* Created: 24/7/2025
* -------------------------------------------------------
*/
#include "compute_pass.h"
#include "buffers/storage_buffer.h"
#include "buffers/uniform_buffer.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

	ComputePass::ComputePass(const ComputePassSpecification& spec) : m_Specification(spec)
	{
		SEDX_CORE_VERIFY(spec.pipeline);

		DescriptorSetManagerSpecification dmSpec;
		dmSpec.debugName = spec.debugName;
		dmSpec.shader = spec.pipeline->GetShader().As<Shader>();
		dmSpec.startSet = 1;
		m_DescriptorSetManager = DescriptorSetManager(dmSpec);
	}

	ComputePass::~ComputePass()
	{
	}

	bool ComputePass::IsInvalidated(uint32_t set, uint32_t binding) const
	{
		return m_DescriptorSetManager.IsInvalidated(set, binding);
	}

	void ComputePass::AddInput(std::string_view name, Ref<UniformBufferSet> uniformBufferSet)
	{
		m_DescriptorSetManager.AddInput(name, uniformBufferSet);
	}

	void ComputePass::AddInput(std::string_view name, Ref<UniformBuffer> uniformBuffer)
	{
		m_DescriptorSetManager.AddInput(name, uniformBuffer);
	}

    void ComputePass::AddInput(std::string_view name, Ref<StorageBufferSet> storageBufferSet)
    {
        m_DescriptorSetManager.AddInput(name, storageBufferSet);
    }

	void ComputePass::AddInput(std::string_view name, Ref<StorageBuffer> storageBuffer)
	{
		m_DescriptorSetManager.AddInput(name, storageBuffer);
	}

	void ComputePass::AddInput(std::string_view name, Ref<Texture2D> texture)
	{
		m_DescriptorSetManager.AddInput(name, texture);
	}

	void ComputePass::AddInput(std::string_view name, Ref<TextureCube> textureCube)
	{
		m_DescriptorSetManager.AddInput(name, textureCube);
	}

	void ComputePass::AddInput(std::string_view name, Ref<Image2D> image)
	{
		m_DescriptorSetManager.AddInput(name, image);
	}

	Ref<Image2D> ComputePass::GetOutput(uint32_t index)
	{
		SEDX_CORE_VERIFY(false, "Not implemented");
		return nullptr;
	}

	Ref<Image2D> ComputePass::GetDepthOutput()
	{
		SEDX_CORE_VERIFY(false, "Not implemented");
		return nullptr;
	}

	bool ComputePass::HasDescriptorSets() const
	{
		return m_DescriptorSetManager.HasDescriptorSets();
	}

	uint32_t ComputePass::GetFirstSetIndex() const
	{
		return m_DescriptorSetManager.GetFirstSetIndex();
	}

	bool ComputePass::Validate()
	{
		return m_DescriptorSetManager.Validate();
	}

	void ComputePass::Bake()
	{
		m_DescriptorSetManager.Bake();
	}

	void ComputePass::Prepare()
	{
		m_DescriptorSetManager.InvalidateAndUpdate();
	}

    const std::vector<VkDescriptorSet> &ComputePass::GetDescriptorSets(uint32_t frameIndex) const
	{
		return m_DescriptorSetManager.GetDescriptorSets(frameIndex);
	}

	Ref<ComputePipeline> ComputePass::GetPipeline() const
	{
		return m_Specification.pipeline;
	}

	bool ComputePass::IsInputValid(std::string_view name) const
	{
		std::string nameStr(name);
		return m_DescriptorSetManager.inputDeclarations.find(nameStr) != m_DescriptorSetManager.inputDeclarations.end();
	}

	std::set<uint32_t> ComputePass::HasBufferSets() const
	{
		return m_DescriptorSetManager.HasBufferSets();
	}

	const RenderPassInputDeclaration* ComputePass::GetInputDeclaration(std::string_view name) const
	{
		return m_DescriptorSetManager.GetInputDeclaration(name);
	}

}

/// -------------------------------------------------------

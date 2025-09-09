/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* compute_pass.h
* -------------------------------------------------------
* Created: 24/7/2025
* -------------------------------------------------------
*/
#pragma once
//#include "compute_pipeline.h"
//#include "texture.h"
//#include "vulkan/vk_descriptor_set_manager.h"
//#include <Math/includes/vector.h>

/// -------------------------------------------------------

/*
namespace SceneryEditorX
{

	struct ComputePassSpecification
	{
	    Ref<ComputePipeline> pipeline;
	    std::string debugName;
	    Vec4 markerColor;
	};

    /// -------------------------------------------------------

	class ComputePass : public RefCounted
	{
	public:
        explicit ComputePass(const ComputePassSpecification &spec);
        virtual ~ComputePass() override;

		ComputePassSpecification &GetSpecification() { return m_Specification; }
		const ComputePassSpecification &GetSpecification() const { return m_Specification; }
        bool HasDescriptorSets() const;

		Ref<Shader> GetShader() const { return m_Specification.pipeline->GetShader(); }
		void AddInput(std::string_view name, Ref<UniformBufferSet> uniformBufferSet);
        void AddInput(std::string_view name, Ref<UniformBuffer> uniformBuffer);
		void AddInput(std::string_view name, Ref<StorageBufferSet> storageBufferSet);
        void AddInput(std::string_view name, Ref<StorageBuffer> storageBuffer);
		void AddInput(std::string_view name, Ref<Texture2D> texture);
        void AddInput(std::string_view name, Ref<TextureCube> textureCube);
        void AddInput(std::string_view name, Ref<Image2D> image);

        uint32_t GetFirstSetIndex() const;
		Ref<Image2D> GetOutput(uint32_t index);
		Ref<Image2D> GetDepthOutput();

		bool Validate();
        void Prepare();
		void Bake();
		bool Baked() const { return (bool)m_DescriptorSetManager.GetDescriptorPool(); }

	    const std::vector<VkDescriptorSet> &GetDescriptorSets(uint32_t frameIndex) const;
        Ref<ComputePipeline> GetPipeline() const;
        bool IsInputValid(std::string_view name) const;
        const RenderPassInputDeclaration *GetInputDeclaration(std::string_view name) const;

    private:
        std::set<uint32_t> HasBufferSets() const;
        bool IsInvalidated(uint32_t set, uint32_t binding) const;

	    ComputePassSpecification m_Specification;
        DescriptorSetManager m_DescriptorSetManager;
	};

}
*/

/// -------------------------------------------------------

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
#include <SceneryEditorX/renderer/compute_pipeline.h>
#include <SceneryEditorX/renderer/vulkan/vk_descriptor_set_manager.h>
// STL
#include <set>            // std::set used in private declaration
#include <string_view>    // std::string_view parameters
#include <string>         // std::string in specification

// Math types
#include <SceneryEditorX/utils/math/vector.h> // Vec4


/// -------------------------------------------------------

namespace SceneryEditorX
{

	/// Forward declarations
	class UniformBufferSet;
	class UniformBuffer;
    class StorageBufferSet;
	class StorageBuffer;
	class Texture2D;
	class TextureCube;
	class Image2D;

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
        ComputePass(const ComputePassSpecification &spec);
		virtual ~ComputePass() override;

		virtual ComputePassSpecification& GetSpecification() { return m_Specification; }
		virtual const ComputePassSpecification& GetSpecification() const { return m_Specification; }

		virtual Ref<Shader> GetShader() const { return m_Specification.pipeline->GetShader(); }

		virtual void AddInput(std::string_view name, Ref<UniformBufferSet> uniformBufferSet);
        virtual void AddInput(std::string_view name, Ref<UniformBuffer> uniformBuffer);

		virtual void AddInput(std::string_view name, Ref<StorageBufferSet> storageBufferSet);
        virtual void AddInput(std::string_view name, Ref<StorageBuffer> storageBuffer);

		virtual void AddInput(std::string_view name, Ref<Texture2D> texture);
        virtual void AddInput(std::string_view name, Ref<TextureCube> textureCube);
        virtual void AddInput(std::string_view name, Ref<Image2D> image);

		virtual Ref<Image2D> GetOutput(uint32_t index);
		virtual Ref<Image2D> GetDepthOutput();
		virtual bool HasDescriptorSets() const;
        virtual uint32_t GetFirstSetIndex() const;

		virtual bool Validate();
		virtual void Bake();
		virtual bool Baked()
		{
		    return static_cast<bool>(m_DescriptorSetManager.GetDescriptorPool());
		}

		virtual void Prepare();

		const std::vector<VkDescriptorSet> &GetDescriptorSets(uint32_t frameIndex) const;

        virtual Ref<ComputePipeline> GetPipeline() const;
        bool IsInputValid(std::string_view name) const;
        const RenderPassInputDeclaration *GetInputDeclaration(std::string_view name) const;

    private:
        std::set<uint32_t> HasBufferSets() const;
        bool IsInvalidated(uint32_t set, uint32_t binding) const;

	    ComputePassSpecification m_Specification;
        DescriptorSetManager m_DescriptorSetManager;
	};


}

/// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* material.cpp
* -------------------------------------------------------
* Created: 11/8/2025
* -------------------------------------------------------
*/
#include "material.h"
#include "SceneryEditorX/renderer/descriptor_set_manager.h"
#include "SceneryEditorX/renderer/renderer.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	Material::Material(const Ref<Shader> &shader, const std::string &name) : m_Shader(shader.As<Shader>()), m_Name(name)
	{
        Init();
        Renderer::RegisterShader(shader, this);
	}

	Material::Material(Ref<Material> &material, const std::string &name) : m_Shader(material->GetShader().As<Shader>()), m_Name(name)
	{
        if (name.empty())
            m_Name = material->GetName();

        Init();
        Renderer::RegisterShader(m_Shader, this);

        auto vulkanMaterial = material.As<Material>();
        m_UniformStorageBuffer = Buffer::Copy(vulkanMaterial->m_UniformStorageBuffer.data, vulkanMaterial->m_UniformStorageBuffer.size);
        m_DescriptorSetManager = DescriptorSetManager::Copy(vulkanMaterial->m_DescriptorSetManager);
	}

	Material::~Material()
	{
        m_UniformStorageBuffer.Release();
	}

    const ShaderUniform* Material::FindUniformDeclaration(const std::string &name)
    {
        const auto &shaderBuffers = m_Shader->GetShaderBuffers();

        SEDX_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");

        if (shaderBuffers.size() > 0)
        {
            const ShaderBuffer &buffer = (*shaderBuffers.begin()).second;
            if (buffer.Uniforms.find(name) == buffer.Uniforms.end())
                return nullptr;

            return &buffer.Uniforms.at(name);
        }
        return nullptr;
    }

    const ShaderResourceDeclaration* Material::FindResourceDeclaration(const std::string &name)
    {
        auto &resources = m_Shader->GetResources();
        if (resources.find(name) != resources.end())
            return &resources.at(name);

        return nullptr;
    }

    void Material::Init()
    {
        AllocateStorage();

        m_MaterialFlags |= (uint32_t)MaterialFlag::DepthTest;
        m_MaterialFlags |= (uint32_t)MaterialFlag::Blend;

        DescriptorSetManagerSpecification dmSpec;
        dmSpec.DebugName = m_Name.empty() ? std::format("{} (Material)", m_Shader->GetName()) : m_Name;
        dmSpec.Shader = m_Shader.As<Shader>();
        dmSpec.StartSet = 0;
        dmSpec.EndSet = 0;
        dmSpec.DefaultResources = true;
        m_DescriptorSetManager = DescriptorSetManager(dmSpec);

        for (const auto &[name, decl] : m_DescriptorSetManager.InputDeclarations)
        {
            switch (decl.Type)
            {
            case RenderPassInputType::ImageSampler1D:
            case RenderPassInputType::ImageSampler2D:
            {
                for (uint32_t i = 0; i < decl.Count; i++)
                    m_DescriptorSetManager.SetInput(name, Renderer::GetWhiteTexture(), i);
                break;
            }
            case RenderPassInputType::ImageSampler3D:
            {
                m_DescriptorSetManager.SetInput(name, Renderer::GetBlackCubeTexture());
                break;
            }
            }
        }

        SEDX_CORE_VERIFY(m_DescriptorSetManager.Validate());
        m_DescriptorSetManager.Bake();
    }
    void Material::AllocateStorage()
    {
        const auto &shaderBuffers = m_Shader->GetShaderBuffers();

        if (shaderBuffers.size() > 0)
        {
            uint32_t size = 0;
            for (auto [name, shaderBuffer] : shaderBuffers)
                size += shaderBuffer.Size;

            m_UniformStorageBuffer.Allocate(size);
            m_UniformStorageBuffer.ZeroInitialize();
        }
    }

    Ref<Material> Material::Create(const Ref<Shader> &shader, const std::string &name)
	{
        return CreateRef<Material>(shader, name);
	}

	Ref<Material> Material::Copy(const Ref<Material> &other, const std::string &name)
	{
        return CreateRef<Material>(other, name);
	}

	void Material::Prepare()
	{
        m_DescriptorSetManager.InvalidateAndUpdate();
	}

	void Material::Invalidate()
	{
        // Allocate descriptor set 0 based on shader layout
        if (m_Shader->HasDescriptorSet(0))
        {
            RenderData renderData;
            VkDescriptorSetLayout dsl = m_Shader->GetDescriptorSetLayout(0);
            VkDescriptorSetAllocateInfo descriptorSetAllocInfo = DescriptorSetAllocInfo(&dsl);
            m_MaterialDescriptorSets.resize(renderData.framesInFlight);
            for (uint32_t i = 0; i < renderData.framesInFlight; i++)
                m_MaterialDescriptorSets[i] = Renderer::AllocateMaterialDescriptorSet(descriptorSetAllocInfo);

            // Sort into map sorted by binding
            const auto &shaderDescriptorSets = m_Shader->GetShaderDescriptorSets();
            std::map<uint32_t, VkWriteDescriptorSet> writeDescriptors;
            std::set<uint32_t> textureCubes; // temp
            for (const auto &[name, writeDescriptor] : shaderDescriptorSets[0].writeDescriptorSets)
            {
                if (writeDescriptor.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
                    writeDescriptor.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
                    writeDescriptor.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                {
                    writeDescriptors[writeDescriptor.dstBinding] = writeDescriptor;
                }
            }

            // Ordered map
            for (const auto &[binding, writeDescriptor] : writeDescriptors)
            {
                m_MaterialWriteDescriptors[binding] = writeDescriptor;
                m_MaterialDescriptorImages[binding] =
                    std::vector<Ref<Resource>>(writeDescriptor.descriptorCount);

                if (writeDescriptor.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                {
                    // Set default image infos
                    for (size_t i = 0; i < writeDescriptor.descriptorCount; i++)
                    {
                        // TODO: check if cube or 2D
                        m_MaterialDescriptorImages[binding][i] = Renderer::GetWhiteTexture();
                    }
                }
            }
        }
        else
        {
            SEDX_CORE_WARN_TAG("Renderer", "[Material] - shader {} has no Set 0!", m_Shader->GetName());
        }
	}

	void Material::OnShaderReloaded()
	{
	}

	void Material::Set(const std::string &name, float value)
	{
        Set<float>(name, value);
	}

	void Material::Set(const std::string &name, int value)
	{
        Set<int>(name, value);
	}

	void Material::Set(const std::string &name, uint32_t value)
	{
        Set<uint32_t>(name, value);
	}

	void Material::Set(const std::string &name, bool value)
	{
        Set<int>(name, (int)value);	// Bool are 4-byte ints
	}

	void Material::Set(const std::string &name, const iVec2 &value)
	{
        Set<iVec2>(name, value);
	}

	void Material::Set(const std::string &name, const iVec3 &value)
    {
        Set<iVec3>(name, value);
	}

	void Material::Set(const std::string &name, const iVec4 &value)
	{
        Set<iVec4>(name, value);
	}

	void Material::Set(const std::string &name, const Vec2 &value)
	{
        Set<Vec2>(name, value);
	}

	void Material::Set(const std::string &name, const Vec3 &value)
	{
        Set<Vec3>(name, value);
	}

	void Material::Set(const std::string &name, const Vec4 &value)
	{
        Set<Vec4>(name, value);
	}

	void Material::Set(const std::string &name, const Mat3 &value)
	{
        Set<Mat3>(name, value);
	}

	void Material::Set(const std::string &name, const Mat4 &value)
	{
        Set<Mat4>(name, value);
	}

	void Material::Set(const std::string &name, const Ref<Texture2D> &texture, uint32_t arrayIndex)
	{
        SetDescriptor(name, texture, arrayIndex);
	}

	void Material::Set(const std::string &name, const Ref<TextureCube> &texture, uint32_t arrayIndex)
	{
        SetDescriptor(name, texture, arrayIndex);
	}

	void Material::Set(const std::string &name, const Ref<Image2D> &image, uint32_t arrayIndex)
	{
        SetDescriptor(name, image, arrayIndex);
	}

	void Material::Set(const std::string &name, const Ref<ImageView> &image, uint32_t arrayIndex)
	{
        SetDescriptor(name, image, arrayIndex);
	}

	Vec2 &Material::GetVector2(const std::string &name)
	{
        return Get<Vec2>(name);
	}

	Vec3 &Material::GetVector3(const std::string &name)
	{
        return Get<Vec3>(name);
	}

	Vec4 &Material::GetVector4(const std::string &name)
	{
        return Get<Vec4>(name);
	}

	Mat3 &Material::GetMatrix3(const std::string &name)
	{
        return Get<Mat3>(name);
	}

	Mat4 &Material::GetMatrix4(const std::string &name)
	{
        return Get<Mat4>(name);
	}

	bool &Material::GetBool(const std::string &name)
	{
        return Get<bool>(name);
	}

	float &Material::GetFloat(const std::string &name)
	{
        return Get<float>(name);
	}

	int32_t &Material::GetInt(const std::string &name)
	{
        return Get<int32_t>(name);
	}

	uint32_t &Material::GetUInt(const std::string &name)
	{
        return Get<uint32_t>(name);
	}

    void Material::SetDescriptor(const std::string &name, const Ref<Texture2D> &texture, uint32_t arrayIndex)
    {
        m_DescriptorSetManager.SetInput(name, texture, arrayIndex);
    }

    void Material::SetDescriptor(const std::string &name, const Ref<TextureCube> &texture, uint32_t arrayIndex)
    {
        m_DescriptorSetManager.SetInput(name, texture, arrayIndex);
    }

    void Material::SetDescriptor(const std::string &name, const Ref<Image2D> &image, uint32_t arrayIndex)
    {
        SEDX_CORE_VERIFY(image);
        m_DescriptorSetManager.SetInput(name, image, arrayIndex);
    }

    void Material::SetDescriptor(const std::string &name, const Ref<ImageView> &image, uint32_t arrayIndex)
    {
        SEDX_CORE_VERIFY(image);
        m_DescriptorSetManager.SetInput(name, image, arrayIndex);
    }

}

// -------------------------------------------------------

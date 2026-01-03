/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* shader.cpp
* -------------------------------------------------------
* Created: 15/4/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/platform/filesystem/file_manager.hpp>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/shaders/shader.h>
#include "SceneryEditorX/renderer/image_data.h"
#include "SceneryEditorX/renderer/descriptors.h"
#include <SceneryEditorX/renderer/shaders/shader_pack.h>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_hlsl.hpp>

// -----------------------------------------

namespace SPIRV_CROSS_NAMESPACE
{
    static void SpirvResourcesToDescriptors( const CompilerHLSL& compiler, std::vector<SceneryEditorX::Descriptors>& descriptors, const SmallVector<Resource>& resources, const SceneryEditorX::DescriptorType descriptor_type, const SceneryEditorX::ShaderType shader_stage)
    {
        // this only matters for textures
        SceneryEditorX::Layout::ImageLayout layout = SceneryEditorX::Layout::ImageLayout::Max;
        layout = descriptor_type == SceneryEditorX::DescriptorType::TextureStorage ? SceneryEditorX::Layout::ImageLayout::General   : layout;
        layout = descriptor_type == SceneryEditorX::DescriptorType::Image          ? SceneryEditorX::Layout::ImageLayout::Read		: layout;

        for (const Resource& resource : resources)
        {
            uint32_t slot         = compiler.get_decoration(resource.id, spv::DecorationBinding);
            SPIRType type         = compiler.get_type(resource.type_id);
            uint32_t size         = 0;
            bool is_array         = !type.array.empty();
            uint32_t array_length = is_array ? type.array[0] : 0;

            if (descriptor_type == SceneryEditorX::DescriptorType::ConstantBuffer || descriptor_type == SceneryEditorX::DescriptorType::PushConstantBuffer)
            {
                size = static_cast<uint32_t>(compiler.get_declared_struct_size(type));
            }

            if (is_array && array_length == 0)
            {
                array_length = SceneryEditorX::MAX_ARRAY_SIZE;
            }

            descriptors.emplace_back
            (
                resource.name,                         // name
                descriptor_type,                       // type
                layout,                                // layout
                slot,                                  // slot
                SceneryEditorX::ShaderTypeToMask(shader_stage),     // stage
                size,                                  // struct size
                is_array,                              // is array
                array_length                           // array length
            );
        }
    };

    std::atomic<bool> spriv_cross_registered = false;
} // namespace SPIRV_CROSS_NAMESPACE

namespace SceneryEditorX
{

	Shader::Shader() = default;

    Shader::Shader(const std::string &filepath) : m_Name(filepath)
    {
        /// Load shader from file
        std::string shaderPath = GetShaderDirectoryPath() + filepath;
        const auto shaderCode = IO::FileManager::ReadShaders(shaderPath);
        if (shaderCode.empty())
        {
            SEDX_CORE_ERROR("Failed to load shader from file: {}", shaderPath);
            return;
        }

        if (const VkShaderModule shaderModule = CreateShaderModule(shaderCode); shaderModule == VK_NULL_HANDLE)
        {
            SEDX_CORE_ERROR("Failed to create shader module from file: {}", shaderPath);
            return;
        }
    }

    Shader::~Shader()
    {
		if (m_Resource)
		{
            const auto device = RenderContext::GetCurrentDevice()->GetDevice();
            m_Resource = nullptr;
		}

        /*
        const auto device = RenderContext::GetCurrentDevice()->GetDevice();
        const auto ctx = RenderContext::Get();
        vkDestroyShaderModule(device, shaderModule, ctx->allocatorCallback);
        */
    }

    void Shader::LoadFromShaderPack(const std::string &filepath, bool forceCompile, bool disableOptimization)
    {
    }

    Ref<Shader> Shader::CreateFromString(const std::string &source)
    {
        auto shader = CreateRef<Shader>();
        shader->LoadFromShaderPack(source);
        return shader;
    }

    const std::string & Shader::GetName() const { return m_Name; }
    void *Shader::GetRendererResource() const { return m_Resource; }

    VkPipelineShaderStageCreateInfo Shader::CreateShaderStage(const Shader *shader)
    {
        VkPipelineShaderStageCreateInfo shader_stage_info = {};
        shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_info.module = static_cast<VkShaderModule>(shader->GetRendererResource());
        shader_stage_info.pName = shader->GetEntryPoint();

        if (shader->GetShaderStage() == ShaderType::Vertex)
        {
            shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        }
        else if (shader->GetShaderStage() == ShaderType::TesselationControl)
        {
            shader_stage_info.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        }
        else if (shader->GetShaderStage() == ShaderType::TesselationEval)
        {
            shader_stage_info.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        }
        else if (shader->GetShaderStage() == ShaderType::Fragment)
        {
            shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        else if (shader->GetShaderStage() == ShaderType::Compute)
        {
            shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        }

        SEDX_ASSERT(shader_stage_info.stage != 0);
        SEDX_ASSERT(shader_stage_info.module != nullptr);
        SEDX_ASSERT(shader_stage_info.pName != nullptr);

        return shader_stage_info;
    }

    void Shader::AddShaderReloadedCallback(const ShaderReloadedCallback &callback)
    {
        /**
         * Implementation of the pure virtual function
         * Store callbacks that will be triggered when the shader is reloaded
         */
        m_ReloadCallbacks.push_back(callback);
    }

    const char* Shader::GetEntryPoint() const
    {
        switch (m_ShaderType)
        {
			case ShaderType::Vertex:				return "main_vs";
			case ShaderType::TesselationControl:	return "main_hs";
			case ShaderType::TesselationEval:		return "main_ds";
			case ShaderType::Fragment:				return "main_ps";
			case ShaderType::Compute:				return "main_cs";
			default:								return nullptr;
        }
    }

    VkShaderModule Shader::CreateShaderModule(const std::vector<char> &code) const
    {
        const auto device = RenderContext::GetCurrentDevice()->GetDevice();
        const auto ctx = RenderContext::Get();
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule = nullptr;
        if (vkCreateShaderModule(device, &createInfo, ctx->allocatorCallback, &shaderModule) != VK_SUCCESS)
            SEDX_CORE_ERROR("Failed to create shader module!");

        return shaderModule;
	}

    ShaderResource::UniformBuffer Shader::GetUniformBuffer(const uint32_t binding, const uint32_t set)
    {
        SEDX_CORE_ASSERT(m_ReflectionData.ShaderDescriptorSets.at(set).uniformBuffers.size() > binding);
        const auto &ub = m_ReflectionData.ShaderDescriptorSets.at(set).uniformBuffers.at(binding);

        ShaderResource::UniformBuffer result;
        result.descriptor = ub.GetDescriptorInfo(1);
        if (ub.GetBufferCount() > 0)
        {
            result.size = ub.GetBufferCount();
        }
        else
            result.size = ub.GetBufferCount(); // You may want to set this to ub.size if available
        result.bindingPoint = binding;
        result.name = "";                                        // If UniformBuffer has a name, set it here
        result.ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM; // Set actual stage if available

        return result;
    }

    uint32_t Shader::GetUniformBufferCount(const uint32_t set) const
    {
        if (m_ReflectionData.ShaderDescriptorSets.size() < set)
            return 0;

        return static_cast<uint32_t>(m_ReflectionData.ShaderDescriptorSets[set].uniformBuffers.size());
    }

    void ShaderLibrary::Add(const Ref<Shader>& shader)
	{
		auto& name = shader->GetName();
        SEDX_CORE_ASSERT(!m_Shaders.contains(name));
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Load(std::string_view path, bool forceCompile, bool disableOptimization)
	{
		Ref<Shader> shader;
		if (!forceCompile && m_ShaderPack)
		{
			if (m_ShaderPack->Contains(path))
				shader = m_ShaderPack->LoadShader(path);
		}
		else
		{
			// Try to compile from source
			// Unavailable at runtime only works in Debug mode
	#if SEDX_HAS_SHADER_COMPILER
            shader = ShaderCompiler::CompileShader(path, forceCompile, disableOptimization);
	#endif
		}

		auto& name = shader->GetName();
		SEDX_CORE_ASSERT(!m_Shaders.contains(name));
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Load(const std::string_view name, const std::string& path)
	{
        SEDX_CORE_ASSERT(!m_Shaders.contains(std::string(name)));
		m_Shaders[std::string(name)] = CreateRef<Shader>(path);
	}

	void ShaderLibrary::LoadShaderPack(const std::filesystem::path& path)
	{
        m_ShaderPack = CreateRef<ShaderPack>(path);
		if (!m_ShaderPack->IsLoaded())
		{
			m_ShaderPack = nullptr;
			SEDX_CORE_ERROR("Could not load shader pack: {}", path.string());
		}
	}

	const Ref<Shader>& ShaderLibrary::Get(const std::string& name) const
	{
        SEDX_CORE_ASSERT(m_Shaders.contains(name));
		return m_Shaders.at(name);
	}

	/*
	ShaderUniform::ShaderUniform(std::string name, const ShaderUniformType type, const uint32_t size, const uint32_t offset) : m_Name(std::move(name)), m_Type(type), m_Size(size), m_Offset(offset)
	{
	}
	*//*

    constexpr std::string_view ShaderUniform::UniformTypeToString(const ShaderUniformType type)
    {
        switch (type)
        {
            case ShaderUniformType::Bool:
                return "Boolean";
            case ShaderUniformType::Int:
                return "Int";
            case ShaderUniformType::Float:
                return "Float";
            case ShaderUniformType::UInt:
                return "Unsigned Int";
            case ShaderUniformType::Vec2:
                return "Vec2";
            case ShaderUniformType::Vec3:
                return "Vec3";
            case ShaderUniformType::Vec4:
                return "Vec4";
            case ShaderUniformType::Mat3:
                return "Mat3";
            case ShaderUniformType::Mat4:
                return "Mat4";
            case ShaderUniformType::IVec2:
                return "IVec2";
            case ShaderUniformType::IVec3:
                return "IVec3";
            case ShaderUniformType::IVec4:
                return "IVec4";
            case ShaderUniformType::None:
                break;
        }

        return "None";
    }
    */


}

// -----------------------------------------

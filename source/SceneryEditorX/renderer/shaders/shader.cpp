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
#include <SceneryEditorX/platform/file_manager.hpp>
#include <SceneryEditorX/renderer/shaders/shader.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/shaders/shader_pack.h>

/// -----------------------------------------

namespace SceneryEditorX
{

	Shader::Shader(const std::string &filepath) : name(filepath)
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
        const auto device = RenderContext::GetCurrentDevice()->GetDevice();
        const auto context = RenderContext::Get();
        vkDestroyShaderModule(device, shaderModule, context->allocatorCallback);
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

    const std::string & Shader::GetName() const
    {
        return name;
    }

    void Shader::AddShaderReloadedCallback(const ShaderReloadedCallback &callback)
    {
        /// Implementation of the pure virtual function
        /// Store callbacks that will be triggered when the shader is reloaded
        reloadCallbacks.push_back(callback);
    }

    VkShaderModule Shader::CreateShaderModule(const std::vector<char> &code) const
    {
        const auto device = RenderContext::GetCurrentDevice()->GetDevice();
        const auto context = RenderContext::Get();
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule = nullptr;
        if (vkCreateShaderModule(device, &createInfo, context->allocatorCallback, &shaderModule) != VK_SUCCESS)
            SEDX_CORE_ERROR("Failed to create shader module!");

        return shaderModule;
	}

    ShaderLibrary::ShaderLibrary() = default;

    ShaderLibrary::~ShaderLibrary() = default;

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
			/// Try to compile from source
			/// Unavailable at runtime only works in Debug mode
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

	ShaderUniform::ShaderUniform(std::string name, const ShaderUniformType type, const uint32_t size, const uint32_t offset) : m_Name(std::move(name)), m_Type(type), m_Size(size), m_Offset(offset)
	{
	}

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


}

/// -----------------------------------------

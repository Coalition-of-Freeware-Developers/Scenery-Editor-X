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
        auto device = RenderContext::GetCurrentDevice()->GetDevice();
        auto context = RenderContext::Get();
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
        // Implementation of the pure virtual function
        // Store callbacks that will be triggered when the shader is reloaded
        reloadCallbacks.push_back(callback);
    }

    VkShaderModule Shader::CreateShaderModule(const std::vector<char> &code) const
    {
        auto device = RenderContext::GetCurrentDevice()->GetDevice();
        auto context = RenderContext::Get();
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule = nullptr;
        if (vkCreateShaderModule(device, &createInfo, context->allocatorCallback, &shaderModule) != VK_SUCCESS)
            SEDX_CORE_ERROR("Failed to create shader module!");

        return shaderModule;
	}

} // namespace SceneryEditorX

/// -----------------------------------------

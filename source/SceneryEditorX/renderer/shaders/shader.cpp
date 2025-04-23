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
#include <SceneryEditorX/platform/windows/file_manager.hpp>
#include <SceneryEditorX/renderer/shaders/shader.h>
#include <string>

// -----------------------------------------

namespace SceneryEditorX
{

	Shader::Shader(const std::string &filepath, bool forceCompile, bool disableOptimization)
	{
	    // Load shader from file
		std::string shaderPath = GetShaderDirectoryPath() + filepath;
		auto shaderCode = IO::FileManager::ReadShaders(shaderPath);
		if (shaderCode.empty())
		{
			SEDX_CORE_ERROR("Failed to load shader from file: {}", shaderPath);
			ErrMsg("failed to load shader from file!");
			return;
		}

		VkShaderModule shaderModule = CreateShaderModule(shaderCode);
		if (shaderModule == VK_NULL_HANDLE)
		{
			SEDX_CORE_ERROR("Failed to create shader module from file: {}", shaderPath);
			ErrMsg("failed to create shader module!");
			return;
		}

	}

    Shader::~Shader()
    {
        VkDevice vkDevice = device->GetDevice();
        vkDestroyShaderModule(vkDevice, shaderModule, nullptr);
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
        return {};
    }

    void Shader::AddShaderReloadedCallback(const ShaderReloadedCallback &callback)
    {
        // Implementation of the pure virtual function
        // Store callbacks that will be triggered when the shader is reloaded
        reloadCallbacks.push_back(callback);
    }

    VkShaderModule Shader::CreateShaderModule(const std::vector<char> &code) const
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device->GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create shader module!");
            ErrMsg("failed to create shader module!");
        }

        return shaderModule;
	}


} // namespace SceneryEditorX

// -----------------------------------------

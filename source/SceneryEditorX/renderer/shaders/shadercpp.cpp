/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* shadercpp.cpp
* -------------------------------------------------------
* Created: 15/4/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/renderer/shaders/shader.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	VkShaderModule Shader::CreateShaderModule(const std::vector<char> &code)
	{
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device->GetDevice(), &createInfo, allocator, &shaderModule) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create shader module!");
            ErrMsg("failed to create shader module!");
        }

        return shaderModule;
	
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

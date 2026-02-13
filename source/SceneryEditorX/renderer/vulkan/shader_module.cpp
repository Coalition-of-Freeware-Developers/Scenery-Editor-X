/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * shader_module.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "shader_module.h"
#include "render_context.h"
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

    ShaderModule::ShaderModule(const void *code, size_t codeSize) : m_Device(RenderContext::Get()->GetDevice())
    {
        if (!code || codeSize == 0)
            return;

        VkShaderModuleCreateInfo ci
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = codeSize,
            .pCode = reinterpret_cast<const uint32_t *>(code)
        };

        if (VkResult res = vkCreateShaderModule(Device::GetDevice(), &ci, nullptr, &m_Module); res != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("vkCreateShaderModule failed: {}", res);
            m_Module = VK_NULL_HANDLE;
        }
    }

    ShaderModule::~ShaderModule()
    {
        if (m_Module != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(Device::GetDevice(), m_Module, nullptr);
        }
    }

    ShaderModule::ShaderModule(ShaderModule && other) noexcept : m_Device(other.m_Device), m_Module(other.m_Module)
    {
        other.m_Device = VK_NULL_HANDLE;
        other.m_Module = VK_NULL_HANDLE;
    }

    ShaderModule &ShaderModule::operator=(ShaderModule &&other) noexcept
    {
        if (this != &other)
        {
            if (m_Module != VK_NULL_HANDLE)
            {
                vkDestroyShaderModule(Device::GetDevice(), m_Module, nullptr);
            }

            m_Device = other.m_Device;
            m_Module = other.m_Module;
            other.m_Device = VK_NULL_HANDLE;
            other.m_Module = VK_NULL_HANDLE;
        }
        return *this;
    }

}

// -------------------------------------------------------

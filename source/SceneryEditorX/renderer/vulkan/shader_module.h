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
 * shader_module.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "device.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	class ShaderModule 
    {
	public:
	    ShaderModule() = default;
	    ShaderModule(const void* code, size_t codeSize);
	    ~ShaderModule();
	
	    ShaderModule(const ShaderModule&) = delete;
	    ShaderModule& operator=(const ShaderModule&) = delete;
	    ShaderModule(ShaderModule&& other) noexcept;
	    ShaderModule& operator=(ShaderModule&& other) noexcept;
	
	    VkShaderModule Get() const { return m_Module; }
	    bool Valid() const { return m_Module != VK_NULL_HANDLE; }
	
	private:
	    Ref<Device> m_Device;
	    VkShaderModule m_Module = VK_NULL_HANDLE;
	};
	
}

// -------------------------------------------------------

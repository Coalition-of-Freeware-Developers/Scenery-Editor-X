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
 * shader_manager.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "shader_module.h"
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
	// Small manager that owns one or more ShaderModule objects along with their
	// corresponding shader stage flags. This allows Pipeline to accept a single
	// object that may contain multiple stages (vertex, fragment, etc.).
	class ShaderManager 
    {
	public:
	    ShaderManager() = default;
	    // Convenience ctor: use the same SPIR-V blob for both vertex and fragment
	    // stages (matches the original sample behavior).
	    ShaderManager(const void* spirvCode, size_t codeSize);
	
	    // General ctor: provide stages and blobs (ownership copied into modules)
	    ShaderManager(const std::vector<std::pair<VkShaderStageFlagBits, std::pair<const void*, size_t>>>& stages);
	
	    ~ShaderManager() = default;
	
	    size_t StageCount() const { return m_Stages.size(); }
	    VkShaderStageFlagBits StageAt(size_t i) const { return m_Stages[i]; }
	    VkShaderModule ModuleAt(size_t i) const { return m_Modules[i].Get(); }
	
	private:
	    std::vector<VkShaderStageFlagBits> m_Stages{};
	    std::vector<ShaderModule> m_Modules{};
	};

}

// -------------------------------------------------------

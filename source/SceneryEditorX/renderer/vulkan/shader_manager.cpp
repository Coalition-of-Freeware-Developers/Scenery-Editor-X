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
 * shader_manager.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "shader_manager.h"
#include <utility>

// -------------------------------------------------------

namespace SceneryEditorX
{
	ShaderManager::ShaderManager(const void* spirvCode, size_t codeSize)
	{
	    // Use the same module for vertex and fragment stages by default.
	    m_Stages.reserve(2);
	    m_Modules.reserve(2);
	    m_Stages.push_back(VK_SHADER_STAGE_VERTEX_BIT);
	    m_Modules.emplace_back(spirvCode, codeSize);
	    m_Stages.push_back(VK_SHADER_STAGE_FRAGMENT_BIT);
	    m_Modules.emplace_back(spirvCode, codeSize);
	}
	
	ShaderManager::ShaderManager(const std::vector<std::pair<VkShaderStageFlagBits, std::pair<const void*, size_t>>>& stages)
	{
	    m_Stages.reserve(stages.size());
	    m_Modules.reserve(stages.size());
	    for (const auto& s : stages)
		{
	        m_Stages.push_back(s.first);
	        m_Modules.emplace_back(s.second.first, s.second.second);
	    }
	}
	
}

// -------------------------------------------------------

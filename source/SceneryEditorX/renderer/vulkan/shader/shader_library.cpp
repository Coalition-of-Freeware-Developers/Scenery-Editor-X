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
 * shader_library.cpp
 * -------------------------------------------------------
 * Created: 03/07/2026
 * -------------------------------------------------------
 */
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include "shader_library.h"
#include "shader_cache.h"
#include "shader_pack.h"
#include <SceneryEditorX/utils/pointers.h>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/logging/logging.hpp>

// -------------------------------------------------------------

namespace SceneryEditorX 
{

	ShaderLibrary::ShaderLibrary() = default;
	ShaderLibrary::~ShaderLibrary() = default;

	// Pipelines need no explicit invalidation on reload: CommandList rebuilds pipelines from the PSO,
	// and the PSO hash (which keys the descriptor-layout cache) includes Shader::GetHash().

	void ShaderLibrary::Add(const Ref<Shader>& shader)
	{
		auto& name = shader->GetName();
		SEDX_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
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

		// Shader::Create compiles .slang sources through SlangCompiler and loads anything else as precompiled SPIR-V.
		if (!shader)
			shader = Shader::Create(std::string(path), forceCompile, disableOptimization);

		if (!shader)
		{
			SEDX_CORE_ERROR_TAG("ShaderLibrary", "Failed to load shader '{}'", std::string(path));
			return;
		}

		const auto& name = shader->GetName();
		SEDX_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Load(std::string_view name, const std::string& path)
	{
		SEDX_CORE_ASSERT(m_Shaders.find(std::string(name)) == m_Shaders.end());
		Ref<Shader> shader = Shader::Create(path);
		if (!shader)
		{
			SEDX_CORE_ERROR_TAG("ShaderLibrary", "Failed to load shader '{}' from '{}'", std::string(name), path);
			return;
		}

		m_Shaders[std::string(name)] = shader;
	}

	void ShaderLibrary::Clear()
	{
		m_Shaders.clear();
		m_HotReloadState.clear();
		m_ShaderPack = nullptr;
	}

	void ShaderLibrary::LoadShaderPack(const std::filesystem::path& path) 
	{
		m_ShaderPack = CreateRef<ShaderPack>(path);
		if (!m_ShaderPack->IsLoaded())
		{
			m_ShaderPack = nullptr;
			SEDX_CORE_ERROR_TAG("ShaderLibrary","Could not load shader pack: {}", path.string());
		}
	}

	const Ref<Shader>& ShaderLibrary::Get(const std::string& name) const 
	{
		SEDX_CORE_ASSERT(m_Shaders.find(name) != m_Shaders.end());
		return m_Shaders.at(name);
	}

	void ShaderLibrary::PollHotReload() noexcept
	{
#if defined(SEDX_DEBUG)
		if (!RenderContext::IsInitialized())
			return;

		for (auto& [name, shader] : m_Shaders)
		{
			if (!shader || shader->GetAssetPath().empty())
				continue;

			// Slang-built shaders track their own source/import timestamps.
			if (!shader->GetSourceDependencies().empty())
			{
				if (shader->TryReloadFromDisk())
				{
					SEDX_CORE_INFO_TAG("ShaderLibrary", "Hot-reloaded shader '{}' after source change", name);
				}
				continue;
			}

			const std::filesystem::path spirvPath = ShaderCache::ResolveSpirvPath(shader->GetAssetPath().string());
			if (!std::filesystem::exists(spirvPath))
				continue;

			const auto writeTime = std::filesystem::last_write_time(spirvPath);
			HotReloadState& state = m_HotReloadState[name];
			if (state.tracked && state.lastWriteTime == writeTime)
				continue;

			const bool previouslyTracked = state.tracked;
			state.lastWriteTime = writeTime;
			state.tracked = true;

			if (!previouslyTracked)
				continue;

			if (shader->TryReloadFromDisk())
			{
				SEDX_CORE_INFO_TAG("ShaderLibrary", "Hot-reloaded shader '{}' after SPIR-V change", name);
			}
		}
#endif
	}

}  // namespace SceneryEditorX 

// -------------------------------------------------------------

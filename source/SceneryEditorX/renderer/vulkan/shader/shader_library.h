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
 * shader_library.h
 * -------------------------------------------------------
 * Created: 03/07/2026
 * -------------------------------------------------------
 */
#pragma once
#include "shader.h"
#include <filesystem>

// -------------------------------------------------------------

namespace SceneryEditorX 
{
	class ShaderPack;

	/**
	 * @class ShaderLibrary
	 * @brief Represents a library of shaders, allowing for loading, adding, and retrieving shaders.
	 */
	class ShaderLibrary : public RefCounted
	{
	public:
		ShaderLibrary();
		~ShaderLibrary();

		void Add(const Ref<Shader>& shader);
		void Load(std::string_view path, bool forceCompile = false, bool disableOptimization = false);
		void Load(std::string_view name, const std::string& path);
		void LoadShaderPack(const std::filesystem::path& path);

		const Ref<Shader>& Get(const std::string& name) const;
		[[nodiscard]] bool Contains(const std::string& name) const { return m_Shaders.contains(name); }
		size_t GetSize() const { return m_Shaders.size(); }

		/** @brief Releases every shader, the loaded shader pack and the hot-reload state. */
		void Clear();

		std::unordered_map<std::string, Ref<Shader>>& GetShaders() { return m_Shaders; }
		const std::unordered_map<std::string, Ref<Shader>>& GetShaders() const { return m_Shaders; }

		/** @brief Reload shaders whose Slang sources or SPIR-V files changed on disk (debug builds). */
		void PollHotReload() noexcept;

	private:
		struct HotReloadState
		{
			std::filesystem::file_time_type lastWriteTime{};
			bool tracked = false;
		};

		std::unordered_map<std::string, Ref<Shader>> m_Shaders;
		Ref<ShaderPack> m_ShaderPack;
		std::unordered_map<std::string, HotReloadState> m_HotReloadState;
	};


}  // namespace SceneryEditorX 

// -------------------------------------------------------------

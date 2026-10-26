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
 * shader_cache.h
 * -------------------------------------------------------
 * Created: 03/07/2026
 * -------------------------------------------------------
 */
#pragma once
#include "shader.h"
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @struct CachedShaderProgram
	 * @brief Everything needed to rebuild a Slang-compiled Shader without invoking the compiler.
	 */
	struct CachedShaderProgram
	{
		std::map<ShaderType, std::vector<uint32_t>> stages;
		std::unordered_map<std::string, ShaderResourceDeclaration> resources;
		std::vector<std::filesystem::path> dependencies;	// Entry module + imported modules, used for invalidation
		VertexType vertexType = VertexType::MaxEnum;
		uint32_t pushConstantSize = 0;
	};

	/**
	 * @class ShaderCache
	 * @brief Resolves precompiled SPIR-V paths and loads bytecode from disk, and stores compiled
	 * Slang programs (SPIR-V + reflection) so unchanged shaders skip compilation on the next run.
	 */
	class ShaderCache
	{
	public:
		[[nodiscard]] static std::filesystem::path ResolveSpirvPath(std::string_view sourcePath);
		[[nodiscard]] static bool TryLoadSpirv(const std::filesystem::path& spirvPath, std::vector<uint32_t>& outSpirv);

		/** @brief Directory holding cached program files (the VFS cache folder's "shaders": %LOCALAPPDATA%/SceneryEditorX/cache/shaders on Windows). */
		[[nodiscard]] static std::filesystem::path GetProgramCacheDirectory();

		/**
		 * @brief Loads a cached program. Fails when the file is missing, from an older format,
		 * or when any recorded dependency was modified after the cache file was written.
		 */
		[[nodiscard]] static bool TryLoadProgram(const std::filesystem::path& cacheFile, CachedShaderProgram& outProgram);

		/** @brief Writes a program to the cache. Returns false (and leaves no partial file) on failure. */
		static bool StoreProgram(const std::filesystem::path& cacheFile, const CachedShaderProgram& program);
	};

}  // namespace SceneryEditorX

// -------------------------------------------------------

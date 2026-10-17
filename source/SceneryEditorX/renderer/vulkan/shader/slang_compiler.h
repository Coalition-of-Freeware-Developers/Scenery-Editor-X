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
 * slang_compiler.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "shader.h"
#include <filesystem>
#include <string_view>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct CachedShaderProgram;

	/**
	 * @class SlangCompiler
	 * @brief Compiles Slang source into Shader programs: one SPIR-V module per requested stage, plus
	 * reflection of every descriptor binding (type, set, binding, count and the stages that use it).
	 *
	 * Results are cached on disk (see ShaderCache::StoreProgram). A cache entry is reused until the entry
	 * module or any module it imports changes, or until "--recompile-shaders" is passed on the command line.
	 */
	class SlangCompiler
	{
	public:
		/**
		 * @brief Compiles (or loads from cache) one program variant.
		 * @return The shader, or nullptr if the source is missing, fails to compile, or a requested stage has no entry point.
		 */
		[[nodiscard]] static Ref<Shader> Compile(const ShaderCompileOptions& options);

		/**
		 * @brief Compiles every entry point defined in a .slang file; the shader is named after the file stem.
		 */
		[[nodiscard]] static Ref<Shader> Compile(std::string_view path, bool forceCompile = false, bool disableOptimization = false);

		/**
		 * @brief Locates a shader source file, trying the path as given and relative to the shader directory
		 * from the working directory and up to two parent directories.
		 * @return The resolved absolute path, or an empty path if the file does not exist.
		 */
		[[nodiscard]] static std::filesystem::path ResolveSourcePath(const std::filesystem::path& path);

		/** @brief Releases the Slang global session. Call once at renderer shutdown. */
		static void Shutdown();

	private:
		/** @brief Creates the Shader (VkShaderModules + reflection) from a compiled or cached program. */
		[[nodiscard]] static Ref<Shader> BuildShader(const ShaderCompileOptions& options, const std::filesystem::path& sourcePath, CachedShaderProgram& program);
	};

}  // namespace SceneryEditorX

// -------------------------------------------------------

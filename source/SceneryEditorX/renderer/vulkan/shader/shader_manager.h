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
 * Created: 03/07/2026
 * -------------------------------------------------------
 */
#pragma once
#include "shader_library.h"
#include <string_view>

// -------------------------------------------------------

namespace SceneryEditorX
{
	enum class Renderer_Shader : uint8_t;

	/**
	 * @class ShaderManager
	 * @brief Process-wide access point for the active shader library, plus the renderer's built-in
	 * programs addressed by Renderer_Shader.
	 */
	class ShaderManager
	{
	public:
		[[nodiscard]] static ShaderLibrary& Get();
		[[nodiscard]] static const ShaderLibrary& GetConst() { return Get(); }

		/** @brief Forward hot-reload polling to the active shader library. */
		static void PollHotReload() noexcept { Get().PollHotReload(); }

		/**
		 * @brief Compiles (or loads from cache) the renderer's built-in programs and adds them to the library.
		 * Only the bootstrap set is built unless "--full-shaders" is on the command line. Safe to call repeatedly:
		 * programs that already exist are skipped.
		 */
		static void CreateShaders();

		/** @brief Releases every shader and the Slang compiler session. */
		static void ClearAll();

		/** @brief Built-in program for a Renderer_Shader slot, or nullptr when it is not registered or failed to compile. */
		[[nodiscard]] static Shader* GetShader(Renderer_Shader type);

		/** @brief True when the slot's program exists and compiled successfully. */
		[[nodiscard]] static bool IsShaderAvailable(Renderer_Shader type);

		/**
		 * @brief True when a Slang module (e.g. "common", "resources") was imported by at least one successfully built program.
		 */
		[[nodiscard]] static bool IsSlangModuleAvailable(std::string_view moduleName);

	private:
		static Ref<ShaderLibrary> s_Library;
	};

}  // namespace SceneryEditorX 

// -------------------------------------------------------

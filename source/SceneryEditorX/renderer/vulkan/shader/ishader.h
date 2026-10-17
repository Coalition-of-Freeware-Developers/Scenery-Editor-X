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
 * ishader.h
 * -------------------------------------------------------
 * Created: 03/07/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/utils/pointers.h>
#include <cstddef>

// -------------------------------------------------------

namespace SceneryEditorX 
{
	enum class ShaderType : uint16_t;

	/**
	 * @struct ShaderDesc
	 * @brief Metadata for a single compiled shader stage.
	 */
	struct ShaderDesc
	{
		ShaderType stage = static_cast<ShaderType>(0);
		const char* entryPoint = "main";
	};

	/**
	 * @class IShader
	 * @brief Interface for a shader object.
	 */
	class IShader : public RefCounted
	{
	public:
		~IShader() override = default;

		// Every shader should be able to report its description (type, entry point, etc.)
		[[nodiscard]] virtual const ShaderDesc& GetDesc() const = 0;

		// Used for debugging or serialization
		virtual void GetBytecode(const void** ppBytecode, size_t* pSize) const = 0;
	};

	using ShaderHandle = Ref<IShader>;

}

// -------------------------------------------------------

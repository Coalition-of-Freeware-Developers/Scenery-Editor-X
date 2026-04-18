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
 * push_constant_buffer.h
 * -------------------------------------------------------
 * Created: 16/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/core/base.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @struct PushConstantBuffer_Pass
	 * @brief Per-pass push constant data uploaded via CommandList::PushConstants().
	 * All helper setters mirror the API used in renderer_passes.cpp. Updates per pass/draw
	 * 
	 * @note - Draw_index references a Sb_DrawData entry in the bindless draw data buffer,
	 * which holds per-draw transforms and material info. 
	 * - materialIndex and isTransparent are pass-level state used by compute shaders (lighting, composition)
	 * that don't have per-draw data.
	 * - The float array carries generic per-pass parameters.
	 */
	struct PushConstantBuffer_Pass
	{
		uint32_t drawIndex     = 0;
		uint32_t materialIndex = 0;
		uint32_t isTransparent = 0;
		uint32_t padding       = 0;

		// generic per-pass parameters, laid out as 3 x float4:
		// v[0..2]  = f3_value  (e.g. light count, fog, mip level)
		// v[3]     = f2_value.x
		// v[4..6]  = f3_value2 (e.g. light index, texel size)
		// v[7]     = f2_value.y
		// v[8..11] = f4_value  (e.g. light coordinate, color)
		float v[12] = {};

		/**
		 * @brief Sets a 2-component float value in the push constant buffer.
		 * @param x The first component of the value.
		 * @param y The second component of the value.
		 */
		void SetF2Value(float x, float y)
		{
			v[3] = x;
			v[7] = y;
		}

		/**
		 * @brief Sets a 3-component float value in the push constant buffer.
		 * @param value The 3-component vector value to set.
		 */
		void SetF3Value(const Vec3& value)
		{
			v[0] = value.x;
			v[1] = value.y;
			v[2] = value.z;
		}

		/**
		 * @brief Sets a 3-component float value in the push constant buffer.
		 * @param x The first component of the value.
		 * @param y The second component of the value.
		 * @param z The third component of the value.
		 */
		void SetF3Value(float x, float y = 0.0f, float z = 0.0f)
		{
			v[0] = x;
			v[1] = y;
			v[2] = z;
		}

		/**
		 * @brief Sets a 3-component float value in the push constant buffer (second set).
		 * @param value The 3-component vector value to set.
		 */
		void SetF3Value2(const Vec3& value)
		{
			v[4] = value.x;
			v[5] = value.y;
			v[6] = value.z;
		}

		/**
		 * @brief Sets a 3-component float value in the push constant buffer (second set).
		 * @param x The first component of the value.
		 * @param y The second component of the value.
		 * @param z The third component of the value.
		 */
		void SetF3Value2(float x, float y, float z)
		{
			v[4] = x;
			v[5] = y;
			v[6] = z;
		}

		/**
		 * @brief Sets a 4-component float value in the push constant buffer.
		 * @param x The first component of the value.
		 * @param y The second component of the value.
		 * @param z The third component of the value.
		 * @param w The fourth component of the value.
		 */
		void SetF4Value(float x, float y, float z, float w)
		{
			v[8] = x;
			v[9] = y;
			v[10] = z;
			v[11] = w;
		}

		/**
		 * @brief Sets a 4-component float value in the push constant buffer.
		 * @param color The 4-component vector value to set.
		 */
		void SetF4Value(const Vec4& color)
		{
			v[8] = color.x;
			v[9] = color.y;
			v[10] = color.z;
			v[11] = color.w;
		}

		/**
		 * @brief Sets a 4-component float value in the push constant buffer.
		 * @tparam T The type of the color object.
		 * @param color The color object containing r, g, b, and a components.
		 */
		template<typename T>
		requires requires(const T& color)
		{
			color.r;
			color.g;
			color.b;
			color.a;
		}
		void SetF4Value(const T& color)
		{
			v[8] = color.r;
			v[9] = color.g;
			v[10] = color.b;
			v[11] = color.a;
		}

	};

}

// -------------------------------------------------------

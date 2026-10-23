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
 * temperature.h
 * -------------------------------------------------------
 * Created: 12/8/2025
 * -------------------------------------------------------
 */
#pragma once
#pragma once
#ifdef __cplusplus

// -----------------------------------------------------

namespace SceneryEditorX::Convert
{

	/// Temperature conversions among Celsius, Fahrenheit, Kelvin (header-only)
	inline float CToF(const float c) { return (c * 9.0f / 5.0f) + 32.0f; }
	inline float FToC(const float f) { return (f - 32.0f) * 5.0f / 9.0f; }
	inline float CToK(const float c) { return c + 273.15f; }
	inline float KToC(const float k) { return k - 273.15f; }
	inline float FToK(const float f) { return CToK(FToC(f)); }
	inline float KToF(const float k) { return CToF(KToC(k)); }

}
#endif // __cplusplus

// -----------------------------------------------------

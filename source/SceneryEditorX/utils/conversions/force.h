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
 * force.h
 * -------------------------------------------------------
 * Created: 12/8/2025
 * -------------------------------------------------------
 */
#pragma once

// -----------------------------------------------------

namespace SceneryEditorX::Convert
{

    /// Force conversions centered on newtons (N) - header-only
    namespace detail
    {
        constexpr float N_PER_LBF = 4.4482216152605f; /// exact (pound-force)
        constexpr float N_PER_KGF = 9.80665f;         /// kilogram-force (standard gravity)
    }

    inline float FromLbf(const float lbf) { return lbf * detail::N_PER_LBF; }
    inline float ToLbf(const float n)     { return n / detail::N_PER_LBF; }

    inline float FromKgf(const float kgf) { return kgf * detail::N_PER_KGF; }
    inline float ToKgf(const float n)     { return n / detail::N_PER_KGF; }

}


// -----------------------------------------------------

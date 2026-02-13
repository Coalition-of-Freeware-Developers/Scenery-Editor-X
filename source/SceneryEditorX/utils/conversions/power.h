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
 * power.h
 * -------------------------------------------------------
 * Created: 12/8/2025
 * -------------------------------------------------------
 */
#pragma once

// -----------------------------------------------------

namespace SceneryEditorX::Convert
{

    /// Power conversions centered on watts (W) - header-only
    namespace detail
    {
        constexpr float W_PER_KW = 1000.0f;
        constexpr float W_PER_MW = 1'000'000.0f;
        constexpr float W_PER_HP_MECH = 745.69987158227022f; /// mechanical hp
        constexpr float W_PER_HP_METRIC = 735.49875f;        /// metric hp (PS)
    }

    inline float FromKW(const float kw)         { return kw * detail::W_PER_KW; }
    inline float ToKW(const float w)            { return w / detail::W_PER_KW; }
    inline float FromMW(const float mw)         { return mw * detail::W_PER_MW; }
    inline float ToMW(const float w)            { return w / detail::W_PER_MW; }

    inline float FromHPMech(const float hp)     { return hp * detail::W_PER_HP_MECH; }
    inline float ToHPMech(const float w)        { return w / detail::W_PER_HP_MECH; }
    inline float FromHPMetric(const float hp)   { return hp * detail::W_PER_HP_METRIC; }
    inline float ToHPMetric(const float w)      { return w / detail::W_PER_HP_METRIC; }

}

// -----------------------------------------------------

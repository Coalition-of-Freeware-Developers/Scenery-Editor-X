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
 * math.slang
 * -------------------------------------------------------
 * Created: 07/03/2026
 * -------------------------------------------------------
 */
module math;
import constants;

float fast_sqrt(float x)
{
    return (float)(asfloat(0x1fbd1df5 + (asint(x) >> 1)));
}

float fast_length(float3 v)
{
    float length_squared = dot(v, v);
    return fast_sqrt(length_squared);
}

float fast_sin(float x)
{
    const float B = 4 / PI;
    const float C = -4 / PI2;
    const float P = 0.225;

    float y = B * x + C * x * abs(x);
    y = P * (y * abs(y) - y) + y;

    return y;
}

float fast_cos(float x)
{
    return abs(abs(x) / PI2 % 4 - 2) - 1;
}

float fast_acos(float in_x)
{
    float x = abs(in_x);
    float res = -0.156583f * x + PI_HALF;
    res *= fast_sqrt(1.0f - x);
    return (in_x >= 0) ? res : PI - res;
}

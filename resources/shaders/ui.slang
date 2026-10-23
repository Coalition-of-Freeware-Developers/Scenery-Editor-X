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
 * ui.slang
 * -------------------------------------------------------
 * Created: 14/03/2026
 * -------------------------------------------------------
 */

// Input from vertex buffer
struct VSInput
{
    float2 position : POSITION;
    float2 uv		: TEXCOORD0;
    float4 color	: COLOR0;
};

// Output from vertex shader / Input to fragment shader
struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv		: TEXCOORD0;
    float4 color	: COLOR0;
};

// Push constants for transformation
struct PushConstants
{
    float2 scale;
    float2 translate;
};

[[vk::push_constant]] PushConstants pushConstants;
[[vk::binding(0, 0)]] Sampler2D font;

[[shader("vertex")]]
VSOutput main_vs(VSInput input)
{
    VSOutput output;

    // Transform position
    output.position = float4(input.position * pushConstants.scale + pushConstants.translate, 0.0, 1.0);

    // Pass UV and color to fragment shader
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

[[shader("fragment")]]
float4 main_frag(VSOutput input) : SV_TARGET
{
    // Sample font texture and multiply by color
    float4 color = input.color * font.Sample(input.uv);
    return color;
}

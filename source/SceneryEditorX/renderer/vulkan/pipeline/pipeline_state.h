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
 * pipeline_state.h
 * -------------------------------------------------------
 * Created: 09/03/2026
 * -------------------------------------------------------
 */
#pragma once

#include <array>
#include <cstdint>
#include <map>

// ---------------------------------------------------------

namespace SceneryEditorX
{
    class Shader;
    class ImageResource;
    class RasterizerState;
    class BlendState;
    class DepthStencilState;

    /* 
     * Color sentinel – matches the Color type used by command lists.
     * Using a raw float[4] here avoids pulling in <colors.h> from this header. 
     */
    struct PipelineStateColor { float r = 0, g = 0, b = 0, a = 0; };

    // Opaque "load" sentinel colours (negative alpha = "load, don't clear").
    inline const PipelineStateColor RHI_COLOR_LOAD{ 0.0f, 0.0f, 0.0f, -1.0f };

    /**
     * @struct PipelineState
     * @brief High-level, API-agnostic descriptor for a graphics or compute pipeline.
     *
     * Passes build a PipelineState, then call CommandList::SetPipelineState() which
     * resolves or creates the underlying VkPipeline and starts the render pass.
     */
    struct PipelineState
    {
        const char* name = nullptr;

        // Shader stages – indexed by Stage enum (vertex=0, geometry=1, tess_ctrl=2, tess_eval=3, fragment=4, compute=5)
        std::map<uint32_t, Shader*> shaders;

        // Pipeline state objects (nullptr = use defaults)
        RasterizerState*   rasterizerState        = nullptr;
        BlendState*        blendState             = nullptr;
        DepthStencilState* depthStencil_State     = nullptr;

        // Render targets
        std::array<ImageResource*, MAX_RENDER_TARGET_COUNT> renderTarget_ColorTextures = {};
        ImageResource*  renderTarget_DepthTexture = nullptr;
        ImageResource*  vrsInputTexture           = nullptr;

        // Clear values (color_load.a < 0 = load; non-negative = clear to this colour)
        std::array<PipelineStateColor, MAX_RENDER_TARGET_COUNT> clearColor = {};
        float clearDepth = RHI_DEPTH_LOAD;

        // Misc flags
        bool resolutionScale = false;
    };
}

// ---------------------------------------------------------

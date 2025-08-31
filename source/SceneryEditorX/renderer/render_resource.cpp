/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* render_resource.cpp
* -------------------------------------------------------
* Created: 29/8/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/primitives.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/fonts/font.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    namespace
    {
        // Graphics States
        std::array<Ref<RasterizerState>, static_cast<uint32_t>(RasterizerState::Max)> rasterizer_states;
        std::array<Ref<DepthStencilState>, static_cast<uint32_t>(DepthStencilState::Max)> depth_stencil_states;
        std::array<Ref<BlendState>, 3> blend_states;

        // Renderer Resources
        std::array<Ref<Texture>, static_cast<uint32_t>(RenderTarget::Max)> render_targets;
        std::array<Ref<Shader>, static_cast<uint32_t>(Renderer_Shader::max)> shaders;
        std::array<Ref<Sampler>, static_cast<uint32_t>(Renderer_Sampler::Max)> samplers;
        std::array<Ref<Buffer>, static_cast<uint32_t>(Renderer_Buffer::Max)> buffers;

        // Asset Resources
        std::array<Ref<Texture>, static_cast<uint32_t>(Renderer_StandardTexture::Max)> standard_textures;
        std::array<Ref<Mesh>, static_cast<uint32_t>(PrimitiveType::MaxEnum)> standard_meshes;
        Ref<Font> standard_font;
        Ref<Material> standard_material;
    }

    Ref<Font>& Renderer::GetFont() { return standard_font; }

    Ref<Material>& Renderer::GetStandardMaterial() { return standard_material; }

}

/// -------------------------------------------------------

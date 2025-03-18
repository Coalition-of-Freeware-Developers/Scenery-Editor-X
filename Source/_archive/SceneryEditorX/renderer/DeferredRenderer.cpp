#include "../src/xpeditorpch.h"
#include "../core/AssetManager.hpp"
#include "DeferredRenderer.hpp"
#include "VK_Wrapper.h"

#include "../platform/windows/FileManager.hpp"

namespace DeferredShading
{

    struct Context
    {
        const char *presentTypes[7] = {"Light", "Albedo", "Normal", "Material", "Emission", "Depth", "All"};
        int presentType = 0;
    
        vkw::Pipeline opaquePipeline;
        vkw::Pipeline lightPipeline;
        vkw::Pipeline composePipeline;
    
        vkw::Image albedo;
        vkw::Image normal;
        vkw::Image material;
        vkw::Image emission;
        vkw::Image depth;
        vkw::Image light;
        vkw::Image compose;
    };
    
    struct ComposeConstant
    {
        int imageType;
        int lightRID;
        int albedoRID;
        int normalRID;
        int materialRID;
        int emissionRID;
        int depthRID;
    };

    Context ctx; // Deferred Renderer Context

    /**
     * @brief Creates the shaders for the deferred renderer.
     * 
     * This function initializes the light, opaque, and compose pipelines
     * by creating the necessary shaders and setting up their configurations.
     * It is important to ensure that images are created before calling this function,
     * as indicated by the critical log message if the albedo format is not set.
     */
    void CreateShaders()
    {
        if (ctx.albedo.format == 0)
        {
            LOG_CRITICAL("CREATE IMAGES BEFORE SHADERS IN DEFERRED RENDERER");
        }

        ctx.lightPipeline = vkw::CreatePipeline({
            .point = vkw::PipelinePoint::Graphics,
            .stages =
                {
                    {.stage = vkw::ShaderStage::Vertex, .path = "light.vert"},
                    {.stage = vkw::ShaderStage::Fragment, .path = "light.frag"},
                },
            .name = "Light Pipeline",
            .vertexAttributes = {},
            .colorFormats = {ctx.light.format},
            .useDepth = false,
        });

        ctx.opaquePipeline = vkw::CreatePipeline(
            {.point = vkw::PipelinePoint::Graphics,
             .stages =
                 {
                     {.stage = vkw::ShaderStage::Vertex, .path = "opaque.vert"},
                     {.stage = vkw::ShaderStage::Fragment, .path = "opaque.frag"},
                 },
             .name = "Opaque Pipeline",
             .vertexAttributes = {vkw::Format::RGB32_sfloat,
                                  vkw::Format::RGB32_sfloat,
                                  vkw::Format::RGBA32_sfloat,
                                  vkw::Format::RG32_sfloat},
             .colorFormats = {ctx.albedo.format, ctx.normal.format, ctx.material.format, ctx.emission.format},
             .useDepth = true,
             .depthFormat = {ctx.depth.format}});
        ctx.composePipeline = vkw::CreatePipeline({
            .point = vkw::PipelinePoint::Graphics,
            .stages =
                {
                    {.stage = vkw::ShaderStage::Vertex, .path = "present.vert"},
                    {.stage = vkw::ShaderStage::Fragment, .path = "present.frag"},
                },
            .name = "Present Pipeline",
            .vertexAttributes = {},
            .colorFormats = {vkw::Format::RGBA8_unorm},
            .useDepth = false,
        });
    }
    
    /**
     * @brief Creates the images required for the deferred renderer.
     * 
     * This function initializes various images such as albedo, normal, material,
     * emission, light, depth, and compose. These images are used as attachments
     * in different rendering passes. The function takes the width and height
     * of the images as parameters.
     * 
     * @param width The width of the images to be created.
     * @param height The height of the images to be created.
     */
    void CreateImages(uint32_t width, uint32_t height)
    {
        ctx.albedo = vkw::CreateImage({.width = width,
                                       .height = height,
                                       .format = vkw::Format::RGBA8_unorm,
                                       .usage = vkw::ImageUsage::ColorAttachment | vkw::ImageUsage::Sampled,
                                       .name = "Albedo Attachment"});
        ctx.normal = vkw::CreateImage({.width = width,
                                       .height = height,
                                       .format = vkw::Format::RGBA32_sfloat,
                                       .usage = vkw::ImageUsage::ColorAttachment | vkw::ImageUsage::Sampled,
                                       .name = "Normal Attachment"});
        ctx.material = vkw::CreateImage({.width = width,
                                         .height = height,
                                         .format = vkw::Format::RGBA8_unorm,
                                         .usage = vkw::ImageUsage::ColorAttachment | vkw::ImageUsage::Sampled,
                                         .name = "Material Attachment"});
        ctx.emission = vkw::CreateImage({.width = width,
                                         .height = height,
                                         .format = vkw::Format::RGBA8_unorm,
                                         .usage = vkw::ImageUsage::ColorAttachment | vkw::ImageUsage::Sampled,
                                         .name = "Emission Attachment"});
        ctx.light = vkw::CreateImage({.width = width,
                                      .height = height,
                                      .format = vkw::Format::RGBA8_unorm,
                                      .usage = vkw::ImageUsage::ColorAttachment | vkw::ImageUsage::Sampled,
                                      .name = "Light Attachment"});
        ctx.depth = vkw::CreateImage({.width = width,
                                      .height = height,
                                      .format = vkw::Format::D32_sfloat,
                                      .usage = vkw::ImageUsage::DepthAttachment | vkw::ImageUsage::Sampled,
                                      .name = "Depth Attachment"});
        ctx.compose = vkw::CreateImage({.width = width,
                                        .height = height,
                                        .format = vkw::Format::RGBA8_unorm,
                                        .usage = vkw::ImageUsage::ColorAttachment | vkw::ImageUsage::Sampled,
                                        .name = "Compose Attachment"});
    }

    /**
     * @brief Destroys the deferred renderer context.
     * 
     * This function resets the deferred renderer context by setting it to an empty state.
     * It should be called to clean up resources when the deferred renderer is no longer needed.
     */
    void Destroy()
    {
        ctx = {};
    }
    
    /**
     * @brief Renders a mesh using the deferred renderer.
     * 
     * This function issues a draw command for the specified mesh resource.
     * It retrieves the mesh from the AssetManager using the provided mesh ID and
     * uses the vertex and index buffers to render the mesh.
     * 
     * @param meshId The resource ID of the mesh to be rendered.
     */
    void RenderMesh(RID meshId)
    {
        MeshResource &mesh = AssetManager::meshes[meshId];
        vkw::CmdDrawMesh(mesh.vertexBuffer, mesh.indexBuffer, mesh.indexCount);
    }

    /**
     * @brief Begins the opaque rendering pass.
     * 
     * This function sets up the necessary barriers and begins the rendering pass
     * for opaque objects. It transitions the layout of the albedo, normal, material,
     * and emission images to be used as color attachments and the depth image to be
     * used as a depth attachment. It then binds the opaque pipeline for rendering.
     */
    void BeginOpaquePass()
    {
        std::vector<vkw::Image> attachs = {ctx.albedo, ctx.normal, ctx.material, ctx.emission};
        for (auto &attach : attachs)
        {
            vkw::CmdBarrier(attach, vkw::Layout::ColorAttachment);
        }
        vkw::CmdBarrier(ctx.depth, vkw::Layout::DepthAttachment);
        vkw::CmdBeginRendering(attachs, ctx.depth);
        vkw::CmdBindPipeline(ctx.opaquePipeline);
    }

    /**
     * @brief Ends the current rendering pass.
     * 
     * This function ends the current rendering pass by issuing the appropriate
     * command to stop rendering. It should be called after all draw commands
     * for the current pass have been issued.
     */
    void EndPass()
    {
        vkw::CmdEndRendering();
    }
    
    /**
     * @brief Executes the light pass in the deferred renderer.
     * 
     * This function sets up the necessary barriers and begins the rendering pass
     * for lighting. It transitions the layout of the albedo, normal, material,
     * emission, and depth images to be used as shader read inputs and the light
     * image to be used as a color attachment. It then binds the light pipeline
     * and issues a draw command with the provided light constants.
     * 
     * @param constants The light constants to be used in the light pass.
     */
    void LightPass(LightConstants constants)
    {
        std::vector<vkw::Image> attachs = {ctx.albedo, ctx.normal, ctx.material, ctx.emission};
        for (auto &attach : attachs)
        {
            vkw::CmdBarrier(attach, vkw::Layout::ShaderRead);
        }
        vkw::CmdBarrier(ctx.depth, vkw::Layout::DepthRead);
        vkw::CmdBarrier(ctx.light, vkw::Layout::ColorAttachment);
    
        constants.albedoRID = ctx.albedo.RID();
        constants.normalRID = ctx.normal.RID();
        constants.materialRID = ctx.material.RID();
        constants.emissionRID = ctx.emission.RID();
        constants.depthRID = ctx.depth.RID();
    
        vkw::CmdBeginRendering({ctx.light}, {});
        vkw::CmdBindPipeline(ctx.lightPipeline);
        vkw::CmdPushConstants(&constants, sizeof(constants));
        vkw::CmdDrawPassThrough();
        vkw::CmdEndRendering();
    }

    /**
     * @brief Executes the compose pass in the deferred renderer.
     * 
     * This function sets up the necessary barriers and begins the rendering pass
     * for composing the final image. It transitions the layout of the light image
     * to be used as a shader read input and the compose image to be used as a color
     * attachment. It then binds the compose pipeline and issues a draw command with
     * the provided compose constants.
     */
    void ComposePass()
    {
        vkw::CmdBarrier(ctx.light, vkw::Layout::ShaderRead);
        vkw::CmdBarrier(ctx.compose, vkw::Layout::ColorAttachment);
    
        ComposeConstant constants;
        constants.lightRID = ctx.light.RID();
        constants.albedoRID = ctx.albedo.RID();
        constants.normalRID = ctx.normal.RID();
        constants.materialRID = ctx.material.RID();
        constants.emissionRID = ctx.emission.RID();
        constants.depthRID = ctx.depth.RID();
        constants.imageType = ctx.presentType;
    
        vkw::CmdBeginRendering({ctx.compose});
        vkw::CmdBindPipeline(ctx.composePipeline);
        vkw::CmdPushConstants(&constants, sizeof(constants));
        vkw::CmdDrawPassThrough();
        vkw::CmdEndRendering();
    
        vkw::CmdBarrier(ctx.compose, vkw::Layout::ShaderRead);
    }

    /**
     * @brief Begins the present pass.
     * 
     * This function starts the present pass by issuing the command to begin presenting.
     * It should be called before any present operations are performed.
     */
    void BeginPresentPass()
    {
        vkw::CmdBeginPresent();
    }
    
    /**
     * @brief Ends the present pass.
     * 
     * This function ends the present pass by issuing the command to end presenting.
     * It should be called after all present operations are completed.
     */
    void EndPresentPass()
    {
        vkw::CmdEndPresent();
    }

    /**
     * @brief Renders the ImGui interface for the deferred renderer.
     * 
     * This function creates an ImGui window for the deferred renderer settings.
     * It provides a combo box to select the present type from the available options.
     * 
     * @param numFrame The current frame number.
     */
    void OnImgui(int numFrame)
    {
        if (ImGui::Begin("Deferred Renderer"))
        {
            if (ImGui::BeginCombo("Present", ctx.presentTypes[ctx.presentType]))
            {
                for (int i = 0; i < COUNT_OF(ctx.presentTypes); i++)
                {
                    bool selected = ctx.presentType == i;
                    if (ImGui::Selectable(ctx.presentTypes[i], &selected))
                    {
                        ctx.presentType = i;
                    }
                }
                ImGui::EndCombo();
            }
        }
        ImGui::End();
    }

    /**
     * @brief Renders the ImGui interface for the viewport.
     * 
     * This function creates an ImGui image using the compose image's ImGui resource ID
     * and sets up the ImGuizmo draw list and rectangle for gizmo manipulation.
     */
    void ViewportOnImGui()
    {
        ImGui::Image(ctx.compose.ImGuiRID(), ImGui::GetWindowSize());
    
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x,
                          ImGui::GetWindowPos().y,
                          ImGui::GetWindowSize().x,
                          ImGui::GetWindowSize().y);
    }

}

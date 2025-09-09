/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* render_pass.h
* -------------------------------------------------------
* Created: 18/8/2025
* -------------------------------------------------------
*/
#pragma once
//#include "command_manager.h"
//#include "texture.h"
//#include "buffers/storage_buffer.h"
//#include "buffers/uniform_buffer.h"
//#include "vulkan/vk_enums.h"
//#include "vulkan/vk_image_view.h"

/// -------------------------------------------------------
/*
namespace SceneryEditorX
{
	
    struct RenderPassInput
    {
        ResourceType type = ResourceType::None;
        std::vector<Ref<RefCounted>> input;

        RenderPassInput() = default;
    	RenderPassInput(const Ref<UniformBuffer>& uniformBuffer) : type(ResourceType::UniformBuffer) { input.reserve(1); input.push_back(uniformBuffer.As<RefCounted>()); }
    	RenderPassInput(const Ref<UniformBufferSet>& uniformBufferSet) : type(ResourceType::UniformSet) { input.reserve(1); input.push_back(uniformBufferSet.As<RefCounted>()); }
    	RenderPassInput(const Ref<StorageBuffer>& storageBuffer) : type(ResourceType::StorageBuffer) { input.reserve(1); input.push_back(storageBuffer.As<RefCounted>()); }
    	RenderPassInput(const Ref<StorageBufferSet>& storageBufferSet) : type(ResourceType::StorageSet) { input.reserve(1); input.push_back(storageBufferSet.As<RefCounted>()); }
    	RenderPassInput(const Ref<Texture2D>& texture) : type(ResourceType::Texture2D) { input.reserve(1); input.push_back(texture.As<RefCounted>()); }
    	RenderPassInput(const Ref<TextureCube>& texture) : type(ResourceType::TextureCube) { input.reserve(1); input.push_back(texture.As<RefCounted>()); }
    	RenderPassInput(const Ref<Image2D>& image) : type(ResourceType::Image2D) { input.reserve(1); input.push_back(image.As<RefCounted>()); }

    	void Set(const Ref<UniformBuffer>& uniformBuffer, uint32_t index = 0) { type = ResourceType::UniformBuffer; if (input.size() <= index) input.resize(index+1); input[index] = uniformBuffer.As<RefCounted>(); }
    	void Set(const Ref<UniformBufferSet>& uniformBufferSet, uint32_t index = 0) { type = ResourceType::UniformSet; if (input.size() <= index) input.resize(index+1); input[index] = uniformBufferSet.As<RefCounted>(); }
    	void Set(const Ref<StorageBuffer>& storageBuffer, uint32_t index = 0) { type = ResourceType::StorageBuffer; if (input.size() <= index) input.resize(index+1); input[index] = storageBuffer.As<RefCounted>(); }
    	void Set(const Ref<StorageBufferSet>& storageBufferSet, uint32_t index = 0) { type = ResourceType::StorageSet; if (input.size() <= index) input.resize(index+1); input[index] = storageBufferSet.As<RefCounted>(); }
    	void Set(const Ref<Texture2D>& texture, uint32_t index = 0) { type = ResourceType::Texture2D; if (input.size() <= index) input.resize(index+1); input[index] = texture.As<RefCounted>(); }
    	void Set(const Ref<TextureCube>& texture, uint32_t index = 0) { type = ResourceType::TextureCube; if (input.size() <= index) input.resize(index+1); input[index] = texture.As<RefCounted>(); }
    	void Set(const Ref<Image2D>& image, uint32_t index = 0) { type = ResourceType::Image2D; if (input.size() <= index) input.resize(index+1); input[index] = image.As<RefCounted>(); }
    	void Set(const Ref<ImageView>& image, uint32_t index = 0) { type = ResourceType::Image2D; if (input.size() <= index) input.resize(index+1); input[index] = image.As<RefCounted>(); }
    };

    struct RenderPassInputDeclaration
    {
        ResourceInputType type = ResourceInputType::None;
        uint32_t set = 0;
        uint32_t binding = 0;
        uint32_t count = 0;
        std::string name;
    };

    class RenderPass : public RefCounted
    {
    public:
		RenderPass() = delete;
		virtual ~RenderPass() override = default;

        // passes - core
        static void ProduceFrame(CommandManager* cmd_list_graphics_present, CommandManager* cmd_list_compute);
        static void Pass_VariableRateShading(CommandManager* cmd_list);
        static void Pass_ShadowMaps(CommandManager* cmd_list);
        static void Pass_Occlusion(CommandManager* cmd_list);
        static void Pass_Depth_Prepass(CommandManager* cmd_list);
        static void Pass_GBuffer(CommandManager* cmd_list, const bool is_transparent_pass);
        static void Pass_ScreenSpaceAmbientOcclusion(CommandManager* cmd_list);
        static void Pass_TransparencyReflectionRefraction(CommandManager* cmd_list);
        static void Pass_ScreenSpaceShadows(CommandManager* cmd_list);
        static void Pass_Skysphere(CommandManager* cmd_list);

        // passes - lighting
        static void Pass_Light(CommandManager* cmd_list, const bool is_transparent_pass);
        static void Pass_Light_Composition(CommandManager* cmd_list, const bool is_transparent_pass);
        static void Pass_Light_ImageBased(CommandManager* cmd_list);
        static void Pass_Lut_BrdfSpecular(CommandManager* cmd_list);
        static void Pass_Lut_AtmosphericScattering(CommandManager* cmd_list);

        // passes - debug/editor
        static void Pass_Grid(CommandManager* cmd_list, Texture* tex_out);
        static void Pass_Lines(CommandManager* cmd_list, Texture* tex_out);
        static void Pass_Outline(CommandManager* cmd_list, Texture* tex_out);
        static void Pass_Icons(CommandManager* cmd_list, Texture* tex_out);
        static void Pass_Text(CommandManager* cmd_list, Texture* tex_out);

        // passes - post-process
        static void Pass_PostProcess(CommandManager* cmd_list);
        static void Pass_Output(CommandManager* cmd_list, Texture* tex_in, Texture* tex_out);
        static void Pass_Fxaa(CommandManager* cmd_list, Texture* tex_in, Texture* tex_out);
        static void Pass_FilmGrain(CommandManager* cmd_list, Texture* tex_in, Texture* tex_out);
        static void Pass_Vhs(CommandManager* cmd_list, Texture* tex_in, Texture* tex_out);
        static void Pass_ChromaticAberration(CommandManager* cmd_list, Texture* tex_in, Texture* tex_out);
        static void Pass_MotionBlur(CommandManager* cmd_list, Texture* tex_in, Texture* tex_out);
        static void Pass_DepthOfField(CommandManager* cmd_list, Texture* tex_in, Texture* tex_out);
        static void Pass_Bloom(CommandManager* cmd_list, Texture* tex_in, Texture* tex_out);
        static void Pass_Sharpening(CommandManager* cmd_list, Texture* tex_in, Texture* tex_out);
        static void Pass_Dithering(CommandManager* cmd_list, Texture* tex_in, Texture* tex_out);
        static void Pass_Upscale(CommandManager* cmd_list);

        // passes - utility
        static void Pass_Blit(CommandManager* cmd_list, Texture* tex_in, Texture* tex_out);
        static void Pass_Downscale(CommandManager* cmd_list, Texture* tex, const DownsampleFilter filter);
        static void Pass_Blur(CommandManager* cmd_list, Texture* tex_in, const bool bilateral, const float radius, const uint32_t mip = rhi_all_mips);

    };

}
*/

/// -------------------------------------------------------

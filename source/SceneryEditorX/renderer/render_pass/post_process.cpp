/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* post_process.cpp
* -------------------------------------------------------
* Created: 29/8/2025
* -------------------------------------------------------
*/
//#include <SceneryEditorX/renderer/command_manager.h>
//#include <SceneryEditorX/renderer/render_pass.h>
//#include <SceneryEditorX/renderer/texture.h>

/// -------------------------------------------------------

/*
namespace SceneryEditorX
{
	
    void RenderPass::Pass_PostProcess(CommandManager *cmd_list)
    {
        // acquire render targets
        Texture* rt_frame_output         = GetRenderTarget(Renderer_RenderTarget::frame_output);
        Texture* rt_frame_output_scratch = GetRenderTarget(Renderer_RenderTarget::frame_output_2);

        cmd_list->BeginMarker("post_proccess");

        // macros which allows us to keep track of which texture is an input/output for each pass
        bool swap_output = true;
        #define get_output_in  swap_output ? rt_frame_output_scratch : rt_frame_output
        #define get_output_out swap_output ? rt_frame_output : rt_frame_output_scratch

        // depth of field
        if (GetOption<bool>(Renderer_Option::DepthOfField))
        {
            swap_output = !swap_output;
            Pass_DepthOfField(cmd_list, get_output_in, get_output_out);
        }
        
        // motion blur
        if (GetOption<bool>(Renderer_Option::MotionBlur))
        {
            swap_output = !swap_output;
            Pass_MotionBlur(cmd_list, get_output_in, get_output_out);
        }
        
        // bloom
        if (GetOption<bool>(Renderer_Option::Bloom))
        {
            swap_output = !swap_output;
            Pass_Bloom(cmd_list, get_output_in, get_output_out);
        }

        // tone-mapping & gamma correction
        {
            swap_output = !swap_output;
            Pass_Output(cmd_list, get_output_in, get_output_out);
        }

        // dithering
        if (GetOption<bool>(Renderer_Option::Dithering))
        {
            swap_output = !swap_output;
            Pass_Dithering(cmd_list, get_output_in, get_output_out);
        }

        // sharpening
        if (GetOption<bool>(Renderer_Option::Sharpness) && GetOption<Renderer_Upsampling>(Renderer_Option::Upsampling) != Renderer_Upsampling::Fsr3)
        {
            swap_output = !swap_output;
            Pass_Sharpening(cmd_list, get_output_in, get_output_out);
        }
        
        // FXAA
        Renderer_Antialiasing antialiasing  = GetOption<Renderer_Antialiasing>(Renderer_Option::Antialiasing);
        bool fxaa_enabled                   = antialiasing == Renderer_Antialiasing::Fxaa || antialiasing == Renderer_Antialiasing::TaaFxaa;
        if (fxaa_enabled)
        {
            swap_output = !swap_output;
            Pass_Fxaa(cmd_list, get_output_in, get_output_out);
        }

        // film grain
        if (GetOption<bool>(Renderer_Option::FilmGrain))
        {
            swap_output = !swap_output;
            Pass_FilmGrain(cmd_list, get_output_in, get_output_out);
        }

        // chromatic aberration
        if (GetOption<bool>(Renderer_Option::ChromaticAberration))
        {
            swap_output = !swap_output;
            Pass_ChromaticAberration(cmd_list, get_output_in, get_output_out);
        }

        // vhs
        if (GetOption<bool>(Renderer_Option::Vhs))
        {
            swap_output = !swap_output;
            Pass_Vhs(cmd_list, get_output_in, get_output_out);
        }

        // if the last written texture is not the output one, then make sure it is
        if (!swap_output)
        {
            cmd_list->Copy(rt_frame_output_scratch, rt_frame_output, false);
        }

        // editor
        Pass_Grid(cmd_list, rt_frame_output);
        Pass_Lines(cmd_list, rt_frame_output);
        Pass_Outline(cmd_list, rt_frame_output);
        Pass_Icons(cmd_list, rt_frame_output);

        cmd_list->EndMarker();
    }

}
*/

/// -------------------------------------------------------

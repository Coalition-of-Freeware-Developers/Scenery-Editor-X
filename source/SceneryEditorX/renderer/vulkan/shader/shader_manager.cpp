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
 * shader_manager.cpp
 * -------------------------------------------------------
 * Created: 03/07/2026
 * -------------------------------------------------------
 */
#include "shader_manager.h"
#include "slang_compiler.h"
#include <algorithm>
#include <array>
#include <mutex>
#include <set>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/renderer/renderer_declarations.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		/**
		 * @struct ShaderRegistration
		 * @brief One built-in program: a Slang source, the stages to extract and the defines selecting the variant.
		 */
		struct ShaderRegistration
		{
			Renderer_Shader id;
			const char* name;						// Library key, matches the Renderer_Shader enumerator
			const char* source;						// Relative to resources/shaders
			std::vector<ShaderType> stages;
			std::vector<ShaderMacro> macros = {};
			VertexType vertexType = VertexType::MaxEnum;
			bool bootstrap = false;					// Built at startup even without --full-shaders
		};

		constexpr ShaderType VS = ShaderType::Vertex;
		constexpr ShaderType TCS = ShaderType::TessellationControl;
		constexpr ShaderType TES = ShaderType::TessellationEvaluation;
		constexpr ShaderType FS = ShaderType::Fragment;
		constexpr ShaderType CS = ShaderType::Compute;

		[[nodiscard]] const std::vector<ShaderRegistration>& GetRegistrations()
		{
			// Variants of one source (bloom, atmosphere LUTs, probe filters, blur, cloud_noise, indirect draws) are selected with
			// preprocessor defines; each define set is compiled into its own program.
			static const std::vector<ShaderRegistration> registrations = {
				// geometry
				{ Renderer_Shader::depth_prepass,          "depth_prepass",          "depth_prepass.slang", {VS, FS}, {}, VertexType::PositionUvNormalTangent, true },
				{ Renderer_Shader::depth_prepass_indirect, "depth_prepass_indirect", "depth_prepass.slang", {VS},     {{"INDIRECT_DRAW"}} },
				{ Renderer_Shader::depth_light,            "depth_light",            "depth_light.slang",   {VS, FS}, {}, VertexType::PositionUvNormalTangent },
				{ Renderer_Shader::gbuffer,                "gbuffer",                "gbuffer.slang",       {VS, FS}, {}, VertexType::PositionUvNormalTangent, true },
				{ Renderer_Shader::gbuffer_indirect,       "gbuffer_indirect",       "gbuffer.slang",       {VS, FS}, {{"INDIRECT_DRAW"}} },
				{ Renderer_Shader::indirect_cull,          "indirect_cull",          "indirect_cull.slang", {CS} },
				// GPU scene: culling / LOD compute and the instanced indirect variants (vertices pulled, no vertex input)
				{ Renderer_Shader::gpu_cull,               "gpu_cull",               "gpu_cull.slang",      {CS},     {},                 VertexType::MaxEnum, true },
				{ Renderer_Shader::gpu_cull_late,          "gpu_cull_late",          "gpu_cull.slang",      {CS},     {{"GPU_CULL_LATE"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::depth_prepass_gpu,      "depth_prepass_gpu",      "depth_prepass.slang", {VS, FS}, {{"GPU_SCENE"}},    VertexType::MaxEnum, true },
				{ Renderer_Shader::gbuffer_gpu,            "gbuffer_gpu",            "gbuffer.slang",       {VS, FS}, {{"GPU_SCENE"}},    VertexType::MaxEnum, true },
				// sun shadows (plan Phase 7): GPU-scene instances into each cascade, the casters drawn into shadow_csm
				{ Renderer_Shader::gpu_cull_shadow,        "gpu_cull_shadow",        "gpu_cull.slang",      {CS},     {{"GPU_CULL_SHADOW"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::depth_light_csm,        "depth_light_csm",        "depth_light.slang",   {VS, FS}, {{"CSM"}},          VertexType::PositionUvNormalTangent, true },
				{ Renderer_Shader::depth_light_csm_gpu,    "depth_light_csm_gpu",    "depth_light.slang",   {VS, FS}, {{"CSM"}, {"GPU_SCENE"}}, VertexType::MaxEnum, true },

				// editor overlays
				{ Renderer_Shader::grid,    "grid",    "grid.slang",    {VS, FS},     {}, VertexType::PositionUvNormalTangent, true },
				{ Renderer_Shader::line,    "line",    "line.slang",    {VS, FS},     {}, VertexType::PositionColor, true },
				{ Renderer_Shader::outline, "outline", "outline.slang", {VS, FS, CS}, {}, VertexType::PositionUvNormalTangent },
				{ Renderer_Shader::font,    "font",    "font.slang",    {VS, FS},     {}, VertexType::PositionUv, true },
				{ Renderer_Shader::icon,    "icon",    "icons.slang",   {CS} },
				{ Renderer_Shader::selection_outline, "selection_outline", "selection_outline.slang", {CS}, {}, VertexType::MaxEnum, true }, // ScenePicking

				// forward passes over the composed frame (blended surfaces, light billboards)
				{ Renderer_Shader::forward_transparent, "forward_transparent", "forward_transparent.slang", {VS, FS}, {}, VertexType::PositionUvNormalTangent, true },
				{ Renderer_Shader::light_billboard,     "light_billboard",     "light_billboard.slang",     {VS, FS}, {}, VertexType::PositionUvNormalTangent, true },

				// lighting
				{ Renderer_Shader::light,                                "light",                                "light.slang",             {CS}, {}, VertexType::MaxEnum, true },
				{ Renderer_Shader::light_composition,                    "light_composition",                    "light_composition.slang", {CS}, {}, VertexType::MaxEnum, true },
				{ Renderer_Shader::light_integration_brdf_specular_lut,  "light_integration_brdf_specular_lut",  "light_integration.slang", {CS}, {}, VertexType::MaxEnum, true },
				{ Renderer_Shader::light_integration_environment_filter, "light_integration_environment_filter", "light_integration.slang", {CS}, {}, VertexType::MaxEnum, true },
				{ Renderer_Shader::ssao,                                 "ssao",                                 "ssao.slang",              {CS}, {}, VertexType::MaxEnum, true }, // GTAO, the AO fallback

				// the min / max depth pyramid (plan Phase 9; XP12: z_minmax), also occlusion culling's Hi-Z, and screen-space
				// reflections (XP12: ssr_deferred) with the history they read
				{ Renderer_Shader::z_minmax_depth,   "z_minmax_depth",   "z_minmax.slang",  {CS}, {{"ZMINMAX_FROM_DEPTH"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::z_minmax,         "z_minmax",         "z_minmax.slang",  {CS}, {},                       VertexType::MaxEnum, true },
				{ Renderer_Shader::ssr_trace,        "ssr_trace",        "ssr_trace.slang", {CS}, {{"SSR_TRACE"}},          VertexType::MaxEnum, true },
				{ Renderer_Shader::ssr_history_copy, "ssr_history_copy", "ssr_trace.slang", {CS}, {{"SSR_HISTORY_COPY"}},   VertexType::MaxEnum, true },
				{ Renderer_Shader::ssr_history_mip,  "ssr_history_mip",  "ssr_trace.slang", {CS}, {{"SSR_HISTORY_MIP"}},    VertexType::MaxEnum, true },

				// ambient occlusion (plan Phase 8; XP12: cacao): AMD FidelityFX CACAO's kernels, X-Plane's qualities (renderer/cacao.h)
				{ Renderer_Shader::cacao_prepare_depths,    "cacao_prepare_depths",     "cacao.slang", {CS}, {{"CACAO_PREPARE_DEPTHS"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::cacao_prepare_normals,   "cacao_prepare_normals",    "cacao.slang", {CS}, {{"CACAO_PREPARE_NORMALS"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::cacao_generate_q1,       "cacao_generate_q1",        "cacao.slang", {CS}, {{"CACAO_GENERATE"}, {"CACAO_QUALITY", "1"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::cacao_generate_q2,       "cacao_generate_q2",        "cacao.slang", {CS}, {{"CACAO_GENERATE"}, {"CACAO_QUALITY", "2"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::cacao_generate_q3,       "cacao_generate_q3",        "cacao.slang", {CS}, {{"CACAO_GENERATE"}, {"CACAO_QUALITY", "3"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::cacao_generate_q3_base,  "cacao_generate_q3_base",   "cacao.slang", {CS}, {{"CACAO_GENERATE_BASE"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::cacao_importance_map,    "cacao_importance_map",     "cacao.slang", {CS}, {{"CACAO_IMPORTANCE_MAP"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::cacao_importance_map_a,  "cacao_importance_map_a",   "cacao.slang", {CS}, {{"CACAO_IMPORTANCE_MAP_A"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::cacao_importance_map_b,  "cacao_importance_map_b",   "cacao.slang", {CS}, {{"CACAO_IMPORTANCE_MAP_B"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::cacao_blur,              "cacao_blur",               "cacao.slang", {CS}, {{"CACAO_BLUR"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::cacao_apply,             "cacao_apply",              "cacao.slang", {CS}, {{"CACAO_APPLY"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::cacao_upscale,           "cacao_upscale",            "cacao.slang", {CS}, {{"CACAO_UPSCALE"}, {"CACAO_SMART", "0"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::cacao_upscale_smart,     "cacao_upscale_smart",      "cacao.slang", {CS}, {{"CACAO_UPSCALE"}, {"CACAO_SMART", "1"}}, VertexType::MaxEnum, true },

				// water (plan Phase 11; XP12: ocean_spectrum, compute_fft, ocean_bake_textures, ocean_shading): the FFT ocean's
				// cascades and the tessellated surface
				{ Renderer_Shader::ocean_spectrum, "ocean_spectrum", "ocean.slang", {CS}, {{"OCEAN_SPECTRUM"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::ocean_evolve,   "ocean_evolve",   "ocean.slang", {CS}, {{"OCEAN_EVOLVE"}},   VertexType::MaxEnum, true },
				{ Renderer_Shader::ocean_fft,      "ocean_fft",      "ocean.slang", {CS}, {{"OCEAN_FFT"}},      VertexType::MaxEnum, true },
				{ Renderer_Shader::ocean_bake,     "ocean_bake",     "ocean.slang", {CS}, {{"OCEAN_BAKE"}},     VertexType::MaxEnum, true },
				{ Renderer_Shader::ocean_mip,      "ocean_mip",      "ocean.slang", {CS}, {{"OCEAN_MIP"}},      VertexType::MaxEnum, true },
				{ Renderer_Shader::water,          "water",          "water.slang", {VS, TCS, TES, FS}, {}, VertexType::PositionUvNormalTangent, true },

				// the atmosphere (plan Phase 6), the sky, environment probes
				{ Renderer_Shader::atmosphere_transmittance_lut, "atmosphere_transmittance_lut", "atmosphere_luts.slang",    {CS}, {{"TRANSMITTANCE"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::atmosphere_multiscatter_lut,  "atmosphere_multiscatter_lut",  "atmosphere_luts.slang",    {CS}, {{"MULTISCATTER"}},  VertexType::MaxEnum, true },
				{ Renderer_Shader::sky_view_lut,                 "sky_view_lut",                 "sky_view_lut.slang",       {CS}, {},                  VertexType::MaxEnum, true },
				{ Renderer_Shader::sky_ambient_lut,              "sky_ambient_lut",              "sky_ambient_lut.slang",    {CS}, {},                  VertexType::MaxEnum, true },
				{ Renderer_Shader::aerial_perspective,           "aerial_perspective",           "aerial_perspective.slang", {CS}, {},                  VertexType::MaxEnum, true },
				{ Renderer_Shader::sky,                          "sky",                          "sky.slang",                {CS}, {},                  VertexType::MaxEnum, true },
				{ Renderer_Shader::cube_filter_capture,          "cube_filter_capture",          "cube_filter.slang",        {CS}, {{"CAPTURE"}},       VertexType::MaxEnum, true },
				{ Renderer_Shader::cube_filter_ggx,              "cube_filter_ggx",              "cube_filter.slang",        {CS}, {{"FILTER"}},        VertexType::MaxEnum, true },
				{ Renderer_Shader::cube_filter_irradiance,       "cube_filter_irradiance",       "cube_filter.slang",        {CS}, {{"IRRADIANCE"}},    VertexType::MaxEnum, true },
				{ Renderer_Shader::cloud_noise_shape,           "cloud_noise_shape",           "cloud_noise.slang",   {CS}, {{"SHAPE_NOISE"}} },
				{ Renderer_Shader::cloud_noise_detail,          "cloud_noise_detail",          "cloud_noise.slang",   {CS}, {{"DETAIL_NOISE"}} },
				{ Renderer_Shader::cloud_shadow,                "cloud_shadow",                "cloud_shadows.slang", {CS}, {}, VertexType::MaxEnum, true }, // cloud shadows (plan TASK-055)

				// post-processing
				{ Renderer_Shader::blur_gaussian,            "blur_gaussian",            "blur.slang",   {CS} },
				{ Renderer_Shader::blur_gaussian_bilateral,  "blur_gaussian_bilateral",  "blur.slang",   {CS}, {{"PASS_BLUR_GAUSSIAN_BILATERAL"}} },
				{ Renderer_Shader::bloom_luminance,          "bloom_luminance",          "bloom.slang",  {CS}, {{"LUMINANCE"}},          VertexType::MaxEnum, true },
				{ Renderer_Shader::bloom_downsample,         "bloom_downsample",         "bloom.slang",  {CS}, {{"DOWNSAMPLE"}},         VertexType::MaxEnum, true },
				{ Renderer_Shader::bloom_upsample_blend_mip, "bloom_upsample_blend_mip", "bloom.slang",  {CS}, {{"UPSAMPLE_BLEND_MIP"}}, VertexType::MaxEnum, true },
				{ Renderer_Shader::bloom_blend_frame,        "bloom_blend_frame",        "bloom.slang",  {CS}, {{"BLEND_FRAME"}} },
				{ Renderer_Shader::dithering,                "dithering",                "dither.slang", {CS} },
				{ Renderer_Shader::blit,                     "blit",                     "blit.slang",   {CS}, {}, VertexType::MaxEnum, true },
				{ Renderer_Shader::reference_overlay,        "reference_overlay",        "reference_overlay.slang", {CS}, {}, VertexType::MaxEnum, true }, // viewport REFERENCE menu
				{ Renderer_Shader::tonemap,                  "tonemap",                  "tonemap.slang",    {CS}, {}, VertexType::MaxEnum, true }, // output transform
				{ Renderer_Shader::debug_view,               "debug_view",               "debug_view.slang", {CS}, {}, VertexType::MaxEnum, true }, // RENDERER panel DEBUG VIEW

				// exposure and the output chain (plan Phase 5): HDR clean-up, metering, local tone mapping, bloom, AA, upscaling
				{ Renderer_Shader::fix_hdr,                    "fix_hdr",                    "fix_hdr.slang",             {CS}, {},                VertexType::MaxEnum, true },
				{ Renderer_Shader::luminance_histogram,        "luminance_histogram",        "luminance_histogram.slang", {CS}, {},                VertexType::MaxEnum, true },
				{ Renderer_Shader::exposure_adapt,             "exposure_adapt",             "exposure_adapt.slang",      {CS}, {},                VertexType::MaxEnum, true },
				{ Renderer_Shader::exposure_fusion_exposures,  "exposure_fusion_exposures",  "exposure_fusion.slang",     {CS}, {{"EXPOSURES"}},   VertexType::MaxEnum, true },
				{ Renderer_Shader::exposure_fusion_downsample, "exposure_fusion_downsample", "exposure_fusion.slang",     {CS}, {{"DOWNSAMPLE"}},  VertexType::MaxEnum, true },
				{ Renderer_Shader::exposure_fusion_collapse,   "exposure_fusion_collapse",   "exposure_fusion.slang",     {CS}, {{"COLLAPSE"}},    VertexType::MaxEnum, true },
				{ Renderer_Shader::exposure_fusion_guide,      "exposure_fusion_guide",      "exposure_fusion.slang",     {CS}, {{"GUIDE"}},       VertexType::MaxEnum, true },
				{ Renderer_Shader::fxaa,                       "fxaa",                       "fxaa.slang",                {CS}, {},                VertexType::MaxEnum, true },
				{ Renderer_Shader::fsr1_easu,                  "fsr1_easu",                  "fsr1_easu.slang",           {CS}, {},                VertexType::MaxEnum, true },
				{ Renderer_Shader::fsr1_rcas,                  "fsr1_rcas",                  "fsr1_rcas.slang",           {CS}, {},                VertexType::MaxEnum, true },

				// Not registered: ffx_spd_* (scale.slang has no entry point yet), sss_bend (sss_blend.slang disabled),
				// tessellation, depth_of_field and the particle / texture-compression passes.
			};

			return registrations;
		}

		std::mutex s_RegistryMutex;
		std::array<Ref<Shader>, static_cast<size_t>(Renderer_Shader::MaxEnum)> s_RendererShaders;
		std::set<std::string, std::less<>> s_AvailableModules;

		[[nodiscard]] bool IsFullShaderSetRequested()
		{
			const PlatformContext* context = Application::Get().GetPlatformContext();
			if (!context)
				return false;

			return std::ranges::any_of(context->GetCommandLineArgs(), [](const std::string& arg)
			{
				return arg == "--full-shaders";
			});
		}
	}

	Ref<ShaderLibrary> ShaderManager::s_Library = CreateRef<ShaderLibrary>();

	ShaderLibrary& ShaderManager::Get()
	{
		if (!s_Library)
			s_Library = CreateRef<ShaderLibrary>();
		return *s_Library;
	}

	void ShaderManager::CreateShaders()
	{
		const bool fullSet = IsFullShaderSetRequested();
		if (!fullSet)
			SEDX_CORE_WARN_TAG("ShaderManager", "Building the bootstrap shader set only (pass --full-shaders to build every registered program)");

		uint32_t built = 0;
		uint32_t failed = 0;
		for (const ShaderRegistration& registration : GetRegistrations())
		{
			if (!fullSet && !registration.bootstrap)
				continue;

			const size_t slot = static_cast<size_t>(registration.id);
			{
				std::scoped_lock lock(s_RegistryMutex);
				if (s_RendererShaders[slot])
					continue;
			}

			ShaderCompileOptions options;
			options.name = registration.name;
			options.sourcePath = registration.source;
			options.stages = registration.stages;
			options.macros = registration.macros;
			options.vertexType = registration.vertexType;

			// Compile outside the lock so the render thread can keep querying slots meanwhile.
			Ref<Shader> shader = SlangCompiler::Compile(options);
			if (!shader || !shader->IsCompiled())
			{
				SEDX_CORE_ERROR_TAG("ShaderManager", "Shader '{}' ({}) failed to build", registration.name, registration.source);
				++failed;
				continue;
			}

			std::scoped_lock lock(s_RegistryMutex);
			for (const std::filesystem::path& dependency : shader->GetSourceDependencies())
				s_AvailableModules.insert(dependency.stem().string());

			ShaderLibrary& library = Get();
			if (!library.Contains(shader->GetName()))
				library.Add(shader);

			s_RendererShaders[slot] = shader;
			++built;
		}

		SEDX_CORE_INFO_TAG("ShaderManager", "Shader build finished: {} built, {} failed", built, failed);
	}

	void ShaderManager::ClearAll()
	{
		{
			std::scoped_lock lock(s_RegistryMutex);
			s_RendererShaders.fill(nullptr);
			s_AvailableModules.clear();
			if (s_Library)
				s_Library->Clear();
		}

		SlangCompiler::Shutdown();
	}

	Shader* ShaderManager::GetShader(Renderer_Shader type)
	{
		const size_t slot = static_cast<size_t>(type);
		if (slot >= s_RendererShaders.size())
			return nullptr;

		std::scoped_lock lock(s_RegistryMutex);
		return s_RendererShaders[slot].Get();
	}

	bool ShaderManager::IsShaderAvailable(Renderer_Shader type)
	{
		const Shader* shader = GetShader(type);
		return shader && shader->IsCompiled();
	}

	bool ShaderManager::IsSlangModuleAvailable(std::string_view moduleName)
	{
		std::scoped_lock lock(s_RegistryMutex);
		return s_AvailableModules.contains(moduleName);
	}

}  // namespace SceneryEditorX

// -------------------------------------------------------

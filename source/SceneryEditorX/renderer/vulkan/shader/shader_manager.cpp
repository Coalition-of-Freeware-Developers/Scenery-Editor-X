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
		constexpr ShaderType FS = ShaderType::Fragment;
		constexpr ShaderType CS = ShaderType::Compute;

		[[nodiscard]] const std::vector<ShaderRegistration>& GetRegistrations()
		{
			// Variants of one source (bloom, skysphere, blur, cloud_noise, indirect draws) are selected with
			// preprocessor defines; each define set is compiled into its own program.
			static const std::vector<ShaderRegistration> registrations = {
				// geometry
				{ Renderer_Shader::depth_prepass,          "depth_prepass",          "depth_prepass.slang", {VS, FS}, {}, VertexType::PositionUvNormalTangent, true },
				{ Renderer_Shader::depth_prepass_indirect, "depth_prepass_indirect", "depth_prepass.slang", {VS},     {{"INDIRECT_DRAW"}} },
				{ Renderer_Shader::depth_light,            "depth_light",            "depth_light.slang",   {VS, FS}, {}, VertexType::PositionUvNormalTangent },
				{ Renderer_Shader::gbuffer,                "gbuffer",                "gbuffer.slang",       {VS, FS}, {}, VertexType::PositionUvNormalTangent, true },
				{ Renderer_Shader::gbuffer_indirect,       "gbuffer_indirect",       "gbuffer.slang",       {VS, FS}, {{"INDIRECT_DRAW"}} },
				{ Renderer_Shader::indirect_cull,          "indirect_cull",          "indirect_cull.slang", {CS} },

				// editor overlays
				{ Renderer_Shader::grid,    "grid",    "grid.slang",    {VS, FS},     {}, VertexType::PositionUvNormalTangent, true },
				{ Renderer_Shader::line,    "line",    "line.slang",    {VS, FS},     {}, VertexType::PositionColor, true },
				{ Renderer_Shader::outline, "outline", "outline.slang", {VS, FS, CS}, {}, VertexType::PositionUvNormalTangent },
				{ Renderer_Shader::font,    "font",    "font.slang",    {VS, FS},     {}, VertexType::PositionUv, true },
				{ Renderer_Shader::icon,    "icon",    "icons.slang",   {CS} },

				// forward passes over the composed frame (blended surfaces, light billboards)
				{ Renderer_Shader::forward_transparent, "forward_transparent", "forward_transparent.slang", {VS, FS}, {}, VertexType::PositionUvNormalTangent, true },
				{ Renderer_Shader::light_billboard,     "light_billboard",     "light_billboard.slang",     {VS, FS}, {}, VertexType::PositionUvNormalTangent, true },

				// lighting
				{ Renderer_Shader::light,                                "light",                                "light.slang",             {CS}, {}, VertexType::MaxEnum, true },
				{ Renderer_Shader::light_composition,                    "light_composition",                    "light_composition.slang", {CS}, {}, VertexType::MaxEnum, true },
				{ Renderer_Shader::light_image_based,                    "light_image_based",                    "light_base.slang",        {CS}, {}, VertexType::MaxEnum, true },
				{ Renderer_Shader::light_integration_brdf_specular_lut,  "light_integration_brdf_specular_lut",  "light_integration.slang", {CS}, {}, VertexType::MaxEnum, true },
				{ Renderer_Shader::light_integration_environment_filter, "light_integration_environment_filter", "light_integration.slang", {CS}, {}, VertexType::MaxEnum, true },
				{ Renderer_Shader::light_reflections,                    "light_reflections",                    "light_reflections.slang", {CS} },
				{ Renderer_Shader::ssao,                                 "ssao",                                 "ssao.slang",              {CS} },

				// sky / atmosphere
				{ Renderer_Shader::skysphere,                   "skysphere",                   "skysphere.slang", {CS}, {},                        VertexType::MaxEnum, true },
				{ Renderer_Shader::skysphere_lut,               "skysphere_lut",               "skysphere.slang", {CS}, {{"LUT"}},                 VertexType::MaxEnum, true },
				{ Renderer_Shader::skysphere_transmittance_lut, "skysphere_transmittance_lut", "skysphere.slang", {CS}, {{"TRANSMITTANCE_LUT"}},   VertexType::MaxEnum, true },
				{ Renderer_Shader::skysphere_multiscatter_lut,  "skysphere_multiscatter_lut",  "skysphere.slang", {CS}, {{"MULTISCATTER_LUT"}},    VertexType::MaxEnum, true },
				{ Renderer_Shader::cloud_noise_shape,           "cloud_noise_shape",           "cloud_noise.slang",   {CS}, {{"SHAPE_NOISE"}} },
				{ Renderer_Shader::cloud_noise_detail,          "cloud_noise_detail",          "cloud_noise.slang",   {CS}, {{"DETAIL_NOISE"}} },
				{ Renderer_Shader::cloud_shadow,                "cloud_shadow",                "cloud_shadows.slang", {CS} },

				// post-processing
				{ Renderer_Shader::blur_gaussian,            "blur_gaussian",            "blur.slang",   {CS} },
				{ Renderer_Shader::blur_gaussian_bilateral,  "blur_gaussian_bilateral",  "blur.slang",   {CS}, {{"PASS_BLUR_GAUSSIAN_BILATERAL"}} },
				{ Renderer_Shader::bloom_luminance,          "bloom_luminance",          "bloom.slang",  {CS}, {{"LUMINANCE"}} },
				{ Renderer_Shader::bloom_downsample,         "bloom_downsample",         "bloom.slang",  {CS}, {{"DOWNSAMPLE"}} },
				{ Renderer_Shader::bloom_upsample_blend_mip, "bloom_upsample_blend_mip", "bloom.slang",  {CS}, {{"UPSAMPLE_BLEND_MIP"}} },
				{ Renderer_Shader::bloom_blend_frame,        "bloom_blend_frame",        "bloom.slang",  {CS}, {{"BLEND_FRAME"}} },
				{ Renderer_Shader::auto_exposure,            "auto_exposure",            "auto_exposure.slang", {CS} },
				{ Renderer_Shader::dithering,                "dithering",                "dither.slang", {CS} },
				{ Renderer_Shader::output,                   "output",                   "output.slang", {CS} },
				{ Renderer_Shader::blit,                     "blit",                     "blit.slang",   {CS}, {}, VertexType::MaxEnum, true },

				// Not registered: ffx_spd_* (scale.slang has no entry point yet), sss_bend (sss_blend.slang disabled),
				// tessellation, depth_of_field, fxaa and the particle / texture-compression passes.
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

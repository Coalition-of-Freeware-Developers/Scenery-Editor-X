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
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "shader_manager.h"
#include "shader_compiler.h"
#include "SceneryEditorX/core/resource/resource_cache.h"

#include <algorithm>
#include <utility>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <slang/slang-com-ptr.h>
#include <slang/slang.h>
#include <spirv_cross/spirv_cross.hpp>
#include <algorithm>

// -------------------------------------------------------

namespace SceneryEditorX
{

#pragma region ShaderManager Static Members

	static Ref<ShaderManager> s_Instance = nullptr;
	std::mutex ShaderManager::s_ShaderMutex;
	std::unordered_map<std::string, Ref<Shader>> ShaderManager::m_Shaders;
	static std::array<Ref<Shader>,  static_cast<uint32_t>(Renderer_Shader::MaxEnum)> s_Shaders;

	/**
	 * @brief Helper function to create a Vulkan shader module from SPIR-V code
	 * @param spirvCode Pointer to the SPIR-V code
	 * @param codeSize Size of the SPIR-V code in bytes
	 * @return VkShaderModule handle to the created shader module
	 */
	static VkShaderModule CreateShaderModule(const void* spirvCode, size_t codeSize)
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();
		SEDX_CORE_ASSERT(device.IsValid(), "Invalid device while creating shader module");
		SEDX_CORE_ASSERT(spirvCode != nullptr && codeSize > 0, "Invalid SPIR-V input for shader module creation");

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = codeSize;
		createInfo.pCode = reinterpret_cast<const uint32_t*>(spirvCode);

		VkShaderModule module = VK_NULL_HANDLE;
		SEDX_VK_RESULT_ASSERT(vkCreateShaderModule(device->GetLogicalDevice(), &createInfo, nullptr, &module), "Failed to create shader module");
		return module;
	}

#pragma endregion

	void ShaderManager::CreateSingleBlob(const void *spirvCode, size_t codeSize)
	{
		// Use the same SPIR-V blob for multiple stages. Avoid inserting duplicate stage flags.
		m_Stages.reserve(2);
		m_Modules.reserve(2);

		auto push_if_absent = [&](VkShaderStageFlagBits stage)
		{
			if (std::ranges::find(m_Stages, stage) != m_Stages.end())
				return; // already present, skip

			VkShaderModule module = CreateShaderModule(spirvCode, codeSize);
			if (module != VK_NULL_HANDLE)
			{
				m_Stages.push_back(stage);
				m_Modules.push_back(module);
			}
			else
			{
				SEDX_CORE_WARN_TAG("ShaderManager", "Failed to create shader module for stage {}", static_cast<uint32_t>(stage));
			}
		};

		push_if_absent(VK_SHADER_STAGE_VERTEX_BIT);
		push_if_absent(VK_SHADER_STAGE_FRAGMENT_BIT);
		/**
		 * Note: compilation state is tracked per-Shader (SceneryEditorX::Shader). ShaderManager does not
		 * own a m_CompilationState for individual shaders; per-shader state should be updated on creation
		 * or reload via Shader instances. Leave this constructor responsibility to the owning Shader objects.
		 */
	}

	void ShaderManager::CreateShaderBlobs(const std::vector<std::pair<VkShaderStageFlagBits, std::pair<const void *, size_t>>> &stages)
	{
		m_Stages.reserve(stages.size());
		m_Modules.reserve(stages.size());
		for (const auto &s : stages)
		{
			VkShaderStageFlagBits stage = s.first;
			// avoid duplicate stage entries
			if (std::ranges::find(m_Stages, stage) != m_Stages.end())
			{
				SEDX_CORE_WARN_TAG("ShaderManager", "Duplicate shader stage requested, skipping: {}", static_cast<uint32_t>(stage));
				continue;
			}

			VkShaderModule module = CreateShaderModule(s.second.first, s.second.second);
			if (module != VK_NULL_HANDLE)
			{
				m_Stages.push_back(stage);
				m_Modules.push_back(module);
			}
			else
			{
				SEDX_CORE_WARN_TAG("ShaderManager", "Failed to create shader module for stage {}", static_cast<uint32_t>(stage));
			}
		}

		// Mark as compiled only if all modules were created successfully
		// Note: Do NOT attempt to set a Shader-specific compilation state from here.
		const bool allValid = !m_Modules.empty() && std::ranges::all_of(m_Modules, [](VkShaderModule m) { return m != VK_NULL_HANDLE; });
	}

	ShaderManager::ShaderManager()
	{
		SEDX_CORE_TRACE_TAG("ShaderManager", "Created ShaderManager");
	}

	ShaderManager::~ShaderManager()
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();
		if (!device.IsValid())
			return;

		for (VkShaderModule module : m_Modules)
		{
			if (module != VK_NULL_HANDLE)
			{
				vkDestroyShaderModule(device->GetLogicalDevice(), module, nullptr);
			}
		}
		m_Modules.clear();
	}

	Ref<ShaderManager> ShaderManager::Get()
	{
		if (!s_Instance)
			s_Instance = CreateRef<ShaderManager>();

		return s_Instance;
	}

	void ShaderManager::CreateShaders()
	{
		const std::string sd = ResourceCache::GetResourceDirectory(ResourceDirectory::Shaders);

		// debug
		SetShaderAvailable(Renderer_Shader::line_vertex);
		ShaderCompiler::CompileShader(Renderer_Shader::line_vertex,    StageType::Vertex,  sd + "line.hlsl",    true, VertexType::PositionColor);
		ShaderCompiler::CompileShader(Renderer_Shader::line_frag,    StageType::Fragment,   sd + "line.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::grid_vertex,    StageType::Vertex,  sd + "grid.hlsl",    true, VertexType::PositionUvNormalTangent);
		ShaderCompiler::CompileShader(Renderer_Shader::grid_frag,    StageType::Fragment,   sd + "grid.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::outline_vertex, StageType::Vertex,  sd + "outline.hlsl", true, VertexType::PositionUvNormalTangent);
		ShaderCompiler::CompileShader(Renderer_Shader::outline_frag, StageType::Fragment,   sd + "outline.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::outline_comp, StageType::Compute, sd + "outline.hlsl");

	    // depth
		ShaderCompiler::CompileShader(Renderer_Shader::depth_prepass_vertex,           StageType::Vertex, sd + "depth_prepass.hlsl", true, VertexType::PositionUvNormalTangent);
		ShaderCompiler::CompileShader(Renderer_Shader::depth_prepass_alpha_test_frag, StageType::Fragment,  sd + "depth_prepass.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::depth_light_vertex,             StageType::Vertex, sd + "depth_light.hlsl",  true, VertexType::PositionUvNormalTangent);
		ShaderCompiler::CompileShader(Renderer_Shader::depth_light_alpha_color_frag, StageType::Fragment,  sd + "depth_light.hlsl");

		// g-buffer
		ShaderCompiler::CompileShader(Renderer_Shader::gbuffer_vertex, StageType::Vertex, sd + "g_buffer.hlsl", true, VertexType::PositionUvNormalTangent);
		ShaderCompiler::CompileShader(Renderer_Shader::gbuffer_frag, StageType::Fragment,  sd + "g_buffer.hlsl");

		// tessellation
		ShaderCompiler::CompileShader(Renderer_Shader::tessellation_h, StageType::TessellationControl,   sd + "common_tessellation.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::tessellation_d, StageType::TessellationEvaluation, sd + "common_tessellation.hlsl");

		// light
		ShaderCompiler::CompileShader(Renderer_Shader::light_integration_brdf_specular_lut_c,  StageType::Compute, sd + "light_integration.hlsl", false, VertexType::MaxEnum, "BRDF_SPECULAR_LUT");
		ShaderCompiler::CompileShader(Renderer_Shader::light_integration_environment_filter_c, StageType::Compute, sd + "light_integration.hlsl", true,  VertexType::MaxEnum, "ENVIRONMENT_FILTER");
		ShaderCompiler::CompileShader(Renderer_Shader::light_c,                                StageType::Compute, sd + "light.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::light_composition_c,                    StageType::Compute, sd + "light_composition.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::light_image_based_c,                    StageType::Compute, sd + "light_image_based.hlsl");

		// blur
		ShaderCompiler::CompileShader(Renderer_Shader::blur_gaussian_c,            StageType::Compute, sd + "blur.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::blur_gaussian_bilaterial_c, StageType::Compute, sd + "blur.hlsl", true, VertexType::MaxEnum, "PASS_BLUR_GAUSSIAN_BILATERAL");

		// bloom
		ShaderCompiler::CompileShader(Renderer_Shader::bloom_luminance_c,          StageType::Compute, sd + "bloom.hlsl", true, VertexType::MaxEnum, "LUMINANCE");
		ShaderCompiler::CompileShader(Renderer_Shader::bloom_downsample_c,         StageType::Compute, sd + "bloom.hlsl", true, VertexType::MaxEnum, "DOWNSAMPLE");
		ShaderCompiler::CompileShader(Renderer_Shader::bloom_upsample_blend_mip_c, StageType::Compute, sd + "bloom.hlsl", true, VertexType::MaxEnum, "UPSAMPLE_BLEND_MIP");
		ShaderCompiler::CompileShader(Renderer_Shader::bloom_blend_frame_c,        StageType::Compute, sd + "bloom.hlsl", true, VertexType::MaxEnum, "BLEND_FRAME");

		// amd fidelityfx
		ShaderCompiler::CompileShader(Renderer_Shader::ffx_cas_c,         StageType::Compute, sd + "amd_fidelity_fx/cas.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::ffx_spd_average_c, StageType::Compute, sd + "amd_fidelity_fx/spd.hlsl", false, VertexType::MaxEnum, "AVERAGE");
		ShaderCompiler::CompileShader(Renderer_Shader::ffx_spd_min_c,     StageType::Compute, sd + "amd_fidelity_fx/spd.hlsl", false, VertexType::MaxEnum, "MIN");
		ShaderCompiler::CompileShader(Renderer_Shader::ffx_spd_max_c,     StageType::Compute, sd + "amd_fidelity_fx/spd.hlsl", false, VertexType::MaxEnum, "MAX");

		// sky
		ShaderCompiler::CompileShader(Renderer_Shader::skysphere_comp,                    StageType::Compute, sd + "sky/skysphere.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::skysphere_lut_comp,                StageType::Compute, sd + "sky/skysphere.hlsl", true,  VertexType::MaxEnum, "LUT");
		ShaderCompiler::CompileShader(Renderer_Shader::skysphere_transmittance_lut_c,  StageType::Compute, sd + "sky/skysphere.hlsl", false, VertexType::MaxEnum, "TRANSMITTANCE_LUT");
		ShaderCompiler::CompileShader(Renderer_Shader::skysphere_multiscatter_lut_c,   StageType::Compute, sd + "sky/skysphere.hlsl", false, VertexType::MaxEnum, "MULTISCATTER_LUT");

		// post-process
		ShaderCompiler::CompileShader(Renderer_Shader::fxaa_comp,                 StageType::Compute, sd + "fxaa/fxaa.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::font_vertex,                 StageType::Vertex,  sd + "font.hlsl", true, VertexType::PositionUv);
		ShaderCompiler::CompileShader(Renderer_Shader::font_frag,                 StageType::Fragment,   sd + "font.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::film_grain_comp,           StageType::Compute, sd + "film_grain.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::chromatic_aberration_c, StageType::Compute, sd + "chromatic_aberration.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::vhs_c,                  StageType::Compute, sd + "vhs.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::output_c,               StageType::Compute, sd + "output.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::motion_blur_c,          StageType::Compute, sd + "motion_blur.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::ssao_comp,                 StageType::Compute, sd + "ssao.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::sss_c_bend,             StageType::Compute, sd + "screen_space_shadows/bend_sss.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::depth_of_field_c,       StageType::Compute, sd + "depth_of_field.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::variable_rate_shading_c, StageType::Compute, sd + "variable_rate_shading.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::blit_c,                 StageType::Compute, sd + "blit.hlsl");

		// indirect draw
		ShaderCompiler::CompileShader(Renderer_Shader::indirect_cull_c,         StageType::Compute, sd + "indirect_cull.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::gbuffer_indirect_vertex,      StageType::Vertex,  sd + "g_buffer.hlsl",      true, VertexType::MaxEnum, "INDIRECT_DRAW");
		ShaderCompiler::CompileShader(Renderer_Shader::gbuffer_indirect_frag,      StageType::Fragment,   sd + "g_buffer.hlsl",      true, VertexType::MaxEnum, "INDIRECT_DRAW");
		ShaderCompiler::CompileShader(Renderer_Shader::depth_prepass_indirect_vertex, StageType::Vertex,  sd + "depth_prepass.hlsl", true, VertexType::MaxEnum, "INDIRECT_DRAW");

		// misc
		ShaderCompiler::CompileShader(Renderer_Shader::icon_c,                                  StageType::Compute, sd + "icon.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::dithering_c,                              StageType::Compute, sd + "dithering.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::transparency_reflection_refraction_c,     StageType::Compute, sd + "transparency_reflection_refraction.hlsl");
		ShaderCompiler::CompileShader(Renderer_Shader::auto_exposure_c,                          StageType::Compute, sd + "auto_exposure.hlsl");

		// volumetric clouds
		ShaderCompiler::CompileShader(Renderer_Shader::cloud_noise_shape_c,  StageType::Compute, sd + "sky/cloud_noise.hlsl",  true, VertexType::MaxEnum, "SHAPE_NOISE");
		ShaderCompiler::CompileShader(Renderer_Shader::cloud_noise_detail_c, StageType::Compute, sd + "sky/cloud_noise.hlsl",  true, VertexType::MaxEnum, "DETAIL_NOISE");
		ShaderCompiler::CompileShader(Renderer_Shader::cloud_shadow_c,       StageType::Compute, sd + "sky/cloud_shadow.hlsl");

		// gpu-driven particles
		ShaderCompiler::CompileShader(Renderer_Shader::particle_emit_c,     StageType::Compute, sd + "particles.hlsl", true, VertexType::MaxEnum, "EMIT");
		ShaderCompiler::CompileShader(Renderer_Shader::particle_simulate_c, StageType::Compute, sd + "particles.hlsl", true, VertexType::MaxEnum, "SIMULATE");
		ShaderCompiler::CompileShader(Renderer_Shader::particle_render_c,   StageType::Compute, sd + "particles.hlsl", true, VertexType::MaxEnum, "RENDER");

		// gpu texture compression (synchronous)
		ShaderCompiler::CompileShader(Renderer_Shader::texture_compress_bc1_c, StageType::Compute, sd + "texture_compress_bc1.hlsl", false);
		ShaderCompiler::CompileShader(Renderer_Shader::texture_compress_bc3_c, StageType::Compute, sd + "texture_compress_bc3.hlsl", false);
		ShaderCompiler::CompileShader(Renderer_Shader::texture_compress_bc5_c, StageType::Compute, sd + "texture_compress_bc5.hlsl", false);
	}

	Ref<Shader> &ShaderManager::CreateShader(const std::string &name)
	{
		std::scoped_lock lock(s_ShaderMutex);
		if (m_Shaders.contains(name))
			return m_Shaders[name];

		m_Shaders[name] = CreateRef<Shader>(name.c_str());
		return m_Shaders[name];
	}

	Ref<Shader> &ShaderManager::CreateShader(const std::string& name, const std::string &path, bool forceCompile)
	{
		std::scoped_lock lock(s_ShaderMutex);
		if (m_Shaders.contains(name))
			return m_Shaders[name];

		m_Shaders[name] = CreateRef<Shader>(name.c_str(), path, forceCompile);
		return m_Shaders[name];
	}

	Ref<Shader> &ShaderManager::GetShader(const std::string &name)
	{
		std::scoped_lock lock(s_ShaderMutex);
		SEDX_CORE_ASSERT(m_Shaders.contains(name), "Shader {} is not present", name.c_str());
		return m_Shaders[name];
	}

	void ShaderManager::ReloadShader(const Ref<Shader> &shaderName, bool forceCompile)
	{
		SEDX_CORE_ASSERT(shaderName.IsValid(), "Invalid shader reference");
		SEDX_CORE_WARN_TAG("ShaderManager", "Shader Reloading is not implemented yet");
		/*
		Renderer::Submit([shaderName, forceCompile]()
		{
			RenderThread_Reload(shaderName, forceCompile);
		});
		*/
	}

	void ShaderManager::RenderThread_Reload(const Ref<Shader> &shaderName, bool forceCompile)
	{
		SEDX_CORE_ASSERT(shaderName.IsValid(), "Invalid shader reference");

		// Iterate over all possible StageType enum values and recompile any stages present
		for (int i = 0; i < static_cast<int>(StageType::MaxEnum); ++i)
		{
			const StageType stage = static_cast<StageType>(i);
			if (!shaderName->HasStage(stage))
				continue; 

			Ref<ShaderStage> shaderStage = shaderName->GetShaderStage(stage);
			SEDX_CORE_ASSERT(shaderStage.IsValid(), "ShaderStage for stage {} is invalid", i);

			// Recompile the individual shader stage which should update its SPIR-V and VkShaderModule
			shaderStage->Recompile();
		}

		// Individual ShaderStage::Recompile handles the stage compilation and module recreation.
	}
		
	void ShaderManager::ClearAll()
	{
		std::scoped_lock lock(s_ShaderMutex);
		m_Shaders.clear();
	}

	void ShaderManager::ClearStage(const Ref<Shader> &shader, const StageType stage)
	{
		// TODO: Implement stage clearing logic. This should remove the specified stage from the shader and destroy its associated Vulkan shader module.
	}
	
	Shader* ShaderManager::GetShader(const Renderer_Shader type)
	{
		return s_Shaders[static_cast<uint8_t>(type)].Get();
	}

	void ShaderManager::SetShaderAvailable(const Renderer_Shader type)
	{
		const uint8_t index = static_cast<uint8_t>(type);

		for (size_t i = 0; i < s_Shaders.size(); ++i)
		{
		    if (!s_Shaders[index])
				s_Shaders[index] = CreateRef<Shader>(type);

		    Ref<Shader>& shader = s_Shaders[index];
		    SEDX_CORE_ASSERT(shader != nullptr, "Failed to allocate shader slot for type {}", static_cast<uint32_t>(type));
		    if (!shader->HasStage(StageType::Compute))
		    {
		        shader->AddShaderStage(StageType::Compute, "resources/shaders/blit.slang");
		    }
		}

		switch (type)
		{
			case Renderer_Shader::grid_vertex:
			case Renderer_Shader::grid_frag:
			{
				constexpr uint8_t gridVertexIndex = static_cast<uint8_t>(Renderer_Shader::grid_vertex);
				constexpr uint8_t gridFragIndex = static_cast<uint8_t>(Renderer_Shader::grid_frag);

				Ref<Shader>& gridShader = s_Shaders[gridVertexIndex];
				if (!gridShader)
				{
					gridShader = CreateRef<Shader>("grid");
				}

				if (!gridShader->HasStage(StageType::Vertex))
				{
					gridShader->AddShaderStage(StageType::Vertex, "resources/shaders/grid.slang");
				}

				if (!gridShader->HasStage(StageType::Fragment))
				{
					gridShader->AddShaderStage(StageType::Fragment, "resources/shaders/grid.slang");
				}

				s_Shaders[gridVertexIndex] = gridShader;
				s_Shaders[gridFragIndex] = gridShader;
				SEDX_CORE_ASSERT(s_Shaders[gridVertexIndex] != nullptr, "Failed to allocate grid shader");
				break;
			}
			case Renderer_Shader::blit_c:
			{
				if (!s_Shaders[index])
					s_Shaders[index] = CreateRef<Shader>("blit");

				Ref<Shader>& shader = s_Shaders[index];
				SEDX_CORE_ASSERT(shader != nullptr, "Failed to allocate shader slot for type {}", static_cast<uint32_t>(type));
				if (!shader->HasStage(StageType::Compute))
				{
					shader->AddShaderStage(StageType::Compute, "resources/shaders/blit.slang");
				}
				break;
			}
			default:
				if (!s_Shaders[index])
					s_Shaders[index] = CreateRef<Shader>();

				break;
		}
	}

	bool ShaderManager::IsShaderAvailable(Renderer_Shader type)
	{
		const uint8_t index = static_cast<uint8_t>(type);
		if (index >= s_Shaders.size())
		{
			SEDX_CORE_WARN_TAG("ShaderManager", "Shader type {} is out of bounds", static_cast<uint32_t>(type));
			return false;
		}

		const Ref<Shader>& shader = s_Shaders[index];
		return shader != nullptr && shader->IsCompiled();
	}

	std::array<Ref<Shader>, static_cast<uint32_t>(Renderer_Shader::MaxEnum)> &ShaderManager::GetShaders()
	{
		return s_Shaders;
	}

	std::vector<uint32_t> ShaderManager::CompileToSpirv(const StageType stage, const std::string &filepath, const bool optimize)
	{
		/* TODO: Do cache lookups here... */

		return ShaderCompiler::CompileShader(stage, filepath, optimize);
	}

	std::vector<ShaderInput> ShaderManager::ReflectInputs(const StageType stage, const std::vector<uint32_t> &spirv)
	{
		return ShaderCompiler::Reflect(stage, spirv);
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

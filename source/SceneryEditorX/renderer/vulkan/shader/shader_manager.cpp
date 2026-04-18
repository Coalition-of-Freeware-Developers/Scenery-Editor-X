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
#include <array>

// -------------------------------------------------------

namespace SceneryEditorX
{

#pragma region ShaderManager Static Members

	static Ref<ShaderManager> s_Instance = nullptr;
	std::mutex ShaderManager::s_ShaderMutex;
	std::unordered_map<std::string, Ref<Shader>> ShaderManager::m_Shaders;
	static std::array<Ref<Shader>,  static_cast<uint32_t>(Renderer_Shader::MaxEnum)> s_Shaders;
	static std::unordered_map<std::string, bool> s_SlangModules;

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
		SEDX_CORE_INFO_TAG("ShaderManager", "Creating startup shader registrations");
		CreateSlangModules();

		const auto& registrations = GetShaderRegistrationMap();
		for (const auto& [shaderType, registration] : registrations)
		{
			SEDX_CORE_ASSERT(registration.id == shaderType, "Shader registration key/id mismatch");
			SetShaderAvailable(shaderType);
		}

		// Initialize known alias slots to shared owners as part of startup.
		//SetShaderAvailable(Renderer_Shader::grid);
	}

	const std::unordered_map<std::string, ShaderManager::SlangModuleRegistration>& ShaderManager::GetSlangModuleRegistrationMap()
	{
		static const std::unordered_map<std::string, SlangModuleRegistration> SLANG_MODULE_REGISTRATIONS = {
			{"resources", {.debugName = "resources", .filepath = "resources/shaders/resources.slang", .required = true}},
			{"color", {.debugName = "color", .filepath = "resources/shaders/color.slang", .required = true}},
			{"math", {.debugName = "math", .filepath = "resources/shaders/math.slang", .required = true}},
			{"noise", {.debugName = "noise", .filepath = "resources/shaders/noise.slang", .required = true}},
			{"depth", {.debugName = "depth", .filepath = "resources/shaders/depth.slang", .required = true}},
			{"position", {.debugName = "position", .filepath = "resources/shaders/position.slang", .required = true}},
			{"constants", {.debugName = "constants", .filepath = "resources/shaders/constants.slang", .required = true}},
			{"common", {.debugName = "common", .filepath = "resources/shaders/common.slang", .required = true}},
			{"core", {.debugName = "core", .filepath = "resources/shaders/core.slang", .required = false}},
			{"surface", {.debugName = "surface", .filepath = "resources/shaders/surface.slang", .required = false}},
			{"lighting_lib", {.debugName = "lighting_lib", .filepath = "resources/shaders/lighting_lib.slang", .required = false}},
			{"pipeline", {.debugName = "pipeline", .filepath = "resources/shaders/pipeline.slang", .required = false}},
		};

		return SLANG_MODULE_REGISTRATIONS;
	}

	void ShaderManager::CreateSlangModules()
	{
		SEDX_CORE_INFO_TAG("ShaderManager", "Compiling registered Slang import modules");

		// Compile in dependency-safe order.
		// NOTE: map iteration is non-deterministic; explicit ordering avoids fragile startup behavior.
		static constexpr std::array<std::string_view, 12> MODULE_COMPILE_ORDER = {
			"constants",
			"resources",
			"color",
			"math",
			"noise",
			"depth",
			"position",
			"common",
			"core",
			"surface",
			"lighting_lib",
			"pipeline"
		};

		const auto& modules = GetSlangModuleRegistrationMap();

		for (const std::string_view moduleNameView : MODULE_COMPILE_ORDER)
		{
			const std::string moduleName{moduleNameView};
			const auto it = modules.find(moduleName);
			SEDX_CORE_ASSERT(it != modules.end(), "Missing Slang module registration for '{}'", moduleName);

			const bool success = CreateSlangModule(moduleName);
			if (!success && it->second.required)
			{
				SEDX_CORE_ASSERT(false, "Required Slang module failed to compile: {}", moduleName);
			}
		}

		// Validate that every registered module has an explicit compile-order entry.
		SEDX_CORE_ASSERT(modules.size() == MODULE_COMPILE_ORDER.size(),
			"Slang module registration count ({}) differs from compile order count ({}). Keep them in sync.",
			modules.size(), MODULE_COMPILE_ORDER.size());
	}

	bool ShaderManager::CreateSlangModule(const std::string& moduleName)
	{
		std::scoped_lock lock(s_ShaderMutex);

		const auto& modules = GetSlangModuleRegistrationMap();
		const auto it = modules.find(moduleName);
		if (it == modules.end())
		{
			SEDX_CORE_WARN_TAG("ShaderManager", "No Slang module registration found for '{}'", moduleName);
			return false;
		}

		const SlangModuleRegistration& registration = it->second;
		SEDX_CORE_ASSERT(!registration.filepath.empty(), "Slang module filepath cannot be empty for '{}'", moduleName);

		const bool success = ShaderCompiler::CompileSlangModule(registration.filepath);
		s_SlangModules[moduleName] = success;

		if (!success)
		{
			SEDX_CORE_ERROR_TAG("ShaderManager", "Failed to compile Slang module '{}' ({})", moduleName, registration.filepath);
			return false;
		}

		SEDX_CORE_INFO_TAG("ShaderManager", "Slang module '{}' is available", moduleName);
		return true;
	}

	const std::unordered_map<Renderer_Shader, ShaderManager::ShaderRegistration>& ShaderManager::GetShaderRegistrationMap()
	{
		static const std::unordered_map<Renderer_Shader, ShaderRegistration> SHADER_REGISTRATIONS = {
			{
				Renderer_Shader::line,
				{
					.id = Renderer_Shader::line,
					.debugName = "line",
					.asyncCompile = true,
					.stages = {
						{StageType::Vertex, "resources/shaders/line.slang", VertexType::PositionColor},
						{StageType::Fragment, "resources/shaders/line.slang"}
					}
				}
			},
			{
				Renderer_Shader::grid,
				{
					.id = Renderer_Shader::grid,
					.debugName = "grid",
					.asyncCompile = true,
					.stages = {
						{StageType::Vertex, "resources/shaders/grid.slang", VertexType::PositionUvNormalTangent},
						{StageType::Fragment, "resources/shaders/grid.slang"}
					}
				}
			},
			{
				Renderer_Shader::outline,
				{
					.id = Renderer_Shader::outline,
					.debugName = "outline",
					.asyncCompile = true,
					.stages = {
						{StageType::Vertex, "resources/shaders/outline.slang", VertexType::PositionUvNormalTangent},
						{StageType::Fragment, "resources/shaders/outline.slang"},
						{StageType::Compute, "resources/shaders/outline.slang"}
					}
				}
			},
			{
				Renderer_Shader::depth_prepass,
				{
					.id = Renderer_Shader::depth_prepass,
					.debugName = "depth_prepass",
					.asyncCompile = true,
					.stages = {
						{StageType::Vertex, "resources/shaders/depth_prepass.slang", VertexType::PositionUvNormalTangent},
						{StageType::Fragment, "resources/shaders/depth_prepass.slang"}
					}
				}
			},
			{
				Renderer_Shader::depth_light,
				{
					.id = Renderer_Shader::depth_light,
					.debugName = "depth_light",
					.asyncCompile = true,
					.stages = {
						{StageType::Vertex, "resources/shaders/depth_light.slang", VertexType::PositionUvNormalTangent},
						{StageType::Fragment, "resources/shaders/depth_light.slang"}
					}
				}
			},
			{
				Renderer_Shader::light,
				{
					.id = Renderer_Shader::light,
					.debugName = "light",
					.asyncCompile = false,
					.stages = {
						{StageType::Compute, "resources/shaders/light.slang"}
					}
				}
			},
			{
				Renderer_Shader::light_integration_brdf_specular_lut_c,
				{
					.id = Renderer_Shader::light_integration_brdf_specular_lut_c,
					.debugName = "light_integration_brdf_specular_lut",
					.asyncCompile = false,
					.stages = {
						{StageType::Compute, "resources/shaders/light_integration.slang", VertexType::MaxEnum}
					}
				}
			},
			{
				Renderer_Shader::light_integration_environment_filter_c,
				{
					.id = Renderer_Shader::light_integration_environment_filter_c,
					.debugName = "light_integration_environment_filter",
					.asyncCompile = true,
					.stages = {
						{StageType::Compute, "resources/shaders/light_integration.slang", VertexType::MaxEnum}
					}
				}
			},
			{
				Renderer_Shader::light_composition,
				{
					.id = Renderer_Shader::light_composition,
					.debugName = "light_composition",
					.asyncCompile = false,
					.stages = {
						{StageType::Compute, "resources/shaders/light_composition.slang"}
					}
				}
			},
			{
				Renderer_Shader::light_image_based_c,
				{
					.id = Renderer_Shader::light_image_based_c,
					.debugName = "light_image_based_c",
					.asyncCompile = false,
					.stages = {
						{StageType::Compute, "resources/shaders/light_base.slang"}
					}
				}
			},
			{
				Renderer_Shader::blur_gaussian,
				{
					.id = Renderer_Shader::blur_gaussian,
					.debugName = "blur_gaussian",
					.asyncCompile = false,
					.stages = {
						{StageType::Compute, "resources/shaders/blur.slang"}
					}
				}
			},
			{
				Renderer_Shader::blur_gaussian_bilateral,
				{
					.id = Renderer_Shader::blur_gaussian_bilateral,
					.debugName = "blur_gaussian_bilateral",
					.asyncCompile = true,
					.stages = {
						{StageType::Compute, "resources/shaders/blur.slang"}
					}
				}
			},
			{
				Renderer_Shader::bloom_luminance,
				{
					.id = Renderer_Shader::bloom_luminance,
					.debugName = "bloom_luminance",
					.asyncCompile = true,
					.stages = {
						{StageType::Compute, "resources/shaders/bloom.slang", VertexType::MaxEnum}
					}
				}
			},
			{
				Renderer_Shader::bloom_downsample_c,
				{
					.id = Renderer_Shader::bloom_downsample_c,
					.debugName = "bloom_downsample_c",
					.asyncCompile = true,
					.stages = {
						{StageType::Compute, "resources/shaders/bloom.slang", VertexType::MaxEnum}
					}
				}
			},
			{
				Renderer_Shader::bloom_upsample_blend_mip_c,
				{
					.id = Renderer_Shader::bloom_upsample_blend_mip_c,
					.debugName = "bloom_upsample_blend_mip_c",
					.asyncCompile = true,
					.stages = {
						{StageType::Compute, "resources/shaders/bloom.slang", VertexType::MaxEnum}
					}
				}
			},
			{
				Renderer_Shader::bloom_blend_frame,
				{
					.id = Renderer_Shader::bloom_blend_frame,
					.debugName = "bloom_blend_frame",
					.asyncCompile = true,
					.stages = {
						{StageType::Compute, "resources/shaders/bloom.slang", VertexType::MaxEnum}
					}
				}
			},
			{
				Renderer_Shader::skysphere,
				{
					.id = Renderer_Shader::skysphere,
					.debugName = "skysphere",
					.asyncCompile = true,
					.stages = {
						{StageType::Compute, "resources/shaders/skysphere.slang"}
					}
				}
			},
			{
				Renderer_Shader::skysphere_lut,
				{
					.id = Renderer_Shader::skysphere_lut,
					.debugName = "skysphere_lut",
					.asyncCompile = true,
					.stages = {
						{StageType::Compute, "resources/shaders/skysphere.slang"}
					}
				}
			},
			{
				Renderer_Shader::skysphere_transmittance_lut,
				{
					.id = Renderer_Shader::skysphere_transmittance_lut,
					.debugName = "skysphere_transmittance_lut",
					.asyncCompile = false,
					.stages = {
						{StageType::Compute, "resources/shaders/skysphere.slang"}
					}
				}
			},
			{
				Renderer_Shader::skysphere_multiscatter_lut,
				{
					.id = Renderer_Shader::skysphere_multiscatter_lut,
					.debugName = "skysphere_multiscatter_lut",
					.asyncCompile = false,
					.stages = {
						{StageType::Compute, "resources/shaders/skysphere.slang"}
					}
				}
			},
			{
				Renderer_Shader::blit,
				{
					.id = Renderer_Shader::blit,
					.debugName = "blit",
					.asyncCompile = false,
					.stages = {
						{StageType::Compute, "resources/shaders/blit.slang"}
					}
				}
			},
			{
				Renderer_Shader::gbuffer,
				{
					.id = Renderer_Shader::gbuffer,
					.debugName = "g_buffer",
					.asyncCompile = false,
					.stages = {
						{StageType::Vertex, "resources/shaders/gbuffer.slang", VertexType::PositionUvNormalTangent},
						{StageType::Fragment, "resources/shaders/gbuffer.slang"}
					}
				}
			},
			{
				Renderer_Shader::font,
				{
					.id = Renderer_Shader::font,
					.debugName = "font",
					.asyncCompile = true,
					.stages = {
						{StageType::Vertex, "resources/shaders/font.slang"},
						{StageType::Fragment, "resources/shaders/font.slang"}
					}
				}
			},
			{
				Renderer_Shader::icon,
				{
					.id = Renderer_Shader::icon,
					.debugName = "icon",
					.asyncCompile = true,
					.stages = {
						{StageType::Vertex, "resources/shaders/icon.slang"},
						{StageType::Fragment, "resources/shaders/icon.slang"}
					}
				}
			},
			{
				Renderer_Shader::output,
				{
					.id = Renderer_Shader::output,
					.debugName = "output",
					.asyncCompile = true,
					.stages = {
						{StageType::Compute, "resources/shaders/output.slang"}
					}
				}
			},
			{
				Renderer_Shader::indirect_cull_c,
				{
					.id = Renderer_Shader::indirect_cull_c,
					.debugName = "indirect_cull_c",
					.asyncCompile = true,
					.stages = {
						{StageType::Compute, "resources/shaders/indirect_cull.slang"}
					}
				}
			},
			{
				Renderer_Shader::gbuffer_indirect,
				{
					.id = Renderer_Shader::gbuffer_indirect,
					.debugName = "gbuffer_indirect",
					.asyncCompile = true,
					.stages = {
						{StageType::Vertex, "resources/shaders/gbuffer.slang", VertexType::MaxEnum},
						{StageType::Fragment, "resources/shaders/gbuffer.slang", VertexType::MaxEnum}
					}
				}
			},
			{
				Renderer_Shader::depth_prepass_indirect,
				{
					.id = Renderer_Shader::depth_prepass_indirect,
					.debugName = "depth_prepass_indirect",
					.asyncCompile = true,
					.stages = {
						{StageType::Vertex, "resources/shaders/depth_prepass.slang", VertexType::MaxEnum}
					}
				}
			},
		};

		return SHADER_REGISTRATIONS;
	}

	bool ShaderManager::TryGetSharedOwner(const Renderer_Shader type, Renderer_Shader& outOwnerType)
	{
		if (type == Renderer_Shader::grid)
		{
			outOwnerType = Renderer_Shader::grid;
			return true;
		}

		if (type == Renderer_Shader::gbuffer)
		{
			outOwnerType = Renderer_Shader::gbuffer;
			return true;
		}

		outOwnerType = type;
		return false;
	}

	const ShaderManager::ShaderRegistration* ShaderManager::FindRegistration(const Renderer_Shader type)
	{
		auto& registrations = GetShaderRegistrationMap();
		if (const auto it = registrations.find(type); it != registrations.end())
		{
			return &it->second;
		}

		if (Renderer_Shader owner = type; TryGetSharedOwner(type, owner))
		{
			if (const auto ownerIt = registrations.find(owner); ownerIt != registrations.end())
				return &ownerIt->second;
		}

		return nullptr;
	}

	void ShaderManager::ApplyRegistration(const ShaderRegistration& registration)
	{
		const uint32_t ownerIndex = static_cast<uint32_t>(registration.id);
		SEDX_CORE_ASSERT(ownerIndex < s_Shaders.size(), "Shader registration index out of bounds: {}", ownerIndex);

		Ref<Shader>& shader = s_Shaders[ownerIndex];
		if (!shader)
		{
			if (registration.debugName)
			{
				shader = CreateRef<Shader>(registration.debugName);
			}
			else
			{
				shader = CreateRef<Shader>(registration.id);
			}
		}

		SEDX_CORE_ASSERT(shader != nullptr, "Failed to allocate shader slot for type {}", static_cast<uint32_t>(registration.id));

		auto apply_stages = [&shader, &registration]()
		{
			for (const ShaderStageDescriptor& stageInfo : registration.stages)
			{
				SEDX_CORE_ASSERT(!stageInfo.filepath.empty(), "Shader stage filepath cannot be empty");
				if (!shader->HasStage(stageInfo.stage))
				{
					shader->AddShaderStage(stageInfo.stage, stageInfo.filepath, stageInfo.vertexType);
				}
			}
		};

		if (registration.asyncCompile)
		{
			Renderer::Submit([apply_stages]() mutable
			{
				apply_stages();
			});
		}
		else
		{
			apply_stages();
		}
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
		s_SlangModules.clear();
	}

	void ShaderManager::ClearShaderStage(const Ref<Shader> &shader, const StageType stage)
	{
		// TODO: Implement stage clearing logic. This should remove the specified stage from the shader and destroy its associated Vulkan shader module.
	}
	
	Shader* ShaderManager::GetShader(const Renderer_Shader type)
	{
		return s_Shaders[static_cast<uint8_t>(type)].Get();
	}

	void ShaderManager::SetShaderAvailable(const Renderer_Shader type)
	{
		std::scoped_lock lock(s_ShaderMutex);

		if (const ShaderRegistration* registration = FindRegistration(type))
		{
			ApplyRegistration(*registration);

			if (Renderer_Shader owner = type; TryGetSharedOwner(type, owner))
			{
				const uint32_t index = static_cast<uint32_t>(type);
				const uint32_t ownerIndex = static_cast<uint32_t>(owner);
				SEDX_CORE_ASSERT(index < s_Shaders.size() && ownerIndex < s_Shaders.size(), "Shared shader alias index out of bounds");
				s_Shaders[index] = s_Shaders[ownerIndex];
			}

			return;
		}

		SEDX_CORE_WARN_TAG("ShaderManager", "No shader registration found for enum {}", static_cast<uint32_t>(type));
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

	bool ShaderManager::IsSlangModuleAvailable(const std::string& moduleName)
	{
		std::scoped_lock lock(s_ShaderMutex);

		const auto it = s_SlangModules.find(moduleName);
		if (it == s_SlangModules.end())
			return false;

		return it->second;
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

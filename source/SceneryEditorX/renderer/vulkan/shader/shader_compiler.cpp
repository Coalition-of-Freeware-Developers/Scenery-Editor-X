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
 * shader_compiler.h
 * -------------------------------------------------------
 * Created: 11/04/2026
 * -------------------------------------------------------
 */
// ReSharper disable CppReinterpretCastFromVoidPtr
#include "shader_compiler.h"
#include <array>
#include <cstring>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <slang/slang-com-ptr.h>
#include <slang/slang.h>

// --------------------------------------------------------------

namespace ShaderCompiler
{
	/**
	 * @brief Checks if a module is a primary shared library module.
	 * @param moduleName The name of the module to check.
	 * @return True if the module is a primary shared library module, false otherwise.
	 */
	static bool IsPrimarySharedLibraryModule(const std::string& moduleName)
	{
		static const std::unordered_set<std::string> SHARED_LIBRARY_PRIMARY_MODULES = {
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

		return SHARED_LIBRARY_PRIMARY_MODULES.contains(moduleName);
	}

	/**
	 * @brief Checks if a module is an entry shader file.
	 * @param moduleName The name of the module to check.
	 * @return True if the module is an entry shader file, false otherwise.
	 */
	static bool IsEntryShaderFile(const std::string& moduleName)
	{
		static const std::unordered_set<std::string> ENTRY_SHADER_MODULES = {
			"line",
			"grid",
			"outline",
			"depth_prepass",
			"depth_light",
			"light",
			"light_integration",
			"light_composition",
			"light_base",
			"blur",
			"bloom",
			"skysphere",
			"blit",
			"gbuffer",
			"font",
			"icon",
			"icons",
			"output",
			"indirect_cull",
			"pre_pass",
			"dof_lighting",
			"lighting",
			"shadow_mapping",
			"ssao",
			"sss_blend",
			"terrain",
			"ui",
			"fog",
			"fxaa",
			"dof",
			"auto_exposure",
			"dither",
			"cloud_noise",
			"cloud_shadows",
			"light_reflections",
			"camera",
			"brdf",
			"shader"
		};

		return ENTRY_SHADER_MODULES.contains(moduleName);
	}

	/**
	 * @brief Creates a Slang compilation session configured for Vulkan SPIR-V output.
	 * @param outGlobalSession Receives the global Slang session.
	 * @param outSession Receives the per-compilation Slang session.
	 * @return True when the session is created successfully, false otherwise.
	 */
	static bool CreateSlangSession(Slang::ComPtr<slang::IGlobalSession>& outGlobalSession, Slang::ComPtr<slang::ISession>& outSession)
	{
		if (SLANG_FAILED(slang::createGlobalSession(outGlobalSession.writeRef())))
		{
			SEDX_CORE_FATAL_TAG("ShaderCompiler", "Failed to create Slang compiler global session");
			return false;
		}

		auto targets = std::to_array<slang::TargetDesc>({
			{.format = SLANG_SPIRV, .profile = outGlobalSession->findProfile("spirv_1_4")}
		});

		auto options = std::to_array<slang::CompilerOptionEntry>({
			{.name = slang::CompilerOptionName::EmitSpirvDirectly,
			 .value = {.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1}}
		});

		slang::SessionDesc sessionDesc{};
		sessionDesc.targets = targets.data();
		sessionDesc.targetCount = static_cast<SlangInt>(targets.size());
		sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
		sessionDesc.compilerOptionEntries = options.data();
		sessionDesc.compilerOptionEntryCount = static_cast<uint32_t>(options.size());

		if (SLANG_FAILED(outGlobalSession->createSession(sessionDesc, outSession.writeRef())))
		{
			SEDX_CORE_FATAL_TAG("ShaderCompiler", "Failed to create Slang session");
			return false;
		}

		return true;
	}

	/**
	 * @brief Loads a Slang module from source and reports diagnostics on failure.
	 * @param session The active Slang session.
	 * @param filepath Source file path.
	 * @param outDiagnostics Receives diagnostic output blob.
	 * @return Loaded module pointer or nullptr on failure.
	 */
	static Slang::ComPtr<slang::IModule> LoadSlangModule(const Slang::ComPtr<slang::ISession>& session, const std::string& filepath, Slang::ComPtr<slang::IBlob>& outDiagnostics)
	{
		const std::filesystem::path shaderPath(filepath);
		const std::string moduleName = shaderPath.stem().string();

		if (!IsPrimarySharedLibraryModule(moduleName) && !IsEntryShaderFile(moduleName))
		{
			SEDX_CORE_ERROR_TAG("ShaderCompiler", "Refusing to compile non-primary/non-entry Slang file '{}'. Compile module primaries or entry shaders only.", filepath);
			return nullptr;
		}

		Slang::ComPtr<slang::IModule> module{session->loadModuleFromSource(moduleName.c_str(), filepath.c_str(), nullptr, outDiagnostics.writeRef())};
		if (!module)
		{
			if (outDiagnostics)
			{
				SEDX_CORE_ERROR_TAG("ShaderCompiler", "Slang compilation failed: {}", static_cast<const char*>(outDiagnostics->getBufferPointer()));
			}
			else
			{
				SEDX_CORE_ERROR_TAG("ShaderCompiler", "Slang compilation failed for '{}'", filepath);
			}
		}

		return module;
	}

	/**
	 * @brief Maps engine shader stage to Slang stage enum.
	 * @param stage Engine shader stage.
	 * @return Corresponding Slang stage.
	 */
	static SlangStage ToSlangStage(const SceneryEditorX::StageType stage)
	{
		switch (stage)
		{
			case SceneryEditorX::StageType::Vertex: return SLANG_STAGE_VERTEX;
			case SceneryEditorX::StageType::Fragment: return SLANG_STAGE_FRAGMENT;
			case SceneryEditorX::StageType::Compute: return SLANG_STAGE_COMPUTE;
			case SceneryEditorX::StageType::Geometry: return SLANG_STAGE_GEOMETRY;
			case SceneryEditorX::StageType::TessellationControl: return SLANG_STAGE_HULL;
			case SceneryEditorX::StageType::TessellationEvaluation: return SLANG_STAGE_DOMAIN;
			default: return SLANG_STAGE_NONE;
		}
	}

	/**
	 * @brief Gets ordered candidate entry-point names for a stage.
	 * @param stage Engine shader stage.
	 * @return Candidate entry-point names.
	 */
	static std::vector<const char*> GetEntryPointCandidates(const SceneryEditorX::StageType stage)
	{
		switch (stage)
		{
			case SceneryEditorX::StageType::Vertex: return {"main_vs", "main"};
			case SceneryEditorX::StageType::Fragment: return {"main_frag", "main"};
			case SceneryEditorX::StageType::Compute: return {"main_comp", "main"};
			case SceneryEditorX::StageType::Geometry: return {"main_geom", "main"};
			case SceneryEditorX::StageType::TessellationControl: return {"main_tesc", "main"};
			case SceneryEditorX::StageType::TessellationEvaluation: return {"main_tese", "main"};
			default: return {"main"};
		}
	}

	/**
	 * @brief Attempts to locate and validate a stage entry point in a Slang module.
	 * @param module Slang module.
	 * @param stage Engine shader stage.
	 * @param outEntryPoint Receives validated entry point.
	 * @return True if an entry point was resolved, false otherwise.
	 */
	static bool FindStageEntryPoint(Slang::ComPtr<slang::IModule>& module, const SceneryEditorX::StageType stage, Slang::ComPtr<slang::IEntryPoint>& outEntryPoint)
	{
		if (!module)
			return false;

		const SlangStage slangStage = ToSlangStage(stage);
		const std::vector<const char*> candidates = GetEntryPointCandidates(stage);

		for (const char* candidate : candidates)
		{
			Slang::ComPtr<slang::IBlob> diagnostics;
			Slang::ComPtr<slang::IEntryPoint> entryPoint;
			if (SLANG_SUCCEEDED(module->findAndCheckEntryPoint(candidate, slangStage, entryPoint.writeRef(), diagnostics.writeRef())) && entryPoint)
			{
				outEntryPoint = entryPoint;
				return true;
			}

			if (diagnostics && diagnostics->getBufferSize() > 0)
			{
				SEDX_CORE_TRACE_TAG("ShaderCompiler", "Entry-point candidate '{}' rejected: {}", candidate, static_cast<const char*>(diagnostics->getBufferPointer()));
			}
		}

		SEDX_CORE_ERROR_TAG("ShaderCompiler", "Failed to resolve Slang entry point for stage {} in module '{}'", static_cast<uint32_t>(stage), module->getName());
		return false;
	}

	/**
	 * @brief Maps Slang binding metadata to engine shader input type.
	 * @param category Slang parameter category.
	 * @param bindingType Slang binding type.
	 * @return Corresponding engine shader input type.
	 */
	static SceneryEditorX::ShaderInputType ToShaderInputType(const slang::ParameterCategory category, const slang::BindingType bindingType)
	{
		auto map_by_binding_type = [](const slang::BindingType type, const bool unorderedAccess) -> SceneryEditorX::ShaderInputType
		{
			switch (type)
			{
				case slang::BindingType::CombinedTextureSampler:
					return SceneryEditorX::ShaderInputType::CombinedImageSampler;

				case slang::BindingType::Sampler:
					return SceneryEditorX::ShaderInputType::Sampler;

				case slang::BindingType::Texture:
				case slang::BindingType::InputRenderTarget:
					return unorderedAccess ? SceneryEditorX::ShaderInputType::StorageImage : SceneryEditorX::ShaderInputType::Texture;

				// MutableTexture is writable and should always map to a storage image,
				// even if Slang reports a non-UnorderedAccess category for this range.
				case slang::BindingType::MutableTexture:
					return SceneryEditorX::ShaderInputType::StorageImage;

				case slang::BindingType::ConstantBuffer:
					return SceneryEditorX::ShaderInputType::UniformBuffer;

				case slang::BindingType::TypedBuffer:
				case slang::BindingType::RawBuffer:
				case slang::BindingType::MutableTypedBuffer:
				case slang::BindingType::MutableRawBuffer:
					return SceneryEditorX::ShaderInputType::StorageBuffer;

				default:
					return SceneryEditorX::ShaderInputType::MaxEnum;
			}
		};

		switch (category)
		{
			case slang::ParameterCategory::ConstantBuffer:
				// Slang can report ConstantBuffer category for descriptor-table members.
				// Only treat true cbuffer bindings as UniformBuffer; classify everything
				// else by binding type to avoid set-0 type mismatches in Vulkan layouts.
				if (bindingType == slang::BindingType::ConstantBuffer)
					return SceneryEditorX::ShaderInputType::UniformBuffer;

				return map_by_binding_type(
					bindingType,
					bindingType == slang::BindingType::MutableTexture ||
					bindingType == slang::BindingType::MutableTypedBuffer ||
					bindingType == slang::BindingType::MutableRawBuffer);

			case slang::ParameterCategory::UnorderedAccess:
				return map_by_binding_type(bindingType, true);

			case slang::ParameterCategory::SamplerState:
				return SceneryEditorX::ShaderInputType::Sampler;

			case slang::ParameterCategory::InputAttachmentIndex:
				return SceneryEditorX::ShaderInputType::Texture;

			case slang::ParameterCategory::ShaderResource:
			case slang::ParameterCategory::DescriptorTableSlot:
			default:
				return map_by_binding_type(bindingType, false);
		}
	}

	/**
	 * @brief Converts reflected descriptor count to engine descriptor count semantics.
	 * @param count Reflected descriptor count.
	 * @return Descriptor count with bindless conversion applied.
	 */
	static uint32_t ConvertDescriptorCount(const SlangInt count)
	{
		if (count <= 0 || count == static_cast<SlangInt>(SLANG_UNKNOWN_SIZE))
			return 1u;

		if (count == static_cast<SlangInt>(SLANG_UNBOUNDED_SIZE))
			return MAX_BINDLESS;

		return static_cast<uint32_t>(count);
	}

	/**
	 * @brief Converts a reflected shader input type to readable text for diagnostics.
	 * @param type Shader input type.
	 * @return Readable string representing the shader input type for diagnostics.
	 */
	static const char* ShaderInputTypeToString(const SceneryEditorX::ShaderInputType type)
	{
		switch (type)
		{
			case SceneryEditorX::ShaderInputType::Texture:               return "Texture";
			case SceneryEditorX::ShaderInputType::Sampler:               return "Sampler";
			case SceneryEditorX::ShaderInputType::StorageImage:          return "StorageImage";
			case SceneryEditorX::ShaderInputType::StorageBuffer:         return "StorageBuffer";
			case SceneryEditorX::ShaderInputType::UniformBuffer:         return "UniformBuffer";
			case SceneryEditorX::ShaderInputType::StorageBufferSet:      return "StorageBufferSet";
			case SceneryEditorX::ShaderInputType::UniformBufferSet:      return "UniformBufferSet";
			case SceneryEditorX::ShaderInputType::CombinedImageSampler:  return "CombinedImageSampler";
			case SceneryEditorX::ShaderInputType::PushConstant:          return "PushConstant";
			default:                                                     return "Unknown";
		}
	}

	/**
	 * @brief Provides Vulkan descriptor type text expected by the reflected shader input type.
	 * @param type Shader input type.
	 * @return Vulkan descriptor type as string for diagnostics.
	 */
	static const char* ShaderInputTypeToVkDescriptorString(const SceneryEditorX::ShaderInputType type)
	{
		switch (type)
		{
			case SceneryEditorX::ShaderInputType::Texture:
			case SceneryEditorX::ShaderInputType::CombinedImageSampler:  return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
			case SceneryEditorX::ShaderInputType::Sampler:               return "VK_DESCRIPTOR_TYPE_SAMPLER";
			case SceneryEditorX::ShaderInputType::StorageImage:          return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
			case SceneryEditorX::ShaderInputType::StorageBuffer:
			case SceneryEditorX::ShaderInputType::StorageBufferSet:      return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
			case SceneryEditorX::ShaderInputType::UniformBuffer:
			case SceneryEditorX::ShaderInputType::UniformBufferSet:      return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
			default:                                                     return "VK_DESCRIPTOR_TYPE_MAX_ENUM";
		}
	}

	/**
	 * @brief Adds reflected descriptor ranges from a type layout into shader inputs.
	 * @param typeLayout Slang type layout to inspect.
	 * @param stage Shader stage associated with reflection.
	 * @param outInputs Destination shader input array.
	 */
	static void AppendDescriptorRanges(slang::TypeLayoutReflection* typeLayout, const SceneryEditorX::StageType stage, std::vector<SceneryEditorX::ShaderInput>& outInputs)
	{
		if (!typeLayout)
			return;

		std::unordered_set<uint64_t> dedupe;
		dedupe.reserve(64);

		const SlangInt setCount = typeLayout->getDescriptorSetCount();
		for (SlangInt setIndex = 0; setIndex < setCount; ++setIndex)
		{
			std::unordered_map<SlangInt, std::string> descriptorRangeNames;
			SlangInt descriptorRangeBase = std::numeric_limits<SlangInt>::max();
			const SlangInt bindingRangeCount = typeLayout->getBindingRangeCount();
			for (SlangInt bindingRangeIndex = 0; bindingRangeIndex < bindingRangeCount; ++bindingRangeIndex)
			{
				if (typeLayout->getBindingRangeDescriptorSetIndex(bindingRangeIndex) != setIndex)
					continue;

				const SlangInt firstRangeIndex = typeLayout->getBindingRangeFirstDescriptorRangeIndex(bindingRangeIndex);
				const SlangInt rangeSpan = typeLayout->getBindingRangeDescriptorRangeCount(bindingRangeIndex);

				descriptorRangeBase = std::min(descriptorRangeBase, firstRangeIndex);

				std::string baseName;
				if (slang::VariableReflection* leafVariable = typeLayout->getBindingRangeLeafVariable(bindingRangeIndex))
				{
					if (const char* leafName = leafVariable->getName(); leafName && leafName[0] != '\0')
						baseName = leafName;
				}

				for (SlangInt localRangeIndex = 0; localRangeIndex < rangeSpan; ++localRangeIndex)
				{
					const SlangInt descriptorRangeIndex = firstRangeIndex + localRangeIndex;

					if (!baseName.empty())
					{
						descriptorRangeNames[descriptorRangeIndex] = baseName;
					}
				}
			}

			if (descriptorRangeBase != std::numeric_limits<SlangInt>::max())
			{
				std::unordered_map<SlangInt, std::string> normalizedRangeNames;
				normalizedRangeNames.reserve(descriptorRangeNames.size());
				for (const auto& [rangeIndex, rangeName] : descriptorRangeNames)
				{
					normalizedRangeNames[rangeIndex - descriptorRangeBase] = rangeName;
				}

				for (const auto& [rangeIndex, rangeName] : normalizedRangeNames)
				{
					descriptorRangeNames.emplace(rangeIndex, rangeName);
				}
			}

			// In Slang, descriptor-set spaces map to Vulkan set indices.
			// `setIndex` is the local descriptor-set index and may be 0 for every
			// space, so we must include the reflected space offset to avoid
			// collapsing all resources into set 0.
			const SlangInt descriptorSetSpace = typeLayout->getDescriptorSetSpaceOffset(setIndex);
			const uint32_t descriptorSet = (descriptorSetSpace >= 0 && descriptorSetSpace != static_cast<SlangInt>(SLANG_UNKNOWN_SIZE))
				? static_cast<uint32_t>(descriptorSetSpace) : static_cast<uint32_t>(setIndex);

			auto get_known_set0_binding_override = [](const std::string& name) -> std::optional<uint32_t>
			{
				if (name == "BufferFrame") return 200u;

				if (name == "tex_uav") return 100u;
				if (name == "tex_uav2") return 101u;
				if (name == "tex_uav3") return 102u;
				if (name == "tex_uav4") return 103u;
				if (name == "tex3d_uav") return 104u;
				if (name == "tex_uav_sss") return 105u;
				if (name == "tex_uav_mips") return 108u;
				if (name == "tex_uav_uint") return 130u;

				if (name == "indirect_draw_args") return 131u;
				if (name == "indirect_draw_data") return 132u;
				if (name == "indirect_draw_args_out") return 133u;
				if (name == "indirect_draw_data_out") return 134u;
				if (name == "indirect_draw_count") return 135u;
				if (name == "particle_buffer_a") return 136u;
				if (name == "particle_counter") return 138u;
				if (name == "particle_emitter") return 139u;
				if (name == "tex_compress_in") return 140u;
				if (name == "tex_compress_out") return 141u;
				if (name == "tex_compress_out_bc1") return 142u;
				if (name == "visibility") return 143u;
				if (name == "g_atomic_counter") return 144u;

				return std::nullopt;
			};

			auto get_known_set0_type_override = [](const std::string& name) -> std::optional<SceneryEditorX::ShaderInputType>
			{
				if (name == "BufferFrame")
					return SceneryEditorX::ShaderInputType::UniformBuffer;

				if (name.rfind("tex_uav", 0) == 0)
					return SceneryEditorX::ShaderInputType::StorageImage;

				if (name == "indirect_draw_args"
					|| name == "indirect_draw_data"
					|| name == "indirect_draw_args_out"
					|| name == "indirect_draw_data_out"
					|| name == "indirect_draw_count"
					|| name == "particle_buffer_a"
					|| name == "particle_counter"
					|| name == "particle_emitter"
					|| name == "tex_compress_in"
					|| name == "tex_compress_out"
					|| name == "tex_compress_out_bc1"
					|| name == "visibility"
					|| name == "g_atomic_counter")
				{
					return SceneryEditorX::ShaderInputType::StorageBuffer;
				}

				return std::nullopt;
			};

			const SlangInt rangeCount = typeLayout->getDescriptorSetDescriptorRangeCount(setIndex);
			for (SlangInt rangeIndex = 0; rangeIndex < rangeCount; ++rangeIndex)
			{
				const SlangInt bindingOffset = typeLayout->getDescriptorSetDescriptorRangeIndexOffset(setIndex, rangeIndex);
				if (bindingOffset < 0 || bindingOffset == static_cast<SlangInt>(SLANG_UNKNOWN_SIZE))
					continue;

				uint32_t binding = static_cast<uint32_t>(bindingOffset);
				const slang::BindingType bindingType = typeLayout->getDescriptorSetDescriptorRangeType(setIndex, rangeIndex);
				const slang::ParameterCategory category = typeLayout->getDescriptorSetDescriptorRangeCategory(setIndex, rangeIndex);
				SceneryEditorX::ShaderInputType inputType = ToShaderInputType(category, bindingType);
				if (inputType == SceneryEditorX::ShaderInputType::MaxEnum)
					continue;

				std::string debugName;
				if (const auto nameIt = descriptorRangeNames.find(rangeIndex); nameIt != descriptorRangeNames.end())
				{
					debugName = nameIt->second;
				}
				if (debugName.empty())
				{
					debugName = "set" + std::to_string(descriptorSet) + "_binding" + std::to_string(binding);
				}

				if (descriptorSet == 0)
				{
					if (const std::optional<uint32_t> knownBinding = get_known_set0_binding_override(debugName); knownBinding.has_value())
					{
						binding = *knownBinding;
					}

					if (const std::optional<SceneryEditorX::ShaderInputType> knownType = get_known_set0_type_override(debugName); knownType.has_value())
					{
						inputType = *knownType;
					}
				}

				const uint64_t dedupeKey = (static_cast<uint64_t>(descriptorSet) << 32ull) | (static_cast<uint64_t>(binding) << 8ull) | static_cast<uint64_t>(inputType);
				if (!dedupe.insert(dedupeKey).second)
					continue;

				if (descriptorSet == 0)
				{
					const uint64_t sampledVsStorageKey = (static_cast<uint64_t>(descriptorSet) << 32ull) | (static_cast<uint64_t>(binding) << 8ull);
					const uint64_t sampledTypeKey = sampledVsStorageKey | static_cast<uint64_t>(SceneryEditorX::ShaderInputType::Texture);
					const uint64_t combinedTypeKey = sampledVsStorageKey | static_cast<uint64_t>(SceneryEditorX::ShaderInputType::CombinedImageSampler);
					if (inputType == SceneryEditorX::ShaderInputType::StorageImage)
					{
						dedupe.erase(sampledTypeKey);
						dedupe.erase(combinedTypeKey);
					}
					else if (inputType == SceneryEditorX::ShaderInputType::StorageBuffer)
					{
						const uint64_t uniformTypeKey = sampledVsStorageKey | static_cast<uint64_t>(SceneryEditorX::ShaderInputType::UniformBuffer);
						dedupe.erase(uniformTypeKey);
					}
				}

				bool alreadyPresent = false;
				for (const auto& existing : outInputs)
				{
					if (existing.stage == stage && existing.set == descriptorSet && existing.binding == binding && existing.type == inputType)
					{
						alreadyPresent = true;
						break;
					}
				}

				if (alreadyPresent)
					continue;

				SceneryEditorX::ShaderInput input{};
				input.stage = stage;
				input.set = descriptorSet;
				input.binding = binding;
				input.count = ConvertDescriptorCount(typeLayout->getDescriptorSetDescriptorRangeDescriptorCount(setIndex, rangeIndex));
				input.type = inputType;
				input.debugName = debugName;

				// Explicitly enforce known set-0 resource contracts used across compute/graphics passes.
				if (descriptorSet == 0)
				{
					if (input.debugName == "BufferFrame" && input.type != SceneryEditorX::ShaderInputType::UniformBuffer)
					{
						SEDX_CORE_WARN_TAG("ShaderCompiler", "Reflection override: '{}' set={} binding={} {} -> UniformBuffer", input.debugName, descriptorSet, binding, ShaderInputTypeToString(input.type));
						input.type = SceneryEditorX::ShaderInputType::UniformBuffer;
					}

					if (input.debugName.rfind("tex_uav", 0) == 0 && input.type != SceneryEditorX::ShaderInputType::StorageImage)
					{
						SEDX_CORE_WARN_TAG("ShaderCompiler", "Reflection override: '{}' set={} binding={} {} -> StorageImage", input.debugName, descriptorSet, binding, ShaderInputTypeToString(input.type));
						input.type = SceneryEditorX::ShaderInputType::StorageImage;
					}

					if (const std::optional<SceneryEditorX::ShaderInputType> knownType = get_known_set0_type_override(input.debugName); knownType.has_value() && input.type != *knownType)
					{
						input.type = *knownType;
					}
				}

				SEDX_CORE_TRACE_TAG("ShaderCompiler", "Reflect descriptor: stage={} set={} binding={} name='{}' reflected={} -> {}",
					static_cast<uint32_t>(stage), descriptorSet, binding, input.debugName,
					ShaderInputTypeToString(input.type), ShaderInputTypeToVkDescriptorString(input.type));
				outInputs.push_back(input);
			}
		}
	}

	std::vector<uint32_t> CompileShader(SceneryEditorX::StageType stage, const std::string& filepath, bool optimize)
	{
		(void)optimize;

		Slang::ComPtr<slang::IGlobalSession> globalSession;
		Slang::ComPtr<slang::ISession> session;

		if (!CreateSlangSession(globalSession, session))
			return {};

		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		Slang::ComPtr<slang::IModule> module = LoadSlangModule(session, filepath, diagnosticsBlob);
		if (!module)
			return {};

		if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
		{
			SEDX_CORE_INFO_TAG("ShaderCompiler", "Slang shader compile diagnostics for '{}': {}", filepath, static_cast<const char*>(diagnosticsBlob->getBufferPointer()));
		}

		Slang::ComPtr<slang::IEntryPoint> entryPoint;
		if (!FindStageEntryPoint(module, stage, entryPoint))
			return {};

		slang::IComponentType* componentTypes[] = { module.get(), entryPoint.get() };
		Slang::ComPtr<slang::IComponentType> composedProgram;
		if (SLANG_FAILED(session->createCompositeComponentType(componentTypes, 2, composedProgram.writeRef(), diagnosticsBlob.writeRef())) || !composedProgram)
		{
			if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
			{
				SEDX_CORE_ERROR_TAG("ShaderCompiler", "Failed to compose Slang program '{}': {}", filepath, static_cast<const char*>(diagnosticsBlob->getBufferPointer()));
			}
			else
			{
				SEDX_CORE_ERROR_TAG("ShaderCompiler", "Failed to compose Slang program '{}'", filepath);
			}
			return {};
		}

		Slang::ComPtr<slang::IComponentType> linkedProgram;
		if (SLANG_FAILED(composedProgram->link(linkedProgram.writeRef(), diagnosticsBlob.writeRef())) || !linkedProgram)
		{
			if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
			{
				SEDX_CORE_ERROR_TAG("ShaderCompiler", "Failed to link Slang program '{}': {}", filepath, static_cast<const char*>(diagnosticsBlob->getBufferPointer()));
			}
			else
			{
				SEDX_CORE_ERROR_TAG("ShaderCompiler", "Failed to link Slang program '{}'", filepath);
			}
			return {};
		}

		Slang::ComPtr<ISlangBlob> spirv;
		if (SLANG_FAILED(linkedProgram->getTargetCode(0, spirv.writeRef())) || !spirv)
		{
			SEDX_CORE_ERROR_TAG("ShaderCompiler", "Failed to retrieve SPIR-V from Slang program: {}", filepath);
			return {};
		}

		const size_t byteSize = spirv->getBufferSize();
		if (byteSize == 0 || (byteSize % sizeof(uint32_t)) != 0)
		{
			SEDX_CORE_ERROR_TAG("ShaderCompiler", "Invalid SPIR-V blob size from Slang for {}", filepath);
			return {};
		}

		std::vector<uint32_t> byteCode(byteSize / sizeof(uint32_t));
		std::memcpy(byteCode.data(), spirv->getBufferPointer(), byteSize);
		return byteCode;
	}

	bool CompileSlangModule(const std::string& filepath)
	{
		Slang::ComPtr<slang::IGlobalSession> globalSession;
		Slang::ComPtr<slang::ISession> session;

		if (!CreateSlangSession(globalSession, session))
			return false;

		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		Slang::ComPtr<slang::IModule> module = LoadSlangModule(session, filepath, diagnosticsBlob);

		if (!module)
			return false;

		if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
		{
			SEDX_CORE_INFO_TAG("ShaderCompiler", "Slang module compile diagnostics for '{}': {}", filepath, static_cast<const char*>(diagnosticsBlob->getBufferPointer()));
		}

		SEDX_CORE_INFO_TAG("ShaderCompiler", "Slang module compiled successfully: {}", filepath);
		return true;
	}
	
	std::vector<SceneryEditorX::ShaderInput> Reflect(const SceneryEditorX::StageType stage, const std::string& filepath)
	{
		std::vector<SceneryEditorX::ShaderInput> shaderInputs;

		Slang::ComPtr<slang::IGlobalSession> globalSession;
		Slang::ComPtr<slang::ISession> session;
		if (!CreateSlangSession(globalSession, session))
			return shaderInputs;

		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
		Slang::ComPtr<slang::IModule> module = LoadSlangModule(session, filepath, diagnosticsBlob);
		if (!module)
			return shaderInputs;

		if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
		{
			SEDX_CORE_INFO_TAG("ShaderCompiler", "Slang reflection diagnostics for '{}': {}", filepath, static_cast<const char*>(diagnosticsBlob->getBufferPointer()));
		}

		Slang::ComPtr<slang::IEntryPoint> entryPoint;
		if (!FindStageEntryPoint(module, stage, entryPoint))
			return shaderInputs;

		slang::IComponentType* componentTypes[] = { module.get(), entryPoint.get() };
		Slang::ComPtr<slang::IComponentType> composedProgram;
		if (SLANG_FAILED(session->createCompositeComponentType(componentTypes, 2, composedProgram.writeRef(), diagnosticsBlob.writeRef())) || !composedProgram)
		{
			if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
			{
				SEDX_CORE_ERROR_TAG("ShaderCompiler", "Failed to compose Slang program for reflection '{}': {}", filepath, static_cast<const char*>(diagnosticsBlob->getBufferPointer()));
			}
			else
			{
				SEDX_CORE_ERROR_TAG("ShaderCompiler", "Failed to compose Slang program for reflection '{}'", filepath);
			}
			return shaderInputs;
		}

		Slang::ComPtr<slang::IComponentType> linkedProgram;
		if (SLANG_FAILED(composedProgram->link(linkedProgram.writeRef(), diagnosticsBlob.writeRef())) || !linkedProgram)
		{
			if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
			{
				SEDX_CORE_ERROR_TAG("ShaderCompiler", "Failed to link Slang program for reflection '{}': {}", filepath, static_cast<const char*>(diagnosticsBlob->getBufferPointer()));
			}
			else
			{
				SEDX_CORE_ERROR_TAG("ShaderCompiler", "Failed to link Slang program for reflection '{}'", filepath);
			}
			return shaderInputs;
		}

		slang::ProgramLayout* programLayout = linkedProgram->getLayout(0, diagnosticsBlob.writeRef());
		if (!programLayout)
		{
			if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0)
			{
				SEDX_CORE_ERROR_TAG("ShaderCompiler", "Failed to acquire Slang program layout for '{}': {}", filepath, static_cast<const char*>(diagnosticsBlob->getBufferPointer()));
			}
			else
			{
				SEDX_CORE_ERROR_TAG("ShaderCompiler", "Failed to acquire Slang program layout for '{}'", filepath);
			}
			return shaderInputs;
		}

		if (slang::VariableLayoutReflection* globalParams = programLayout->getGlobalParamsVarLayout())
		{
			AppendDescriptorRanges(globalParams->getTypeLayout(), stage, shaderInputs);
		}

		const SlangStage expectedStage = ToSlangStage(stage);
		for (SlangUInt entryIndex = 0; entryIndex < programLayout->getEntryPointCount(); ++entryIndex)
		{
			slang::EntryPointReflection* entryLayout = programLayout->getEntryPointByIndex(entryIndex);
			if (!entryLayout)
				continue;

			if (entryLayout->getStage() != expectedStage)
				continue;

			if (slang::VariableLayoutReflection* entryVars = entryLayout->getVarLayout())
			{
				AppendDescriptorRanges(entryVars->getTypeLayout(), stage, shaderInputs);
			}
		}

		return shaderInputs;
	}

}

// --------------------------------------------------------------

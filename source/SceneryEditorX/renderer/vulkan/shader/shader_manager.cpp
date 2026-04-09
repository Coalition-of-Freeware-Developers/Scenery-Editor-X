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
#include <array>
#include <filesystem>
#include <fstream>
#include <utility>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <slang/slang-com-ptr.h>
#include <slang/slang.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	std::unordered_map<std::string, Ref<Shader>> ShaderManager::m_Shaders;

/**
	 * @brief Creates a Vulkan shader module from the given SPIR-V bytecode.
	 * @param spirvCode Pointer to the SPIR-V bytecode.
	 * @param codeSize Size of the SPIR-V bytecode in bytes.
	 * @return The created Vulkan shader module.
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
		SEDX_VK_RESULT_ASSERT(vkCreateShaderModule(device->GetLogicalDevice(), &createInfo, nullptr, &module),
			"Failed to create shader module");
		return module;
	}

	ShaderManager::ShaderManager(const void *spirvCode, size_t codeSize)
	{
		// Use the same module for vertex and fragment stages by default.
		m_Stages.reserve(2);
		m_Modules.reserve(2);
		m_Stages.push_back(VK_SHADER_STAGE_VERTEX_BIT);
		m_Modules.push_back(CreateShaderModule(spirvCode, codeSize));
		m_Stages.push_back(VK_SHADER_STAGE_FRAGMENT_BIT);
		m_Modules.push_back(CreateShaderModule(spirvCode, codeSize));

	   // Mark as compiled only if all modules were created successfully
	   const bool allValid = std::ranges::all_of(m_Modules, [](VkShaderModule m) { return m != VK_NULL_HANDLE; });
	   m_CompilationState = allValid ? ShaderCompiler::State::Succeeded : ShaderCompiler::State::Failed;
	}

	ShaderManager::ShaderManager(const std::vector<std::pair<VkShaderStageFlagBits, std::pair<const void *, size_t>>> &stages)
	{
		m_Stages.reserve(stages.size());
		m_Modules.reserve(stages.size());
		for (const auto &s : stages)
		{
			m_Stages.push_back(s.first);
			m_Modules.push_back(CreateShaderModule(s.second.first, s.second.second));
		}

	   // Mark as compiled only if all modules were created successfully
	   const bool allValid = !m_Modules.empty() && std::ranges::all_of(m_Modules, [](VkShaderModule m) { return m != VK_NULL_HANDLE; });
	   m_CompilationState = allValid ? ShaderCompiler::State::Succeeded : ShaderCompiler::State::Failed;
	}

	ShaderManager::~ShaderManager()
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();
		if (!device.IsValid())
		{
			return;
		}

		for (VkShaderModule module : m_Modules)
		{
			if (module != VK_NULL_HANDLE)
			{
				vkDestroyShaderModule(device->GetLogicalDevice(), module, nullptr);
			}
		}
		m_Modules.clear();
	}
	
	Ref<Shader> &ShaderManager::CreateShader(const std::string &name)
	{
		if (m_Shaders.contains(name))
		{
			return m_Shaders[name];
		}

		m_Shaders[name] = CreateRef<Shader>();
		return m_Shaders[name];
	}
	
	Ref<Shader> &ShaderManager::GetShader(const std::string &name)
	{
		SEDX_CORE_ASSERT(m_Shaders.contains(name), "Shader {} is not present", name.c_str());
		return m_Shaders[name];
	}
	
	void ShaderManager::Clear()
	{
		m_Shaders.clear();
	}


} // namespace SceneryEditorX

#pragma region Slang Compilation and Reflection

namespace ShaderCompiler
{
	// -------------------------------------------------------
	// Internal helpers
	// -------------------------------------------------------

	/**
	 * @brief Returns the conventional Slang entry-point name for the given stage.
	 * Slang shaders use named entry points (main_vs, main_frag, etc.) instead of "main".
	 * Non-Slang files fall back to "main".
	 */
	static const char* GetEntryPointName(const SceneryEditorX::Stage stage, const std::string& filepath)
	{
		if (!filepath.ends_with(".slang"))
			return "main";

		switch (stage)
		{
			case SceneryEditorX::Stage::Vertex:                 return "main_vs";
			case SceneryEditorX::Stage::Fragment:               return "main_frag";
			case SceneryEditorX::Stage::Compute:                return "main_comp";
			case SceneryEditorX::Stage::Geometry:               return "main_geo";
			case SceneryEditorX::Stage::TessellationControl:    return "main_tcs";
			case SceneryEditorX::Stage::TessellationEvaluation: return "main_tes";
			default:                                            return "main";
		}
	}

	/**
	 * @brief Creates a configured Slang global session and per-compilation session targeting
	 * SPIR-V 1.4 with direct emission, Vulkan column-major matrix layout, and preserved
	 * entry point names in the emitted SPIR-V binary.
	 *
	 * `VulkanUseEntryPointName` instructs the SPIR-V backend to keep source-level entry point
	 * names (e.g. "main_vs", "main_frag") as the `OpEntryPoint` name in the binary, which is
	 * required because `VkPipelineShaderStageCreateInfo::pName` must match that name exactly.
	 * Without this option Slang normalises all entry points to "main", causing Vulkan
	 * validation error VUID-VkPipelineShaderStageCreateInfo-pName-00707.
	 *
	 * @param shaderDir  Directory to add as a search path so `import` statements can resolve
	 *                   sibling modules without requiring fully-qualified paths.
	 * @return true on success, false if either session creation step fails.
	 */
	static bool CreateSlangSession(Slang::ComPtr<slang::IGlobalSession>& outGlobal, Slang::ComPtr<slang::ISession>& outSession, const std::string& shaderDir = {})
	{
		if (SLANG_FAILED(slang::createGlobalSession(outGlobal.writeRef())))
		{
			SEDX_CORE_ERROR_TAG("Shader", "Failed to create Slang global session");
			return false;
		}

		auto targets = std::to_array<slang::TargetDesc>({{
			.format  = SLANG_SPIRV,
			.profile = outGlobal->findProfile("spirv_1_4")
		}});

		auto options = std::to_array<slang::CompilerOptionEntry>({
			{
				.name  = slang::CompilerOptionName::EmitSpirvDirectly,
				.value = {.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1}
			},
			{
				.name  = slang::CompilerOptionName::VulkanUseEntryPointName,
				.value = {.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1}
			}
		});

		// Build search paths: always include the shader source directory so that
		// `import color;` inside common.slang resolves to the sibling color.slang.
		std::vector<const char*> searchPaths;
		if (!shaderDir.empty())
		{
		    searchPaths.push_back(shaderDir.c_str());
		}

		slang::SessionDesc desc{};
		desc.targets                  = targets.data();
		desc.targetCount              = static_cast<SlangInt>(targets.size());
		desc.defaultMatrixLayoutMode  = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
		desc.compilerOptionEntries    = options.data();
		desc.compilerOptionEntryCount = static_cast<uint32_t>(options.size());
		desc.searchPaths              = searchPaths.empty() ? nullptr : searchPaths.data();
		desc.searchPathCount          = static_cast<SlangInt>(searchPaths.size());

		if (SLANG_FAILED(outGlobal->createSession(desc, outSession.writeRef())))
		{
			SEDX_CORE_ERROR_TAG("Shader", "Failed to create Slang compilation session");
			return false;
		}

		return true;
	}

	/**
	 * @brief Returns the .slang-module cache path for a given shader source file.
	 * Mirrors the .spv cache convention: cache/<shaderStem>.slang-module.
	 * @param filepath Full or relative path to the .slang source file.
	 */
	static std::filesystem::path GetModuleCachePath(const std::string& filepath)
	{
		const std::string shaderStem = filepath.substr(0, filepath.rfind('.'));
		return std::filesystem::path("cache") / (shaderStem + ".slang-module");
	}

	/**
	 * @brief Loads a Slang module from the .slang-module cache when it is fresh, or falls
	 * back to source compilation and writes a new cache entry for subsequent loads.
	 *
	 * The cache hit path uses the Slang C-API `slang_loadModuleFromIRBlob` which accepts raw
	 * bytes, avoiding the need to create an ISlangBlob wrapper for the cached data.
	 *
	 * @param session  An already-created Slang session.
	 * @param filepath Full or relative path to the .slang source file.
	 * @return A valid IModule on success, or an empty ComPtr on failure.
	 */
	static Slang::ComPtr<slang::IModule> LoadOrCompileModule(slang::ISession* session, const std::string& filepath)
	{
		const std::filesystem::path srcPath(filepath);
		const std::string            moduleName  = srcPath.stem().string();
		const std::filesystem::path  moduleCache = GetModuleCachePath(filepath);

		Slang::ComPtr<slang::IBlob> diagBlob;

		// ---- Cache hit path -------------------------------------------------------
		const bool srcExists   = std::filesystem::exists(filepath);
		const bool cacheExists = std::filesystem::exists(moduleCache);

		if (cacheExists && srcExists)
		{
			const auto cacheTime = std::filesystem::last_write_time(moduleCache);
			const auto srcTime   = std::filesystem::last_write_time(filepath);

			if (cacheTime >= srcTime)
			{
				std::ifstream cacheFile(moduleCache, std::ios::binary | std::ios::ate);
				if (cacheFile.is_open())
				{
					const auto fileSize = static_cast<size_t>(cacheFile.tellg());
					cacheFile.seekg(0);
					std::vector<char> cacheData(fileSize);
					cacheFile.read(cacheData.data(), static_cast<std::streamsize>(fileSize));

					// Use the C-API overload that takes raw void*/size_t so we do not need
					// to implement an ISlangBlob wrapper around the cached bytes.
					Slang::ComPtr<slang::IModule> cachedModule(
						slang_loadModuleFromIRBlob(session, moduleName.c_str(), filepath.c_str(),
							cacheData.data(), fileSize, diagBlob.writeRef()));

					if (cachedModule)
					{
						SEDX_CORE_TRACE_TAG("Shader", "Loaded .slang-module from cache: {}", moduleCache.string());
						return cachedModule;
					}

					SEDX_CORE_WARN_TAG("Shader", "Corrupt or incompatible .slang-module cache for '{}'; recompiling", filepath);
				}
			}
		}

		// ---- Source compilation path ---------------------------------------------
		Slang::ComPtr<slang::IModule> module(session->loadModuleFromSource(moduleName.c_str(), filepath.c_str(), nullptr, diagBlob.writeRef()));

		if (!module)
		{
			if (diagBlob)
			{
				SEDX_CORE_ERROR_TAG("Shader", "Slang compilation failed for '{}': {}", filepath, static_cast<const char*>(diagBlob->getBufferPointer()));
			}
			else
			{
				SEDX_CORE_ERROR_TAG("Shader", "Slang compilation failed for '{}' (no diagnostics available)", filepath);
			}
			return {};
		}

		// Serialize the freshly compiled module to disk for the next invocation
		Slang::ComPtr<slang::IBlob> serializedBlob;
		if (SLANG_SUCCEEDED(module->serialize(serializedBlob.writeRef())) && serializedBlob)
		{
			std::filesystem::create_directories(moduleCache.parent_path());
			std::ofstream outFile(moduleCache, std::ios::binary);
			if (outFile.is_open())
			{
				outFile.write(
					static_cast<const char*>(serializedBlob->getBufferPointer()),
					static_cast<std::streamsize>(serializedBlob->getBufferSize()));
				SEDX_CORE_TRACE_TAG("Shader", "Cached .slang-module to: {}", moduleCache.string());
			}
			else
			{
				SEDX_CORE_WARN_TAG("Shader", "Could not write .slang-module cache to '{}'", moduleCache.string());
			}
		}

		return module;
	}

	/**
	 * @brief Inspects one Slang variable layout and appends a ShaderInput to outInputs if the
	 * type maps to a recognised descriptor binding category.
	 *
	 * Handles:
	 * - ConstantBuffer / ParameterBlock → UniformBuffer / UniformBufferSet
	 * - ShaderStorageBuffer → StorageBuffer
	 * - SamplerState        → Sampler
	 * - Resource            → Texture / StorageImage / StorageBuffer / CombinedImageSampler
	 *                         (distinguished by resource shape and access flags)
	 * - Array wrapping of any of the above (element count 0 = unbounded/bindless)
	 *
	 * @param varLayout  Layout of the variable to inspect.
	 * @param stage      Shader stage that owns this binding.
	 * @param outInputs  Destination vector for collected ShaderInput entries.
	 */
	static void CollectInputsFromVar(
		slang::VariableLayoutReflection*          varLayout,
		SceneryEditorX::Stage                     stage,
		std::vector<SceneryEditorX::ShaderInput>& outInputs)
	{
		if (!varLayout) return;

		slang::TypeLayoutReflection* typeLayout = varLayout->getTypeLayout();
		if (!typeLayout) return;

		slang::TypeReflection* type = typeLayout->getType();
		if (!type) return;

		typedef slang::TypeReflection::Kind TK;
		TK kind = type->getKind();

		// Unwrap a single array dimension and record the element count.
		// count == 0 signals an unbounded (bindless) array.
		uint32_t arrayCount = 1u;
		if (kind == TK::Array)
		{
			arrayCount  = static_cast<uint32_t>(type->getElementCount());
			typeLayout  = typeLayout->getElementTypeLayout();
			if (!typeLayout) return;
			type = typeLayout->getType();
			if (!type) return;
			kind = type->getKind();
		}

		SceneryEditorX::ShaderInputType inputType{};
		bool validBinding = true;

		switch (kind)
		{
			case TK::ConstantBuffer: inputType = SceneryEditorX::ShaderInputType::UniformBuffer;
				break;

			case TK::ParameterBlock: inputType = SceneryEditorX::ShaderInputType::UniformBufferSet;
				break;

			case TK::ShaderStorageBuffer: inputType = SceneryEditorX::ShaderInputType::StorageBuffer;
				break;

			case TK::SamplerState: inputType = SceneryEditorX::ShaderInputType::Sampler;
				break;

			case TK::Resource:
			{
				const SlangResourceShape  shape      = type->getResourceShape();
				const SlangResourceAccess access     = type->getResourceAccess();
				const unsigned            baseShape  = shape & SLANG_RESOURCE_BASE_SHAPE_MASK;

				if (baseShape == SLANG_STRUCTURED_BUFFER)
				{
					inputType = SceneryEditorX::ShaderInputType::StorageBuffer;
				}
				else if (shape & SLANG_TEXTURE_COMBINED_FLAG)
				{
					inputType = SceneryEditorX::ShaderInputType::CombinedImageSampler;
				}
				else if (baseShape >= SLANG_TEXTURE_1D && baseShape <= SLANG_TEXTURE_SUBPASS)
				{
					inputType = (access == SLANG_RESOURCE_ACCESS_READ_WRITE || access == SLANG_RESOURCE_ACCESS_WRITE)
						? SceneryEditorX::ShaderInputType::StorageImage
						: SceneryEditorX::ShaderInputType::Texture;
				}
				else
				{
					validBinding = false;
				}
				break;
			}

			default:
				validBinding = false;
				break;
		}

		if (!validBinding) return;

		SceneryEditorX::ShaderInput input{};
		input.debugName = varLayout->getName() ? varLayout->getName() : "";
		input.stage     = stage;
		input.set       = varLayout->getBindingSpace();
		input.binding   = varLayout->getBindingIndex();
		input.count     = arrayCount;
		input.type      = inputType;

		outInputs.push_back(input);
	}

	// -------------------------------------------------------
	// Public API
	// -------------------------------------------------------

	ShaderCompilationResult CompileAndReflect(SceneryEditorX::Stage stage, const std::string& filepath, bool optimize)
	{
		// TODO: Implement optimization passes in the Slang compilation pipeline and set the 'optimize' flag accordingly.
		(void)optimize;

		ShaderCompilationResult result;

		// Derive the shader directory from the filepath so sibling module imports resolve correctly.
		std::string shaderDir;
		const std::filesystem::path fsPath(filepath);
		if (fsPath.has_parent_path())
		{
		    shaderDir = fsPath.parent_path().string();
		}

		// 1. Create a Slang session shared for both compilation and reflection
		Slang::ComPtr<slang::IGlobalSession> globalSession;
		Slang::ComPtr<slang::ISession>       session;
		if (!CreateSlangSession(globalSession, session, shaderDir))
			return result;

		// 2. Load module from .slang-module cache or compile from source
		Slang::ComPtr<slang::IModule> module = LoadOrCompileModule(session, filepath);
		if (!module)
			return result;

		// 3. Locate the entry point for this specific stage
		const char* entryName = GetEntryPointName(stage, filepath);
		Slang::ComPtr<slang::IEntryPoint> entryPoint;
		module->findEntryPointByName(entryName, entryPoint.writeRef());

		if (!entryPoint)
		{
			SEDX_CORE_ERROR_TAG("Shader", "Entry point '{}' not found in module '{}'", entryName, filepath);
			return result;
		}

		// 4. Compose module + entry point, then link to resolve cross-module references
		slang::IComponentType* components[] = { module.get(), entryPoint.get() };
		Slang::ComPtr<slang::IComponentType> composed;
		Slang::ComPtr<slang::IBlob>          diagBlob;
		session->createCompositeComponentType(components, 2, composed.writeRef(), diagBlob.writeRef());

		if (!composed)
		{
			SEDX_CORE_ERROR_TAG("Shader", "Failed to compose shader components for '{}'", filepath);
			return result;
		}

		Slang::ComPtr<slang::IComponentType> linked;
		composed->link(linked.writeRef(), diagBlob.writeRef());

		if (!linked)
		{
			if (diagBlob)
			{
				SEDX_CORE_ERROR_TAG("Shader", "Shader link failed for '{}': {}",
					filepath, static_cast<const char*>(diagBlob->getBufferPointer()));
			}
			return result;
		}

		// 5. Extract per-entry-point SPIR-V words
		Slang::ComPtr<slang::IBlob> spirvBlob;
		if (SLANG_SUCCEEDED(linked->getEntryPointCode(0, 0, spirvBlob.writeRef(), diagBlob.writeRef())) && spirvBlob)
		{
			const size_t byteSize = spirvBlob->getBufferSize();
			if (byteSize > 0 && (byteSize % sizeof(uint32_t)) == 0)
			{
				result.spirv.resize(byteSize / sizeof(uint32_t));
				std::memcpy(result.spirv.data(), spirvBlob->getBufferPointer(), byteSize);
			}
			else
			{
				SEDX_CORE_ERROR_TAG("Shader", "SPIR-V blob has invalid size ({} bytes) for '{}'", byteSize, filepath);
			}
		}
		else if (diagBlob)
		{
			SEDX_CORE_ERROR_TAG("Shader", "SPIR-V code generation failed for '{}': {}",
				filepath, static_cast<const char*>(diagBlob->getBufferPointer()));
		}

		// 6. Reflect global shader parameters from the linked program layout
		slang::ProgramLayout* layout = linked->getLayout(0, diagBlob.writeRef());
		if (layout)
		{
			const unsigned paramCount = layout->getParameterCount();
			for (unsigned i = 0; i < paramCount; ++i)
				CollectInputsFromVar(layout->getParameterByIndex(i), stage, result.inputs);
		}

		return result;
	}

	std::vector<uint32_t> CompileVulkanShader(SceneryEditorX::Stage stage, const std::string& filepath, bool optimize)
	{
		return CompileAndReflect(stage, filepath, optimize).spirv;
	}

	std::vector<SceneryEditorX::ShaderInput> Reflect(SceneryEditorX::Stage stage, const std::string& filepath)
	{
		return CompileAndReflect(stage, filepath).inputs;
	}

}

#pragma endregion

// -------------------------------------------------------

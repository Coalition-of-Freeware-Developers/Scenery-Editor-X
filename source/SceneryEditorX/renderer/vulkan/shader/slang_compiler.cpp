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
 * slang_compiler.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "slang_compiler.h"
#include "shader_cache.h"
#include "shader_reflection.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <format>
#include <fstream>
#include <iterator>
#include <mutex>
#include <set>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/identifiers/hash.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <slang/slang-com-ptr.h>
#include <slang/slang.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		// Bump when the cached program layout or the reflection rules below change.
		constexpr uint32_t kCompilerCacheRevision = 1;

		std::mutex s_CompilerMutex; // Slang global sessions are not thread-safe
		Slang::ComPtr<slang::IGlobalSession> s_GlobalSession;

		[[nodiscard]] slang::IGlobalSession* AcquireGlobalSession()
		{
			if (!s_GlobalSession && SLANG_FAILED(slang::createGlobalSession(s_GlobalSession.writeRef())))
			{
				SEDX_CORE_ERROR_TAG("SlangCompiler", "Failed to create the Slang global session");
				s_GlobalSession.setNull();
			}

			return s_GlobalSession.get();
		}

		[[nodiscard]] std::string BlobToString(slang::IBlob* blob)
		{
			if (!blob || blob->getBufferSize() == 0)
				return {};

			return {static_cast<const char*>(blob->getBufferPointer()), blob->getBufferSize()};
		}

		[[nodiscard]] bool IsRecompileRequested()
		{
			const PlatformContext* context = Application::Get().GetPlatformContext();
			if (!context)
				return false;

			return std::ranges::any_of(context->GetCommandLineArgs(), [](const std::string& arg)
			{
				return arg == "--recompile-shaders";
			});
		}

		[[nodiscard]] const char* StageName(ShaderType type)
		{
			switch (type)
			{
				case ShaderType::Vertex:                 return "vertex";
				case ShaderType::TessellationControl:    return "tessellation control";
				case ShaderType::TessellationEvaluation: return "tessellation evaluation";
				case ShaderType::Geometry:               return "geometry";
				case ShaderType::Fragment:               return "fragment";
				case ShaderType::Compute:                return "compute";
				case ShaderType::Task:                   return "task";
				case ShaderType::Mesh:                   return "mesh";
				default:                                 return "unknown";
			}
		}

		[[nodiscard]] SlangStage ToSlangStage(ShaderType type)
		{
			switch (type)
			{
				case ShaderType::Vertex:                 return SLANG_STAGE_VERTEX;
				case ShaderType::TessellationControl:    return SLANG_STAGE_HULL;
				case ShaderType::TessellationEvaluation: return SLANG_STAGE_DOMAIN;
				case ShaderType::Geometry:               return SLANG_STAGE_GEOMETRY;
				case ShaderType::Fragment:               return SLANG_STAGE_FRAGMENT;
				case ShaderType::Compute:                return SLANG_STAGE_COMPUTE;
				case ShaderType::Task:                   return SLANG_STAGE_AMPLIFICATION;
				case ShaderType::Mesh:                   return SLANG_STAGE_MESH;
				case ShaderType::RayGeneration:          return SLANG_STAGE_RAY_GENERATION;
				case ShaderType::AnyHit:                 return SLANG_STAGE_ANY_HIT;
				case ShaderType::ClosestHit:             return SLANG_STAGE_CLOSEST_HIT;
				case ShaderType::Miss:                   return SLANG_STAGE_MISS;
				case ShaderType::Intersection:           return SLANG_STAGE_INTERSECTION;
				case ShaderType::Callable:               return SLANG_STAGE_CALLABLE;
				default:                                 return SLANG_STAGE_NONE;
			}
		}

		[[nodiscard]] ShaderType FromSlangStage(SlangStage stage)
		{
			switch (stage)
			{
				case SLANG_STAGE_VERTEX:         return ShaderType::Vertex;
				case SLANG_STAGE_HULL:           return ShaderType::TessellationControl;
				case SLANG_STAGE_DOMAIN:         return ShaderType::TessellationEvaluation;
				case SLANG_STAGE_GEOMETRY:       return ShaderType::Geometry;
				case SLANG_STAGE_FRAGMENT:       return ShaderType::Fragment;
				case SLANG_STAGE_COMPUTE:        return ShaderType::Compute;
				case SLANG_STAGE_AMPLIFICATION:  return ShaderType::Task;
				case SLANG_STAGE_MESH:           return ShaderType::Mesh;
				case SLANG_STAGE_RAY_GENERATION: return ShaderType::RayGeneration;
				case SLANG_STAGE_ANY_HIT:        return ShaderType::AnyHit;
				case SLANG_STAGE_CLOSEST_HIT:    return ShaderType::ClosestHit;
				case SLANG_STAGE_MISS:           return ShaderType::Miss;
				case SLANG_STAGE_INTERSECTION:   return ShaderType::Intersection;
				case SLANG_STAGE_CALLABLE:       return ShaderType::Callable;
				default:                         return ShaderType::None;
			}
		}

		/**
		 * @brief Entry-point names tried per stage. resources/shaders uses main_vs / main_frag / main_comp / main_tesc / main_tese.
		 */
		[[nodiscard]] std::vector<const char*> GetEntryPointCandidates(ShaderType type)
		{
			switch (type)
			{
				case ShaderType::Vertex:                 return {"main_vs", "main"};
				case ShaderType::TessellationControl:    return {"main_tesc", "main"};
				case ShaderType::TessellationEvaluation: return {"main_tese", "main"};
				case ShaderType::Geometry:               return {"main_geom", "main"};
				case ShaderType::Fragment:               return {"main_frag", "main_ps", "main"};
				case ShaderType::Compute:                return {"main_comp", "main_cs", "main"};
				default:                                 return {"main"};
			}
		}

		/**
		 * @brief Maps a Slang binding range type to the engine descriptor type.
		 * @return DescriptorType::MaxEnum for binding kinds the renderer cannot bind (texel buffers, acceleration structures).
		 */
		[[nodiscard]] DescriptorType ToDescriptorType(slang::BindingType type)
		{
			switch (type)
			{
				case slang::BindingType::Texture:
				case slang::BindingType::CombinedTextureSampler:
				case slang::BindingType::InputRenderTarget:      return DescriptorType::Image;
				case slang::BindingType::MutableTexture:         return DescriptorType::TextureStorage;
				case slang::BindingType::ConstantBuffer:         return DescriptorType::ConstantBuffer;
				case slang::BindingType::RawBuffer:
				case slang::BindingType::MutableRawBuffer:       return DescriptorType::StructuredBuffer;
				case slang::BindingType::Sampler:                return DescriptorType::Sampler;
				default:                                         return DescriptorType::MaxEnum;
			}
		}

		[[nodiscard]] std::string NormalizeSemantic(const char* semantic)
		{
			std::string name(semantic);
			std::ranges::transform(name, name.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
			while (!name.empty() && std::isdigit(static_cast<unsigned char>(name.back())))
				name.pop_back();
			return name;
		}

		void CollectVertexSemantics(slang::VariableLayoutReflection* var, std::set<std::string>& outSemantics)
		{
			if (!var)
				return;

			slang::TypeLayoutReflection* typeLayout = var->getTypeLayout();
			if (typeLayout && typeLayout->getKind() == slang::TypeReflection::Kind::Struct)
			{
				for (unsigned int i = 0; i < typeLayout->getFieldCount(); ++i)
					CollectVertexSemantics(typeLayout->getFieldByIndex(i), outSemantics);
				return;
			}

			if (const char* semantic = var->getSemanticName(); semantic && semantic[0] != '\0')
			{
				std::string name = NormalizeSemantic(semantic);
				if (!name.starts_with("SV_"))
					outSemantics.insert(std::move(name));
			}
		}

		/**
		 * @brief Picks the InputLayout matching the vertex-stage input semantics.
		 * @return MaxEnum when the stage has no vertex attributes (vertex pulling via SV_VertexID).
		 */
		[[nodiscard]] VertexType InferVertexType(const std::set<std::string>& semantics)
		{
			if (semantics.empty())
				return VertexType::MaxEnum;

			const bool hasTexcoord = semantics.contains("TEXCOORD");
			const bool hasColor = semantics.contains("COLOR");
			if (semantics.contains("NORMAL") || semantics.contains("TANGENT"))
				return VertexType::PositionUvNormalTangent;
			if (hasColor && hasTexcoord)
				return VertexType::Position2dUvColor8;
			if (hasColor)
				return VertexType::PositionColor;
			if (hasTexcoord)
				return VertexType::PositionUv;
			return VertexType::Position;
		}

		[[nodiscard]] std::vector<std::filesystem::path> CollectDependencies(slang::IModule* module, const std::filesystem::path& sourcePath)
		{
			std::vector<std::filesystem::path> dependencies;
			auto add = [&dependencies](const std::filesystem::path& path)
			{
				std::error_code ec;
				if (!std::filesystem::exists(path, ec))
					return; // Built-in / virtual modules have no file to watch

				std::filesystem::path canonical = std::filesystem::weakly_canonical(path, ec);
				if (ec)
					canonical = path;

				if (std::ranges::find(dependencies, canonical) == dependencies.end())
					dependencies.push_back(std::move(canonical));
			};

			add(sourcePath);
			for (SlangInt32 i = 0; i < module->getDependencyFileCount(); ++i)
			{
				if (const char* dependency = module->getDependencyFilePath(i); dependency && dependency[0] != '\0')
					add(dependency);
			}

			return dependencies;
		}

		[[nodiscard]] std::filesystem::path GetProgramCacheFile(const ShaderCompileOptions& options, const std::filesystem::path& sourcePath)
		{
			std::string key = std::format("{}|{}|{}|{}|{}", options.name, sourcePath.generic_string(), options.disableOptimization,
										  kCompilerCacheRevision, spGetBuildTagString());
			for (const ShaderType stage : options.stages)
				key += std::format("|s{}", static_cast<uint32_t>(stage));
			for (const ShaderMacro& macro : options.macros)
				key += std::format("|d{}={}", macro.name, macro.value);

			const uint64_t hash = Hash::GenerateFNV1A(std::string_view{key});
			return ShaderCache::GetProgramCacheDirectory() / std::format("{}-{:016x}.sxprog", options.name, hash);
		}

		/**
		 * @brief Reflects every global descriptor binding of the linked program.
		 * Stage flags come from per-entry-point usage metadata, so a binding is only visible to the stages that read it.
		 * Bindings no stage uses keep the program's full stage mask so the set layout still matches every pipeline built from it.
		 */
		void ReflectResources(slang::IComponentType* linkedProgram, slang::ProgramLayout* layout,
							  const std::vector<ShaderType>& entryStages, const std::string& programName, CachedShaderProgram& out)
		{
			std::vector<Slang::ComPtr<slang::IMetadata>> metadata(entryStages.size());
			uint32_t programStageMask = 0;
			for (size_t i = 0; i < entryStages.size(); ++i)
			{
				programStageMask |= static_cast<uint32_t>(ShaderTypeToVkStageFlags(entryStages[i]));
				if (SLANG_FAILED(linkedProgram->getEntryPointMetadata(static_cast<SlangInt>(i), 0, metadata[i].writeRef())))
					metadata[i].setNull();
			}

			for (unsigned int paramIndex = 0; paramIndex < layout->getParameterCount(); ++paramIndex)
			{
				slang::VariableLayoutReflection* param = layout->getParameterByIndex(paramIndex);
				if (!param)
					continue;

				slang::TypeLayoutReflection* typeLayout = param->getTypeLayout();
				const slang::ParameterCategory category = param->getCategory();

				if (category == slang::ParameterCategory::PushConstantBuffer)
				{
					slang::TypeLayoutReflection* elementLayout = typeLayout ? typeLayout->getElementTypeLayout() : nullptr;
					const size_t size = elementLayout ? elementLayout->getSize() : (typeLayout ? typeLayout->getSize() : 0);
					out.pushConstantSize = std::max(out.pushConstantSize, static_cast<uint32_t>(size));
					continue;
				}

				bool hasDescriptorSlot = category == slang::ParameterCategory::DescriptorTableSlot;
				for (unsigned int c = 0; !hasDescriptorSlot && c < param->getCategoryCount(); ++c)
					hasDescriptorSlot = param->getCategoryByIndex(c) == slang::ParameterCategory::DescriptorTableSlot;

				if (!hasDescriptorSlot || !typeLayout || typeLayout->getBindingRangeCount() == 0)
					continue;

				const char* rawName = param->getName();
				const std::string name = rawName && rawName[0] != '\0' ? rawName : std::format("param{}", paramIndex);

				const slang::BindingType bindingType = typeLayout->getBindingRangeType(0);
				const DescriptorType type = ToDescriptorType(bindingType);
				if (type == DescriptorType::MaxEnum)
				{
					SEDX_CORE_WARN_TAG("SlangCompiler", "Shader '{}': resource '{}' uses an unsupported binding type ({}); it will not be bound",
						programName, name, static_cast<uint32_t>(bindingType));
					continue;
				}

				const uint32_t set = static_cast<uint32_t>(param->getBindingSpace(slang::ParameterCategory::DescriptorTableSlot));
				const uint32_t binding = static_cast<uint32_t>(param->getOffset(slang::ParameterCategory::DescriptorTableSlot));

				// 0 marks an unbounded (bindless) array.
				const SlangInt reflectedCount = typeLayout->getBindingRangeBindingCount(0);
				const uint32_t count = (reflectedCount <= 0 || reflectedCount == static_cast<SlangInt>(SLANG_UNBOUNDED_SIZE))
					? 0u : static_cast<uint32_t>(reflectedCount);

				uint32_t stageFlags = 0;
				for (size_t i = 0; i < entryStages.size(); ++i)
				{
					const uint32_t vkStage = static_cast<uint32_t>(ShaderTypeToVkStageFlags(entryStages[i]));
					bool used = true;
					if (metadata[i] && SLANG_FAILED(metadata[i]->isParameterLocationUsed(SLANG_PARAMETER_CATEGORY_DESCRIPTOR_TABLE_SLOT, set, binding, used)))
						used = true;

					if (used)
						stageFlags |= vkStage;
				}

				if (stageFlags == 0)
					stageFlags = programStageMask;

				out.resources.insert_or_assign(name, ShaderResourceDeclaration(name, set, binding, count, type, stageFlags));
			}
		}

		[[nodiscard]] bool CompileWithSlang(const ShaderCompileOptions& options, const std::filesystem::path& sourcePath, CachedShaderProgram& out)
		{
			slang::IGlobalSession* globalSession = AcquireGlobalSession();
			if (!globalSession)
				return false;

			std::ifstream file(sourcePath, std::ios::binary);
			if (!file)
			{
				SEDX_CORE_ERROR_TAG("SlangCompiler", "Shader '{}': cannot read '{}'", options.name, sourcePath.string());
				return false;
			}

			const std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			if (source.empty())
			{
				SEDX_CORE_ERROR_TAG("SlangCompiler", "Shader '{}': '{}' is empty", options.name, sourcePath.string());
				return false;
			}

			// Session: SPIR-V for Vulkan, column-major matrices, variant macros, imports resolved next to the source.
			const std::string searchPath = sourcePath.parent_path().string();
			const char* searchPaths[] = { searchPath.c_str() };

			std::vector<slang::PreprocessorMacroDesc> macros;
			macros.reserve(options.macros.size());
			for (const ShaderMacro& macro : options.macros)
				macros.push_back({macro.name.c_str(), macro.value.c_str()});

			slang::TargetDesc target{};
			target.format = SLANG_SPIRV;
			target.profile = globalSession->findProfile("spirv_1_4");

			const std::array options_ = {
				slang::CompilerOptionEntry{.name = slang::CompilerOptionName::EmitSpirvDirectly,
					.value = {.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1}},
				slang::CompilerOptionEntry{.name = slang::CompilerOptionName::Optimization,
					.value = {.kind = slang::CompilerOptionValueKind::Int,
							  .intValue0 = static_cast<int32_t>(options.disableOptimization ? SLANG_OPTIMIZATION_LEVEL_NONE : SLANG_OPTIMIZATION_LEVEL_DEFAULT)}},
			};

			slang::SessionDesc sessionDesc{};
			sessionDesc.targets = &target;
			sessionDesc.targetCount = 1;
			sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
			sessionDesc.searchPaths = searchPaths;
			sessionDesc.searchPathCount = 1;
			sessionDesc.preprocessorMacros = macros.empty() ? nullptr : macros.data();
			sessionDesc.preprocessorMacroCount = static_cast<SlangInt>(macros.size());
			sessionDesc.compilerOptionEntries = options_.data();
			sessionDesc.compilerOptionEntryCount = static_cast<uint32_t>(options_.size());

			Slang::ComPtr<slang::ISession> session;
			if (SLANG_FAILED(globalSession->createSession(sessionDesc, session.writeRef())))
			{
				SEDX_CORE_ERROR_TAG("SlangCompiler", "Shader '{}': failed to create a Slang session", options.name);
				return false;
			}

			Slang::ComPtr<slang::IBlob> sourceBlob;
			sourceBlob.attach(slang_createBlob(source.data(), source.size()));

			const std::string moduleName = sourcePath.stem().string();
			const std::string modulePath = sourcePath.string();
			Slang::ComPtr<slang::IBlob> diagnostics;
			Slang::ComPtr<slang::IModule> module;
			module = session->loadModuleFromSource(moduleName.c_str(), modulePath.c_str(), sourceBlob.get(), diagnostics.writeRef());
			if (!module)
			{
				SEDX_CORE_ERROR_TAG("SlangCompiler", "Shader '{}': failed to compile '{}':\n{}", options.name, modulePath, BlobToString(diagnostics));
				return false;
			}

			if (diagnostics && diagnostics->getBufferSize() > 0)
				SEDX_CORE_TRACE_TAG("SlangCompiler", "Shader '{}' diagnostics:\n{}", options.name, BlobToString(diagnostics));

			// Resolve the entry points: the requested stages, or every entry point the module defines.
			std::vector<Slang::ComPtr<slang::IEntryPoint>> entryPoints;
			std::vector<ShaderType> entryStages;
			if (options.stages.empty())
			{
				for (SlangInt32 i = 0; i < module->getDefinedEntryPointCount(); ++i)
				{
					Slang::ComPtr<slang::IEntryPoint> entryPoint;
					if (SLANG_FAILED(module->getDefinedEntryPoint(i, entryPoint.writeRef())) || !entryPoint)
						continue;

					slang::ProgramLayout* entryLayout = entryPoint->getLayout();
					slang::EntryPointReflection* reflection = entryLayout ? entryLayout->getEntryPointByIndex(0) : nullptr;
					const ShaderType stage = reflection ? FromSlangStage(reflection->getStage()) : ShaderType::None;
					if (stage == ShaderType::None || std::ranges::find(entryStages, stage) != entryStages.end())
						continue;

					entryPoints.push_back(entryPoint);
					entryStages.push_back(stage);
				}
			}
			else
			{
				for (const ShaderType stage : options.stages)
				{
					Slang::ComPtr<slang::IEntryPoint> entryPoint;
					for (const char* candidate : GetEntryPointCandidates(stage))
					{
						Slang::ComPtr<slang::IBlob> entryDiagnostics;
						if (SLANG_SUCCEEDED(module->findAndCheckEntryPoint(candidate, ToSlangStage(stage), entryPoint.writeRef(), entryDiagnostics.writeRef())) && entryPoint)
							break;
						entryPoint.setNull();
					}

					if (!entryPoint)
					{
						SEDX_CORE_ERROR_TAG("SlangCompiler", "Shader '{}': no {} entry point found in '{}'", options.name, StageName(stage), modulePath);
						return false;
					}

					entryPoints.push_back(entryPoint);
					entryStages.push_back(stage);
				}
			}

			if (entryPoints.empty())
			{
				SEDX_CORE_ERROR_TAG("SlangCompiler", "Shader '{}': '{}' defines no entry points", options.name, modulePath);
				return false;
			}

			std::vector<slang::IComponentType*> components;
			components.reserve(entryPoints.size() + 1);
			components.push_back(module.get());
			for (const Slang::ComPtr<slang::IEntryPoint>& entryPoint : entryPoints)
				components.push_back(entryPoint.get());

			Slang::ComPtr<slang::IComponentType> composed;
			if (SLANG_FAILED(session->createCompositeComponentType(components.data(), static_cast<SlangInt>(components.size()), composed.writeRef(), diagnostics.writeRef())))
			{
				SEDX_CORE_ERROR_TAG("SlangCompiler", "Shader '{}': failed to compose program:\n{}", options.name, BlobToString(diagnostics));
				return false;
			}

			Slang::ComPtr<slang::IComponentType> linked;
			if (SLANG_FAILED(composed->link(linked.writeRef(), diagnostics.writeRef())))
			{
				SEDX_CORE_ERROR_TAG("SlangCompiler", "Shader '{}': failed to link program:\n{}", options.name, BlobToString(diagnostics));
				return false;
			}

			CachedShaderProgram program;
			for (size_t i = 0; i < entryStages.size(); ++i)
			{
				Slang::ComPtr<slang::IBlob> code;
				if (SLANG_FAILED(linked->getEntryPointCode(static_cast<SlangInt>(i), 0, code.writeRef(), diagnostics.writeRef())) || !code)
				{
					SEDX_CORE_ERROR_TAG("SlangCompiler", "Shader '{}': {} stage code generation failed:\n{}", options.name, StageName(entryStages[i]), BlobToString(diagnostics));
					return false;
				}

				const size_t byteSize = code->getBufferSize();
				if (byteSize == 0 || byteSize % sizeof(uint32_t) != 0)
				{
					SEDX_CORE_ERROR_TAG("SlangCompiler", "Shader '{}': {} stage produced an invalid SPIR-V blob ({} bytes)", options.name, StageName(entryStages[i]), byteSize);
					return false;
				}

				std::vector<uint32_t>& spirv = program.stages[entryStages[i]];
				spirv.resize(byteSize / sizeof(uint32_t));
				std::memcpy(spirv.data(), code->getBufferPointer(), byteSize);
			}

			slang::ProgramLayout* layout = linked->getLayout(0, diagnostics.writeRef());
			if (!layout)
			{
				SEDX_CORE_ERROR_TAG("SlangCompiler", "Shader '{}': reflection unavailable:\n{}", options.name, BlobToString(diagnostics));
				return false;
			}

			ReflectResources(linked.get(), layout, entryStages, options.name, program);

			// Vertex layout: explicit registration wins, otherwise derive it from the vertex-stage input semantics.
			program.vertexType = options.vertexType;
			if (program.vertexType == VertexType::MaxEnum)
			{
				for (size_t i = 0; i < entryStages.size(); ++i)
				{
					if (entryStages[i] != ShaderType::Vertex)
						continue;

					std::set<std::string> semantics;
					if (slang::EntryPointReflection* entry = layout->getEntryPointByIndex(static_cast<SlangUInt>(i)))
					{
						for (unsigned int p = 0; p < entry->getParameterCount(); ++p)
						{
							slang::VariableLayoutReflection* param = entry->getParameterByIndex(p);
							if (param && param->getCategory() != slang::ParameterCategory::Uniform)
								CollectVertexSemantics(param, semantics);
						}
					}

					program.vertexType = InferVertexType(semantics);
				}
			}

			program.dependencies = CollectDependencies(module.get(), sourcePath);
			out = std::move(program);
			return true;
		}
	}

	Ref<Shader> SlangCompiler::BuildShader(const ShaderCompileOptions& options, const std::filesystem::path& sourcePath, CachedShaderProgram& program)
	{
		auto shader = CreateRef<Shader>();
		shader->m_Name = options.name;
		shader->m_AssetPath = sourcePath;
		shader->m_DisableOptimization = options.disableOptimization;
		shader->m_Resources = std::move(program.resources);
		shader->m_VertexType = program.vertexType;
		shader->m_PushConstantSize = program.pushConstantSize;
		shader->m_SourceDependencies = std::move(program.dependencies);
		shader->m_BuildTime = std::filesystem::file_time_type::clock::now();
		shader->m_CompileOptions = std::make_unique<ShaderCompileOptions>(options);
		shader->m_CompileOptions->sourcePath = sourcePath;
		shader->m_CompileOptions->forceCompile = false;

		shader->ValidateBindings();
		shader->LoadAndCreateShaders(program.stages);
		shader->CreateDescriptors();
		return shader;
	}

	std::filesystem::path SlangCompiler::ResolveSourcePath(const std::filesystem::path& path)
	{
		std::error_code ec;
		if (path.is_absolute())
			return std::filesystem::exists(path, ec) ? path : std::filesystem::path{};

		const std::filesystem::path cwd = std::filesystem::current_path(ec);
		const std::filesystem::path shaderDirectory = Shader::GetShaderDirectoryPath();
		const std::array<std::filesystem::path, 6> candidates = {
			cwd / path,
			cwd / shaderDirectory / path,
			cwd / ".." / path,
			cwd / ".." / shaderDirectory / path,
			cwd / ".." / ".." / path,
			cwd / ".." / ".." / shaderDirectory / path,
		};

		for (const std::filesystem::path& candidate : candidates)
		{
			if (std::filesystem::exists(candidate, ec))
				return std::filesystem::weakly_canonical(candidate, ec);
		}

		return {};
	}

	Ref<Shader> SlangCompiler::Compile(const ShaderCompileOptions& options)
	{
		const std::filesystem::path sourcePath = ResolveSourcePath(options.sourcePath);
		if (sourcePath.empty())
		{
			SEDX_CORE_ERROR_TAG("SlangCompiler", "Shader '{}': source '{}' not found", options.name, options.sourcePath.string());
			return nullptr;
		}

		std::scoped_lock lock(s_CompilerMutex);

		const std::filesystem::path cacheFile = GetProgramCacheFile(options, sourcePath);
		CachedShaderProgram program;

		if (!options.forceCompile && !IsRecompileRequested() && ShaderCache::TryLoadProgram(cacheFile, program))
		{
			SEDX_CORE_TRACE_TAG("SlangCompiler", "Shader '{}' loaded from cache", options.name);
			return BuildShader(options, sourcePath, program);
		}

		SEDX_CORE_INFO_TAG("SlangCompiler", "Compiling shader '{}' from '{}'", options.name, sourcePath.string());
		if (!CompileWithSlang(options, sourcePath, program))
			return nullptr;

		if (!ShaderCache::StoreProgram(cacheFile, program))
			SEDX_CORE_WARN_TAG("SlangCompiler", "Shader '{}': could not write cache file '{}'", options.name, VFS::ToUtf8(cacheFile));

		return BuildShader(options, sourcePath, program);
	}

	Ref<Shader> SlangCompiler::Compile(std::string_view path, bool forceCompile, bool disableOptimization)
	{
		ShaderCompileOptions options;
		options.sourcePath = std::filesystem::path(path);
		options.name = options.sourcePath.stem().string();
		options.forceCompile = forceCompile;
		options.disableOptimization = disableOptimization;
		return Compile(options);
	}

	void SlangCompiler::Shutdown()
	{
		std::scoped_lock lock(s_CompilerMutex);
		s_GlobalSession.setNull();
	}

}  // namespace SceneryEditorX

// -------------------------------------------------------

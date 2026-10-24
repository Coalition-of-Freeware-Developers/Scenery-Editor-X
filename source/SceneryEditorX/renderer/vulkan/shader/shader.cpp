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
 * shader.cpp
 * -------------------------------------------------------
 * Created: 03/07/2026
 * -------------------------------------------------------
 */
#include "../device.h"
#include "../render_context.h"
#include "shader.h"
#include "shader_cache.h"
#include "shader_reflection.h"
#include "slang_compiler.h"
#include "spirv_hash.h"
#include <SceneryEditorX/core/identifiers/hash.h>
#include <SceneryEditorX/utils/pointers.h>
#include <algorithm>
#include <fstream>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		constexpr const char* kShaderDirectoryPath = "resources/shaders/";
		constexpr const char* kSpirvEntryPoint = "main"; // Slang names every SPIR-V entry point "main" unless told otherwise
		constexpr uint32_t kSpirvMagic = 0x07230203u;

		[[nodiscard]] bool IsValidSpirv(const std::vector<uint32_t>& spirv)
		{
			return spirv.size() >= 5u && spirv.front() == kSpirvMagic;
		}

		/**
		 * @brief Locations of a module's user inputs: Input-storage variables decorated with Location (built-ins such as
		 * SV_VertexID carry BuiltIn instead). Bit n = location n. Each variable counts its first location only, which
		 * covers the scalar and vector vertex attributes the engine's vertex formats use (no matrix attributes).
		 */
		[[nodiscard]] uint32_t ReflectInputLocations(const std::vector<uint32_t>& spirv)
		{
			constexpr uint32_t kOpVariable = 59;
			constexpr uint32_t kOpDecorate = 71;
			constexpr uint32_t kDecorationLocation = 30;
			constexpr uint32_t kStorageClassInput = 1;

			std::unordered_map<uint32_t, uint32_t> locations; // result id -> Location
			std::vector<uint32_t> inputs;                     // result ids of Input variables
			for (size_t word = 5; word < spirv.size();)
			{
				const uint32_t wordCount = spirv[word] >> 16;
				const uint32_t opcode = spirv[word] & 0xFFFFu;
				if (wordCount == 0 || word + wordCount > spirv.size())
					break;

				if (opcode == kOpDecorate && wordCount >= 4 && spirv[word + 2] == kDecorationLocation)
					locations[spirv[word + 1]] = spirv[word + 3];
				else if (opcode == kOpVariable && wordCount >= 4 && spirv[word + 3] == kStorageClassInput)
					inputs.push_back(spirv[word + 2]);

				word += wordCount;
			}

			uint32_t mask = 0;
			for (const uint32_t id : inputs)
			{
				const auto it = locations.find(id);
				if (it != locations.end() && it->second < 32)
					mask |= 1u << it->second;
			}
			return mask;
		}

		[[nodiscard]] std::string ExtractShaderName(const std::filesystem::path& path)
		{
			return path.stem().string();
		}

		[[nodiscard]] VkDevice GetActiveVkDevice()
		{
			if (!RenderContext::IsInitialized())
				return VK_NULL_HANDLE;

			const Ref<Device> device = RenderContext::Get()->GetDevice();
			return device ? device->GetLogicalDevice() : VK_NULL_HANDLE;
		}

		[[nodiscard]] VkShaderModule CreateVkShaderModule(VkDevice device, const std::vector<uint32_t>& spirv)
		{
			if (device == VK_NULL_HANDLE || !IsValidSpirv(spirv))
				return VK_NULL_HANDLE;

			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = spirv.size() * sizeof(uint32_t);
			createInfo.pCode = spirv.data();

			VkShaderModule module = VK_NULL_HANDLE;
			if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS)
				return VK_NULL_HANDLE;

			return module;
		}

		void DestroyVkShaderModule(VkDevice device, VkShaderModule& module) noexcept
		{
			if (device != VK_NULL_HANDLE && module != VK_NULL_HANDLE)
			{
				vkDestroyShaderModule(device, module, nullptr);
				module = VK_NULL_HANDLE;
			}
		}

		[[nodiscard]] std::filesystem::path NormalizeShaderPath(const std::string& filepath)
		{
			std::filesystem::path path(filepath);
			if (path.is_relative())
				path = std::filesystem::path(kShaderDirectoryPath) / path;
			return path.lexically_normal();
		}
	}

	class VulkanShaderStage final : public IShader
	{
	public:
		VulkanShaderStage(ShaderType stage, std::vector<uint32_t> spirv, VkDevice device)
			: m_Desc{stage, kSpirvEntryPoint}
			, m_Bytecode(std::move(spirv))
			, m_Device(device)
		{
			m_Module = CreateVkShaderModule(m_Device, m_Bytecode);
		}

		~VulkanShaderStage() override { DestroyVkShaderModule(m_Device, m_Module); }

		[[nodiscard]] const ShaderDesc& GetDesc() const override { return m_Desc; }

		void GetBytecode(const void** ppBytecode, size_t* pSize) const override
		{
			if (ppBytecode)
				*ppBytecode = m_Bytecode.empty() ? nullptr : m_Bytecode.data();

			if (pSize)
				*pSize = m_Bytecode.size() * sizeof(uint32_t);
		}

		[[nodiscard]] VkShaderModule GetModule() const noexcept { return m_Module; }

	private:
		ShaderDesc m_Desc{};
		std::vector<uint32_t> m_Bytecode;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkShaderModule m_Module = VK_NULL_HANDLE;
	};

	ShaderUniform::ShaderUniform(std::string name, ShaderUniformType type, uint32_t size, uint32_t offset)
		: m_Name(std::move(name))
		, m_Type(type)
		, m_Size(size)
		, m_Offset(offset)
	{
	}

	constexpr std::string_view ShaderUniform::UniformTypeToString(ShaderUniformType type)
	{
		switch (type)
		{
			case ShaderUniformType::Bool:  return "bool";
			case ShaderUniformType::Int:   return "int";
			case ShaderUniformType::UInt:  return "uint";
			case ShaderUniformType::Float: return "float";
			case ShaderUniformType::Vec2:  return "vec2";
			case ShaderUniformType::Vec3:  return "vec3";
			case ShaderUniformType::Vec4:  return "vec4";
			case ShaderUniformType::Mat3:  return "mat3";
			case ShaderUniformType::Mat4:  return "mat4";
			case ShaderUniformType::IVec2: return "ivec2";
			case ShaderUniformType::IVec3: return "ivec3";
			case ShaderUniformType::IVec4: return "ivec4";
			default:                       return "none";
		}
	}

	Shader::~Shader()
	{
		ReleaseShaderModules();
	}

	void Shader::ReleaseShaderModules() noexcept
	{
		m_PipelineShaderStageCreateInfos.clear();
		m_ShaderHandles.clear();
	}

	Ref<Shader> Shader::Create(const std::string& filepath, bool forceCompile, bool disableOptimization)
	{
		const std::filesystem::path sourcePath = NormalizeShaderPath(filepath);

		// Slang source compiles through the Slang front-end (with reflection); anything else is treated as precompiled SPIR-V.
		if (sourcePath.extension() == ".slang")
			return SlangCompiler::Compile(sourcePath.string(), forceCompile, disableOptimization);

		const std::filesystem::path spirvPath = ShaderCache::ResolveSpirvPath(sourcePath.string());

		std::vector<uint32_t> spirv;
		if (!ShaderCache::TryLoadSpirv(spirvPath, spirv))
		{
			SEDX_CORE_ERROR_TAG("Shader", "Failed to load SPIR-V for shader '{}'", sourcePath.string());
			return nullptr;
		}

		auto shader = CreateRef<Shader>();
		shader->m_AssetPath = sourcePath;
		shader->m_Name = ExtractShaderName(sourcePath);
		shader->m_DisableOptimization = disableOptimization;

		std::map<ShaderType, std::vector<uint32_t>> modules;
		modules.emplace(InferShaderTypeFromPath(spirvPath), std::move(spirv));
		shader->LoadAndCreateShaders(modules);
		shader->CreateDescriptors();
		return shader;
	}

	Ref<Shader> Shader::LoadFromShaderPack(const std::string& /*filepath*/, bool /*forceCompile*/, bool /*disableOptimization*/)
	{
		SEDX_CORE_WARN_TAG("Shader", "LoadFromShaderPack(path) is deprecated; load via ShaderLibrary::LoadShaderPack + ShaderPack::LoadShader");
		return nullptr;
	}

	Ref<Shader> Shader::CreateFromString(const std::string& /*source*/)
	{
		SEDX_CORE_WARN_TAG("Shader", "CreateFromString is not supported; compile from a .slang file via SlangCompiler::Compile");
		return nullptr;
	}

	const std::unordered_map<std::string, ShaderBuffer>& Shader::GetShaderBuffers() const
	{
		return m_ShaderBuffers;
	}

	const std::unordered_map<std::string, ShaderResourceDeclaration>& Shader::GetResources() const
	{
		return m_Resources;
	}

	size_t Shader::GetHash() const
	{
		size_t hash = static_cast<size_t>(Hash::GenerateFNV1A(std::string_view{m_Name}));
		for (const auto& [stage, bytecode] : m_ShaderData)
		{
			hash = Hash::HashCombine(hash, static_cast<size_t>(stage));
			hash = Hash::HashCombine(hash, HashSpirv(bytecode));
		}
		return hash;
	}

	void Shader::AddShaderReloadedCallback(const ShaderReloadedCallback& callback)
	{
		if (callback)
			m_ReloadCallbacks.push_back(callback);
	}

	bool Shader::AreSourcesNewerThanBuild() const
	{
		std::error_code ec;
		for (const std::filesystem::path& dependency : m_SourceDependencies)
		{
			const auto writeTime = std::filesystem::last_write_time(dependency, ec);
			if (ec || writeTime > m_BuildTime)
				return true;
		}
		return false;
	}

	void Shader::AdoptCompiledState(Shader& other)
	{
		ReleaseShaderModules();
		m_ShaderHandles = std::move(other.m_ShaderHandles);
		m_ShaderData = std::move(other.m_ShaderData);
		m_PipelineShaderStageCreateInfos = std::move(other.m_PipelineShaderStageCreateInfos);
		m_TypeCounts = std::move(other.m_TypeCounts);
		m_ShaderBuffers = std::move(other.m_ShaderBuffers);
		m_Resources = std::move(other.m_Resources);
		m_SourceDependencies = std::move(other.m_SourceDependencies);
		m_BuildTime = other.m_BuildTime;
		m_VertexType = other.m_VertexType;
		m_VertexInputLocations = other.m_VertexInputLocations;
		m_PushConstantSize = other.m_PushConstantSize;
		m_HasBindingConflicts = other.m_HasBindingConflicts;
	}

	bool Shader::TryReloadFromDisk()
	{
		// Slang-built shaders: recompile when the entry module or any imported module changed.
		if (m_CompileOptions)
		{
			if (!AreSourcesNewerThanBuild())
				return false;

			ShaderCompileOptions options = *m_CompileOptions;
			options.forceCompile = true;

			const Ref<Shader> rebuilt = SlangCompiler::Compile(options);
			if (!rebuilt || !rebuilt->IsCompiled())
			{
				// Keep the last good program; retry once the source changes again.
				m_BuildTime = std::filesystem::file_time_type::clock::now();
				return false;
			}

			AdoptCompiledState(*rebuilt);
			NotifyReloaded();
			return true;
		}

		if (m_AssetPath.empty())
			return false;

		const std::filesystem::path spirvPath = ShaderCache::ResolveSpirvPath(m_AssetPath.string());
		if (!std::filesystem::exists(spirvPath))
			return false;

		std::vector<uint32_t> spirv;
		if (!ShaderCache::TryLoadSpirv(spirvPath, spirv))
			return false;

		const ShaderType stage = InferShaderTypeFromPath(spirvPath);
		const auto existing = m_ShaderData.find(stage);
		if (existing != m_ShaderData.end() && existing->second == spirv)
			return false;

		std::map<ShaderType, std::vector<uint32_t>> modules;
		if (m_ShaderData.size() > 1)
		{
			modules = m_ShaderData;
			modules[stage] = spirv;
		}
		else
		{
			modules.emplace(stage, std::move(spirv));
		}

		LoadAndCreateShaders(modules);
		CreateDescriptors();
		NotifyReloaded();
		return true;
	}

	void Shader::NotifyReloaded()
	{
		for (const ShaderReloadedCallback& callback : m_ReloadCallbacks)
		{
			if (callback)
				callback();
		}
	}

	ShaderHandle Shader::GetHandle() const
	{
		if (m_ShaderHandles.empty())
			return {};

		return m_ShaderHandles.begin()->second;
	}

	ShaderHandle Shader::GetHandle(ShaderType type) const
	{
		const auto it = m_ShaderHandles.find(type);
		SEDX_CORE_VERIFY(it != m_ShaderHandles.end());
		return it != m_ShaderHandles.end() ? it->second : ShaderHandle{};
	}

	const std::map<ShaderType, ShaderHandle>& Shader::GetHandles() const
	{
		return m_ShaderHandles;
	}

	const char* Shader::GetShaderDirectoryPath()
	{
		return kShaderDirectoryPath;
	}

	bool Shader::IsCompiled() const
	{
		return !m_PipelineShaderStageCreateInfos.empty() && !m_HasBindingConflicts;
	}

	bool Shader::HasStage(ShaderType type) const
	{
		const VkShaderStageFlagBits vkStage = ShaderTypeToVkStage(type);
		return std::ranges::any_of(m_PipelineShaderStageCreateInfos, [vkStage](const VkPipelineShaderStageCreateInfo& info)
		{
			return info.stage == vkStage && info.module != VK_NULL_HANDLE;
		});
	}

	bool Shader::HasStage(StageType stage) const
	{
		const ShaderType type = ToShaderType(stage);
		return type != ShaderType::None && HasStage(type);
	}

	VkPipelineShaderStageCreateInfo Shader::GetStageCreateInfo(ShaderType type) const
	{
		const VkShaderStageFlagBits vkStage = ShaderTypeToVkStage(type);
		for (const VkPipelineShaderStageCreateInfo& info : m_PipelineShaderStageCreateInfos)
		{
			if (info.stage == vkStage)
				return info;
		}

		VkPipelineShaderStageCreateInfo missing{};
		missing.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		missing.stage = vkStage;
		return missing;
	}

	VkPipelineShaderStageCreateInfo Shader::GetStageCreateInfo(StageType stage) const
	{
		return GetStageCreateInfo(ToShaderType(stage));
	}

	std::vector<Descriptor> Shader::GetDescriptors(ShaderType stageFilter) const
	{
		const uint32_t filterMask = stageFilter == ShaderType::None ? 0u : static_cast<uint32_t>(ShaderTypeToVkStageFlags(stageFilter));

		std::vector<Descriptor> result;
		result.reserve(m_Resources.size());

		for (const ShaderResourceDeclaration& resource : m_Resources | std::views::values)
		{
			// Set 0 is the per-draw dynamic set; space1+ are bindless sets owned by BindlessManager.
			// Types without a VkDescriptorType mapping (e.g. separate samplers) cannot go into the dynamic set layout.
			if (resource.GetSet() != 0 || ToVkDescriptorType(resource.GetType()) == VK_DESCRIPTOR_TYPE_MAX_ENUM)
				continue;

			if (filterMask != 0 && (resource.GetStageFlags() & filterMask) == 0)
				continue;

			DescriptorSpec spec{};
			spec.name        = resource.GetName();
			spec.type        = resource.GetType();
			spec.layout      = Layout::ImageLayout::MaxEnum;
			spec.slot        = resource.GetRegister();
			spec.stage       = filterMask != 0 ? filterMask : resource.GetStageFlags();
			spec.structSize  = 0;
			spec.asArray     = resource.GetCount() > 1;
			spec.arrayLength = resource.GetCount();
			result.emplace_back(spec);
		}

		std::ranges::sort(result, [](const Descriptor& lhs, const Descriptor& rhs)
		{
			return lhs.GetSlot() < rhs.GetSlot();
		});

		return result;
	}

	std::vector<Descriptor> Shader::GetDescriptors(StageType stage) const
	{
		const ShaderType type = ToShaderType(stage);
		if (type == ShaderType::None)
			return {};

		return GetDescriptors(type);
	}

	void Shader::SetName(std::string name)
	{
		m_Name = std::move(name);
	}

	void Shader::TryReadReflectionData(StreamReader* reader)
	{
		if (!reader)
			return;

		reader->ReadMap(m_ShaderBuffers);
		reader->ReadMap(m_Resources);
		ValidateBindings();
	}

	void Shader::SerializeReflectionData(StreamWriter* writer)
	{
		if (!writer)
			return;

		writer->WriteMap(m_ShaderBuffers);
		writer->WriteMap(m_Resources);
	}

	void Shader::ValidateBindings()
	{
		m_HasBindingConflicts = false;

		std::unordered_map<uint64_t, const ShaderResourceDeclaration*> bySetBinding;
		bySetBinding.reserve(m_Resources.size());

		for (const ShaderResourceDeclaration& resource : m_Resources | std::views::values)
		{
			const uint64_t key = (static_cast<uint64_t>(resource.GetSet()) << 32ull) | resource.GetRegister();
			const auto [it, inserted] = bySetBinding.emplace(key, &resource);
			if (inserted || it->second->GetType() == resource.GetType())
				continue;

			// Two different descriptor types on one (set, binding) can never form a valid VkDescriptorSetLayout.
			SEDX_CORE_ERROR_TAG("Shader", "Shader '{}': binding conflict at set={} binding={} between '{}' and '{}' (descriptor types {} vs {})",
				m_Name, resource.GetSet(), resource.GetRegister(), it->second->GetName(), resource.GetName(),
				static_cast<uint32_t>(it->second->GetType()), static_cast<uint32_t>(resource.GetType()));
			m_HasBindingConflicts = true;
		}
	}

	void Shader::LoadAndCreateShaders(const std::map<ShaderType, std::vector<uint32_t>>& shaderModules)
	{
		ReleaseShaderModules();

		const VkDevice device = GetActiveVkDevice();
		m_ShaderData.clear();
		m_PipelineShaderStageCreateInfos.clear();
		m_VertexInputLocations = ~0u;

		for (const auto& [stage, spirv] : shaderModules)
		{
			if (!IsValidSpirv(spirv))
			{
				SEDX_CORE_WARN_TAG("Shader", "Shader '{}': skipping invalid SPIR-V module for stage {}", m_Name, static_cast<unsigned>(stage));
				continue;
			}

			m_ShaderData.emplace(stage, spirv);
			if (stage == ShaderType::Vertex)
				m_VertexInputLocations = ReflectInputLocations(spirv);

			auto module = CreateRef<VulkanShaderStage>(stage, spirv, device);
			m_ShaderHandles.emplace(stage, module);

			if (module->GetModule() == VK_NULL_HANDLE)
			{
				SEDX_CORE_ERROR_TAG("Shader", "Shader '{}': vkCreateShaderModule failed for stage {}", m_Name, static_cast<unsigned>(stage));
				continue;
			}

			VkPipelineShaderStageCreateInfo stageInfo{};
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.stage = ShaderTypeToVkStage(stage);
			stageInfo.module = module->GetModule();
			stageInfo.pName = module->GetDesc().entryPoint;
			m_PipelineShaderStageCreateInfos.push_back(stageInfo);
		}
	}

	void Shader::CreateDescriptors()
	{
		m_TypeCounts.clear();

		for (const ShaderResourceDeclaration& resource : m_Resources | std::views::values)
		{
			const VkDescriptorType vkType = ToVkDescriptorType(resource.GetType());
			if (vkType == VK_DESCRIPTOR_TYPE_MAX_ENUM)
				continue;

			VkDescriptorPoolSize poolSize{};
			poolSize.type = vkType;
			poolSize.descriptorCount = std::max(1u, resource.GetCount());
			m_TypeCounts[resource.GetSet()].push_back(poolSize);
		}
	}

}  // namespace SceneryEditorX

// -------------------------------------------------------

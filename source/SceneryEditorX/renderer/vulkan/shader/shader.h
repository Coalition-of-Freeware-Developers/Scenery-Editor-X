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
 * shader.h
 * -------------------------------------------------------
 * Created: 22/06/2026
 * -------------------------------------------------------
 */
#pragma once
#include "ishader.h"
#include "shader_uniform.h"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <ranges>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/renderer/vulkan/descriptor.h>
#include <SceneryEditorX/renderer/vulkan/enums.h>
#include <SceneryEditorX/utils/filestreaming/filestream_reader.h>
#include <SceneryEditorX/utils/filestreaming/filestream_writer.h>
#include <SceneryEditorX/utils/inheritance.h>
#include <SceneryEditorX/utils/pointers.h>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class ShaderPack;
	class SlangCompiler;

	/**
	 * @enum ShaderStage
	 * @brief Represents the various stages of the graphics pipeline where shaders can be executed.
	 * This enumeration is used to identify and manage shader stages such as vertex, tessellation control, geometry,
	 * fragment, compute, task, and mesh shaders. It is essential for shader compilation, resource binding, and pipeline
	 * configuration in graphics applications.
	 */
	enum ShaderStage : uint8_t
	{
		SHT_VERTEX                  = 0,
		SHT_TESSELLATION_CONTROL    = 1,  // Vulkan: Tessellation control	/ DirectX: Hull
		SHT_TESSELLATION_EVALUATION = 2,  // Vulkan: Tessellation evaluation / DirectX: Domain
		SHT_GEOMETRY                = 3,
		SHT_FRAGMENT                = 4,  // Vulkan: Fragment / DirectX: Pixel
		SHT_COMPUTE                 = 5,
		SHT_TASK                    = 6,  // Vulkan: Task / DirectX: Amplification
		SHT_MESH                    = 7,
		SHT_LAST,
	};

	/**
	 * @enum ShaderType
	 * @brief Represents the type of shader being used in the graphics pipeline.
	 * This enumeration is used to identify and manage different shader stages, such as vertex, fragment, compute, etc.,
	 * and is essential for shader compilation, resource binding, and pipeline configuration.
	 */
	enum class ShaderType : uint16_t
	{
		None					= 0x0000,
		Vertex					= 0x0001,
		TessellationControl		= 0x0002,  // Vulkan: Tessellation control	/ DirectX: Hull
		TessellationEvaluation	= 0x0004,  // Vulkan: Tessellation evaluation / DirectX: Domain
		Geometry				= 0x0008,
		Fragment				= 0x0010,  // Vulkan: Fragment / DirectX: Pixel
		Compute					= 0x0020,
		Task					= 0x0040,  // Vulkan: Task / DirectX: Amplification
		Mesh					= 0x0080,
		AllGraphics				= 0x00DF,  // All graphics shader stages (vertex, tessellation control, tessellation evaluation, geometry, fragment, task, mesh)

		RayGeneration			= 0x0100,
		AnyHit					= 0x0200,
		ClosestHit				= 0x0400,
		Miss					= 0x0800,
		Intersection			= 0x1000,
		Callable				= 0x2000,
		AllRayTracing			= 0x3F00,

		All						= 0x3FFF,  // All shader stages (graphics and ray tracing)
		Library					= 0x5000,  // Shader library (not a shader stage, used for shader module grouping)

	};

	/**
	 * @struct ShaderMacro
	 * @brief A preprocessor define passed to the shader compiler (e.g. LUMINANCE=1 to select a bloom variant).
	 */
	struct ShaderMacro
	{
		std::string name;
		std::string value = "1";
	};

	/**
	 * @struct ShaderCompileOptions
	 * @brief Describes one shader program variant to compile from Slang source.
	 */
	struct ShaderCompileOptions
	{
		std::string name;						// Library key and debug name (e.g. "bloom_luminance")
		std::filesystem::path sourcePath;		// .slang source, absolute or relative to the shader directory
		std::vector<ShaderType> stages;			// Stages to extract; empty = every entry point defined in the module
		std::vector<ShaderMacro> macros;		// Preprocessor defines selecting the variant
		VertexType vertexType = VertexType::MaxEnum; // Vertex input layout; MaxEnum = infer from vertex-stage semantics
		bool forceCompile = false;				// Bypass the on-disk program cache
		bool disableOptimization = false;
	};

	/**
	 * @enum ShaderUniformType
	 * @brief Represents the type of a shader uniform.
	 */
	enum class ShaderUniformType
	{
		None = 0,
		Bool,
		Int,
		UInt,
		Float,
		Vec2,
		Vec3,
		Vec4,
		Mat3,
		Mat4,
		IVec2,
		IVec3,
		IVec4
	};

	/**
	 * @class ShaderUniform
	 * @brief Represents a shader uniform, which is a variable that can be accessed by shaders.
	 */
	class ShaderUniform
	{
	public:
		ShaderUniform() = default;

		/**
		 * @brief Constructs a ShaderUniform instance with the specified parameters.
		 * @param name The name of the shader uniform.
		 * @param type The type of the shader uniform.
		 * @param size The size of the shader uniform in bytes.
		 * @param offset The offset of the shader uniform within its buffer.
		 */
		ShaderUniform(std::string name, ShaderUniformType type, uint32_t size, uint32_t offset);

		const std::string& GetName() const { return m_Name; }
		ShaderUniformType GetType() const { return m_Type; }
		uint32_t GetSize() const { return m_Size; }
		uint32_t GetOffset() const { return m_Offset; }

		/**
		 * @brief Converts a ShaderUniformType to its corresponding string representation.
		 * @param type The ShaderUniformType to convert.
		 * @return A string view representing the ShaderUniformType.
		 */
		static constexpr std::string_view UniformTypeToString(ShaderUniformType type);

		/**
		 * @brief Serializes a ShaderUniform instance to a stream.
		 * @param serializer The stream writer used for serialization.
		 * @param instance The ShaderUniform instance to serialize.
		 */
		static void Serialize(StreamWriter* serializer, const ShaderUniform& instance)
		{
			serializer->WriteString(instance.m_Name);
			serializer->WriteRaw(instance.m_Type);
			serializer->WriteRaw(instance.m_Size);
			serializer->WriteRaw(instance.m_Offset);
		}

		/**
		 * @brief Deserializes a ShaderUniform instance from a stream.
		 * @param deserializer The stream reader used for deserialization.
		 * @param instance The ShaderUniform instance to deserialize.
		 */
		static void Deserialize(StreamReader* deserializer, ShaderUniform& instance)
		{
			deserializer->ReadString(instance.m_Name);
			deserializer->ReadRaw(instance.m_Type);
			deserializer->ReadRaw(instance.m_Size);
			deserializer->ReadRaw(instance.m_Offset);
		}
	private:
		std::string m_Name;
		ShaderUniformType m_Type = ShaderUniformType::None;
		uint32_t m_Size = 0;
		uint32_t m_Offset = 0;
	};

	/**
	 * @struct ShaderUniformBuffer
	 * @brief Represents a shader uniform buffer, which is used for storing uniform data that can be accessed by shaders.
	 */
	struct ShaderUniformBuffer
	{
		std::string Name;
		uint32_t Index;
		uint32_t BindingPoint;
		uint32_t Size;
		uint32_t RendererID;
		std::vector<ShaderUniform> Uniforms;
	};

	/**
	 * @struct ShaderStorageBuffer
	 * @brief Represents a shader storage buffer, which is used for storing large amounts of data that can be read and written by shaders.
	 */
	struct ShaderStorageBuffer
	{
		std::string Name;
		uint32_t Index;
		uint32_t BindingPoint;
		uint32_t Size;
		uint32_t RendererID;
		//std::vector<ShaderUniform> Uniforms;
	};

	/**
	 * @struct ShaderBuffer
	 * @brief Represents a shader buffer, which is used for storing uniform data that can be accessed by shaders.
	 */
	struct ShaderBuffer
	{
		std::string Name;
		uint32_t Size = 0;
		std::unordered_map<std::string, ShaderUniform> Uniforms;

		/**
		 * @brief Serializes a ShaderBuffer instance to a stream.
		 * @param serializer The stream writer used for serialization.
		 * @param instance The ShaderBuffer instance to serialize.
		 */
		static void Serialize(StreamWriter* serializer, const ShaderBuffer& instance)
		{
			serializer->WriteString(instance.Name);
			serializer->WriteRaw(instance.Size);
			serializer->WriteMap(instance.Uniforms);
		}

		/**
		 * @brief Deserializes a ShaderBuffer instance from a stream.
		 * @param deserializer The stream reader used for deserialization.
		 * @param instance The ShaderBuffer instance to deserialize.
		 */
		static void Deserialize(StreamReader* deserializer, ShaderBuffer& instance)
		{
			deserializer->ReadString(instance.Name);
			deserializer->ReadRaw(instance.Size);
			deserializer->ReadMap(instance.Uniforms);
		}
	};

	/**
	 * @class Shader
	 * @brief A shader program: one SPIR-V module per stage plus the reflected resource bindings shared by those stages.
	 */
	class Shader : public SharedObject
	{
	public:
		/* @brief Type definition for a callback function that is called when a shader is reloaded. */
		typedef std::function<void()> ShaderReloadedCallback;

		/**
		 * @brief Creates a shader from a file.
		 * @param filepath The path to the shader file.
		 * @param forceCompile Whether to force compilation of the shader.
		 * @param disableOptimization Whether to disable optimization for the shader.
		 * @return A reference to the created shader.
		 */
		static Ref<Shader> Create(const std::string& filepath, bool forceCompile = false, bool disableOptimization = false);

		/**
		 * @brief Loads a shader from a shader pack.
		 * @param filepath The path to the shader pack file.
		 * @param forceCompile Whether to force compilation of the shader.
		 * @param disableOptimization Whether to disable optimization for the shader.
		 * @return A reference to the loaded shader.
		 */
		static Ref<Shader> LoadFromShaderPack(const std::string& filepath, bool forceCompile = false, bool disableOptimization = false);

		/**
		 * @brief Creates a shader from a string source.
		 * @param source The shader source code as a string.
		 * @return A reference to the created shader.
		 */
		static Ref<Shader> CreateFromString(const std::string& source);

		~Shader() override;

		[[nodiscard]] const std::unordered_map<std::string, ShaderBuffer>& GetShaderBuffers() const;
		[[nodiscard]] const std::unordered_map<std::string, ShaderResourceDeclaration>& GetResources() const;

		[[nodiscard]] const std::string& GetName() const { return m_Name; }
		[[nodiscard]] const std::filesystem::path& GetAssetPath() const { return m_AssetPath; }

		[[nodiscard]] size_t GetHash() const;

		void AddShaderReloadedCallback(const ShaderReloadedCallback& callback);

		/** @brief Rebuild the shader when its source (Slang) or bytecode (SPIR-V) changed on disk. Returns true after a successful reload. */
		[[nodiscard]] bool TryReloadFromDisk();

		[[nodiscard]] ShaderHandle GetHandle() const;
		[[nodiscard]] ShaderHandle GetHandle(ShaderType type) const;

		[[nodiscard]] const std::map<ShaderType, ShaderHandle>& GetHandles() const;
		[[nodiscard]] static const char* GetShaderDirectoryPath();

		[[nodiscard]] const std::map<ShaderType, std::vector<uint32_t>>& GetShaderBytecode() const { return m_ShaderData; }

		/** @brief True when at least one stage has a valid VkShaderModule and reflection found no binding conflicts. */
		[[nodiscard]] bool IsCompiled() const;

		/** @brief True when two reflected resources claim the same (set, binding) with different descriptor types. */
		[[nodiscard]] bool HasBindingConflicts() const { return m_HasBindingConflicts; }

		[[nodiscard]] bool HasStage(ShaderType type) const;
		[[nodiscard]] bool HasStage(StageType stage) const;

		/**
		 * @brief Returns the pipeline stage description (module + entry point) for one stage.
		 * @return A zeroed struct with module == VK_NULL_HANDLE when the stage is missing.
		 */
		[[nodiscard]] VkPipelineShaderStageCreateInfo GetStageCreateInfo(ShaderType type) const;
		[[nodiscard]] VkPipelineShaderStageCreateInfo GetStageCreateInfo(StageType stage) const;

		/**
		 * @brief Returns the set-0 (dynamic) descriptor bindings, sorted by binding slot.
		 * Bindless sets (space1+) are owned by BindlessManager and are not included.
		 * @param stageFilter When not None, only bindings referenced by that stage are returned.
		 */
		[[nodiscard]] std::vector<Descriptor> GetDescriptors(ShaderType stageFilter = ShaderType::None) const;
		[[nodiscard]] std::vector<Descriptor> GetDescriptors(StageType stage) const;

		/** @brief Vertex input layout expected by the vertex stage (MaxEnum when the shader pulls vertices itself). */
		[[nodiscard]] VertexType GetVertexType() const { return m_VertexType; }

		/**
		 * @brief Vertex attribute locations the vertex stage actually reads (bit n = location n), from its SPIR-V.
		 * A shader may read fewer attributes than its VertexType describes (the grid only reads POSITION); pipelines
		 * leave the others out of their vertex input state. All bits are set when there is no vertex stage to inspect.
		 */
		[[nodiscard]] uint32_t GetVertexInputLocations() const { return m_VertexInputLocations; }

		/** @brief Size in bytes of the push-constant block declared by the program (0 if none). */
		[[nodiscard]] uint32_t GetPushConstantSize() const { return m_PushConstantSize; }

		/** @brief Source files (entry module + imports) this shader was compiled from. Empty for precompiled SPIR-V. */
		[[nodiscard]] const std::vector<std::filesystem::path>& GetSourceDependencies() const { return m_SourceDependencies; }

	private:
		friend class ShaderPack;
		friend class SlangCompiler;

		void SetName(std::string name);
		void TryReadReflectionData(StreamReader* reader);
		void SerializeReflectionData(StreamWriter* writer);
		void LoadAndCreateShaders(const std::map<ShaderType, std::vector<uint32_t>>& shaderModules);
		void CreateDescriptors();
		void ReleaseShaderModules() noexcept;
		void NotifyReloaded();
		void ValidateBindings();
		void AdoptCompiledState(Shader& other);
		[[nodiscard]] bool AreSourcesNewerThanBuild() const;

		std::string m_Name;
		std::filesystem::path m_AssetPath;
		std::map<ShaderType, ShaderHandle> m_ShaderHandles;
		std::map<ShaderType, std::vector<uint32_t>> m_ShaderData;
		std::vector<VkPipelineShaderStageCreateInfo> m_PipelineShaderStageCreateInfos;
		std::unordered_map<uint32_t, std::vector<VkDescriptorPoolSize>> m_TypeCounts;
		std::unordered_map<std::string, ShaderBuffer> m_ShaderBuffers;
		std::unordered_map<std::string, ShaderResourceDeclaration> m_Resources;
		std::vector<ShaderReloadedCallback> m_ReloadCallbacks;
		std::vector<std::filesystem::path> m_SourceDependencies;
		std::filesystem::file_time_type m_BuildTime{};
		std::unique_ptr<ShaderCompileOptions> m_CompileOptions;	// Set when built from Slang source (enables recompilation)
		VertexType m_VertexType = VertexType::MaxEnum;
		uint32_t m_VertexInputLocations = ~0u;
		uint32_t m_PushConstantSize = 0;
		bool m_HasBindingConflicts = false;
		bool m_DisableOptimization = false;
	};

	// @brief A compile-time range of all valid ShaderType enumeration values, excluding the 'None' value.
	inline constexpr auto shaderTypesRange = std::views::iota(uint8_t(0), uint8_t(9)) | std::views::transform([](auto v) { return static_cast<ShaderType>(v); });

	// @brief The number of valid ShaderType enumeration values, excluding the 'None' value.
	inline constexpr auto shaderTypesN = std::ranges::size(shaderTypesRange);

	/**
	 * @brief Converts a single-stage ShaderType to its corresponding ShaderStage index.
	 */
	[[nodiscard]] inline ShaderStage ToShaderStage(ShaderType type)
	{
		switch (type)
		{
			case ShaderType::Vertex:                 return SHT_VERTEX;
			case ShaderType::TessellationControl:    return SHT_TESSELLATION_CONTROL;
			case ShaderType::TessellationEvaluation: return SHT_TESSELLATION_EVALUATION;
			case ShaderType::Geometry:               return SHT_GEOMETRY;
			case ShaderType::Fragment:               return SHT_FRAGMENT;
			case ShaderType::Compute:                return SHT_COMPUTE;
			case ShaderType::Task:                   return SHT_TASK;
			case ShaderType::Mesh:                   return SHT_MESH;
			default:
				SEDX_CORE_ASSERT(false, "ShaderType is not a single graphics stage: %u", static_cast<unsigned>(type));
				return SHT_VERTEX;
		}
	}

	/**
	 * @brief Converts a shader stage enumeration value to its corresponding shader type flag.
	 */
	[[nodiscard]] inline ShaderType ToShaderType(ShaderStage stage)
	{
		SEDX_CORE_ASSERT(stage < SHT_LAST, "stage: %zu", static_cast<size_t>(stage));
		switch (stage)
		{
			case SHT_VERTEX:                  return ShaderType::Vertex;
			case SHT_TESSELLATION_CONTROL:    return ShaderType::TessellationControl;
			case SHT_TESSELLATION_EVALUATION: return ShaderType::TessellationEvaluation;
			case SHT_GEOMETRY:                return ShaderType::Geometry;
			case SHT_FRAGMENT:                return ShaderType::Fragment;
			case SHT_COMPUTE:                 return ShaderType::Compute;
			case SHT_TASK:                    return ShaderType::Task;
			case SHT_MESH:                    return ShaderType::Mesh;
			default:                          return ShaderType::None;
		}
	}

	/**
	 * @brief Converts a pipeline-state stage slot (StageType) to the matching ShaderType flag.
	 * @return ShaderType::None for aggregate or invalid slots (Graphics, All, None, MaxEnum).
	 */
	[[nodiscard]] inline ShaderType ToShaderType(StageType stage)
	{
		switch (stage)
		{
			case StageType::Vertex:                 return ShaderType::Vertex;
			case StageType::TessellationControl:    return ShaderType::TessellationControl;
			case StageType::TessellationEvaluation: return ShaderType::TessellationEvaluation;
			case StageType::Geometry:               return ShaderType::Geometry;
			case StageType::Fragment:               return ShaderType::Fragment;
			case StageType::Compute:                return ShaderType::Compute;
			default:                                return ShaderType::None;
		}
	}

}  // namespace SceneryEditorX

// -------------------------------------------------------

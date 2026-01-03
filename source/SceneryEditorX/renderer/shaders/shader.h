/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* shader.h
* -------------------------------------------------------
* Created: 8/4/2025
* -------------------------------------------------------
*/
#pragma once
#include "shader_resource.h"
#include "shader_uniforms.h"
#include "SceneryEditorX/renderer/render_context.h"
#include "SceneryEditorX/renderer/uniform_buffer.h"
#include "SceneryEditorX/utils/filestreaming/filestream_reader.h"
#include "SceneryEditorX/utils/filestreaming/filestream_writer.h"
#include <spirv_cross/spirv_hlsl.hpp>

// -------------------------------------------------------

using namespace SPIRV_CROSS_NAMESPACE;

// -------------------------------------------------------

namespace SceneryEditorX
{

	class ShaderUniform
	{
	public:
		ShaderUniform() = default;
        ShaderUniform(std::string name, ShaderDataType type, uint32_t size, uint32_t offset);

        [[nodiscard]] const std::string& GetName() const { return m_Name; }
        [[nodiscard]] ShaderDataType GetType() const { return m_Type; }
		[[nodiscard]] uint32_t GetSize() const { return m_Size; }
		[[nodiscard]] uint32_t GetOffset() const { return m_Offset; }

		static constexpr std::string_view UniformTypeToString(ShaderDataType type);

		/*
		static void Serialize(SerializeWriter* serializer, const ShaderUniform& instance)
		{
			serializer->WriteString(instance.m_Name);
			serializer->WriteRaw(instance.m_Type);
			serializer->WriteRaw(instance.m_Size);
			serializer->WriteRaw(instance.m_Offset);
		}

		static void Deserialize(SerializeReader *deserializer, ShaderUniform &instance)
		{
			deserializer->ReadString(instance.m_Name);
			deserializer->ReadRaw(instance.m_Type);
			deserializer->ReadRaw(instance.m_Size);
			deserializer->ReadRaw(instance.m_Offset);
		}
		*/

	private:
		std::string m_Name;
        ShaderDataType m_Type = ShaderDataType::None;
		uint32_t m_Size = 0;
		uint32_t m_Offset = 0;
	};

    // -------------------------------------------------------

	struct ShaderUniformBuffer
	{
		std::string Name;
		uint32_t Index;
		uint32_t BindingPoint;
		uint32_t Size;
		uint32_t RendererID;
		std::vector<ShaderUniform> Uniforms;
	};

    // -------------------------------------------------------

	struct ShaderStorageBuffer
	{
		std::string Name;
		uint32_t Index;
		uint32_t BindingPoint;
		uint32_t Size;
		uint32_t RendererID;
		std::vector<ShaderUniform> Uniforms;
	};

    // -------------------------------------------------------

	struct ShaderBuffer
	{
		std::string Name;
		uint32_t Size = 0;
		std::unordered_map<std::string, ShaderUniform> Uniforms;

		/*
		static void Serialize(SerializeWriter *serializer, const ShaderBuffer &instance)
		{
			serializer->WriteString(instance.Name);
			serializer->WriteRaw(instance.Size);
			serializer->WriteMap(instance.Uniforms);
		}

		static void Deserialize(SerializeReader *deserializer, ShaderBuffer &instance)
		{
			deserializer->ReadString(instance.Name);
			deserializer->ReadRaw(instance.Size);
			deserializer->ReadMap(instance.Uniforms);
		}
		*/

	};

    // -------------------------------------------------------

	/**
	 * @class Shader
	 * @brief Represents a Vulkan shader program.
	 *
	 * This class manages shader loading, compilation, and lifecycle in the Scenery Editor X renderer.
	 * It handles shader modules that can be loaded from files or created from string sources.
	 * The class supports hot-reloading through a callback system to notify dependents of changes.
	 */
	class Shader : public RefCounted
	{
	public:

        struct ReflectionData
        {
            std::unordered_map<std::string, ShaderBuffer> ConstantBuffers;
            std::unordered_map<std::string, ShaderResourceDeclaration> Resources;
            std::vector<ShaderResource::PushConstantRange> PushConstantRanges;
            std::vector<ShaderResource::ShaderDescriptorSet> ShaderDescriptorSets;
        };

		/**
		 * @typedef ShaderReloadedCallback.
		 * @brief Function type for shader reload notifications
		 *
		 * Called when a shader is reloaded to allow dependents to update resources.
		 */
		//using ShaderReloadedCallback = std::function<void()>;
		//using ShaderModuleErrorCallback = void (*)(RenderContext*, Ref<Shader>, int line, int col, const char* debugName);

        Shader() = default;
        Shader(const std::string &path, bool forceCompile, bool disableOptimization);
        virtual ~Shader();

        void Release();

	    /**
         * @brief Reload the shader from its source file.
         * @param forceCompile Whether to force recompilation even if a cached version exists
         */
        void Reload(bool forceCompile = false);

        void Reload_RenderThread(bool forceCompile = false);

        virtual size_t GetHash() const;

        /**
	     * @brief Get the base directory path for shader assets.
	     * @return const char* Path to the shader directory
	     */
        static constexpr const char *GetShaderDirectoryPath() { return "assets/shaders/"; }
        const std::vector<VkPipelineShaderStageCreateInfo> &GetPipelineShaderStageCreateInfos() const { return m_PipelineShaderStageCreateInfos; }


        /**
		 * @brief Declares the main entry point for the shader
		 */
	    const char* GetEntryPoint() const;

		ShaderType GetShaderStage() const { return m_ShaderType; }

		/**
		 * @brief Load shader from a shader pack file.
		 *
		 * @param filepath Path to the shader pack file
		 * @param forceCompile Whether to force recompilation even if cached version exists
		 * @param disableOptimization Whether to disable shader optimization during compilation
		 */
		void LoadFromShaderPack(const std::string& filepath, bool forceCompile = false, bool disableOptimization = false);

        /**
         * @brief Create a shader from source code string.
         *
         * @param source The shader source code as a string
         * @return Ref<Shader> Reference to the newly created shader
         */
        static Ref<Shader> CreateFromString(const std::string &source);

		const std::unordered_map<std::string, ShaderBuffer>& GetShaderBuffers() const;
		const std::unordered_map<std::string, ShaderResourceDeclaration>& GetResources() const;

        void* GetRendererResource() const;


        VkPipelineShaderStageCreateInfo CreateShaderStage(const Shader *shader);

		/**
		 * @brief Register a callback to be invoked when shader is reloaded.
		 * @param callback Function to call when shader is recompiled or reloaded
		 */
	    void AddShaderReloadedCallback(const ShaderReloadedCallback& callback);

        /**
         * @brief Get the name of the shader.
         * @return const std::string& The shader name, typically derived from the filename
         */
        [[nodiscard]] const std::string &GetName() const;


	    /**
	     * @brief Create a Vulkan shader module from compiled bytecode.
	     *
	     * @param code Vector containing the compiled shader bytecode
	     * @return VkShaderModule The created Vulkan shader module
	     */
	    [[nodiscard]] VkShaderModule CreateShaderModule(const std::vector<char> &code) const;

	    VkDescriptorSet GetDescriptorSet() { return m_DescriptorSet; }
		VkDescriptorSetLayout GetDescriptorSetLayout(uint32_t set) { return m_DescriptorSetLayouts.at(set); }
		std::vector<VkDescriptorSetLayout> GetAllDescriptorSetLayouts();

        ShaderResource::UniformBuffer GetUniformBuffer(const uint32_t binding = 0, const uint32_t set = 0);

        uint32_t GetUniformBufferCount(const uint32_t set = 0) const;

        const std::vector<ShaderResource::ShaderDescriptorSet>& GetShaderDescriptorSets() const { return m_ReflectionData.ShaderDescriptorSets; }
		const std::vector<ShaderResource::PushConstantRange> &GetPushConstantRanges() const { return m_ReflectionData.PushConstantRanges; }
        bool HasDescriptorSet(uint32_t set) const { return m_TypeCounts.contains(set); }

		struct ShaderMaterialDescriptorSet
		{
			VkDescriptorPool Pool = nullptr;
			std::vector<VkDescriptorSet> DescriptorSets;
		};

        bool TryReadReflectionData(StreamReader *serializer);
        void SerializeReflectionData(StreamWriter *serializer);
        void SetReflectionData(const ReflectionData &reflectionData);

		ShaderMaterialDescriptorSet AllocateDescriptorSet(uint32_t set = 0);
		ShaderMaterialDescriptorSet CreateDescriptorSets(uint32_t set = 0);
		ShaderMaterialDescriptorSet CreateDescriptorSets(uint32_t set, uint32_t numberOfSets);
		const VkWriteDescriptorSet* GetDescriptorSet(const std::string& name, uint32_t set = 0) const;

	private:
        /** @brief Vulkan shader module handle */
        VkShaderModule m_ShaderModule = VK_NULL_HANDLE;

        void LoadAndCreateShaders(const std::map<VkShaderStageFlagBits, std::vector<uint32_t>> &shaderData);
        void CreateDescriptors();

        /** @brief List of callbacks to invoke when shader is reloaded */
        std::vector<ShaderReloadedCallback> m_ReloadCallbacks;
        ShaderModuleErrorCallback m_ShaderModuleErrorCallback = nullptr;
        std::vector<VkPipelineShaderStageCreateInfo> m_PipelineShaderStageCreateInfos;
        std::unordered_map<uint32_t, std::vector<VkDescriptorPoolSize>> m_TypeCounts;
        std::filesystem::path m_AssetPath;
        bool m_DisableOptimization = false;
        std::string m_Name;
        uint64_t m_Hash = 0;
        void* m_Resource = nullptr;

		std::map<VkShaderStageFlagBits, std::vector<uint32_t>> m_ShaderData;
        ReflectionData m_ReflectionData;

        std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
        VkDescriptorSet m_DescriptorSet;

	    ShaderType m_ShaderType = ShaderType::MaxEnum;
        ShaderType m_VertexType = ShaderType::MaxEnum;

	    friend class ShaderCache;
        friend class ShaderPack;
        friend class VulkanShaderCompiler;
	};

    // -------------------------------------------------------

    class ShaderLibrary : public RefCounted
	{
	public:
	    ShaderLibrary() = default;
        virtual ~ShaderLibrary() override;

	    void Add(const Ref<Shader> &shader);
	    void Load(std::string_view path, bool forceCompile = false, bool disableOptimization = false);
	    void Load(std::string_view name, const std::string &path);
	    void LoadShaderPack(const std::filesystem::path &path);

		const Ref<Shader> &Get(const std::string& name) const;
		size_t GetSize() const { return m_Shaders.size(); }

		std::unordered_map<std::string, Ref<Shader>> &GetShaders() { return m_Shaders; }
		const std::unordered_map<std::string, Ref<Shader>> &GetShaders() const { return m_Shaders; }

	private:
		std::unordered_map<std::string, Ref<Shader>> m_Shaders;
		Ref<ShaderPack> m_ShaderPack;
	};

}

// -------------------------------------------------------

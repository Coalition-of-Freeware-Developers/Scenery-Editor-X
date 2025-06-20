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
#include <functional>
#include <SceneryEditorX/core/pointers.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
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
		/**
		 * @typedef ShaderReloadedCallback.
		 * @brief Function type for shader reload notifications
		 * 
		 * Called when a shader is reloaded to allow dependents to update resources.
		 */
		using ShaderReloadedCallback = std::function<void()>;

		//using ShaderModuleErrorCallback = void (*)(RenderContext*, Ref<Shader>, int line, int col, const char* debugName);

        /**
         * @brief Default constructor.
         * 
         * Creates an uninitialized shader instance that must be loaded before use.
         */
        Shader() = default;

        /**
         * @brief Construct shader from file.
         * 
         * @param filepath Path to the shader file, relative to shader directory
         * @param forceCompile Whether to force recompilation even if cached version exists
         * @param disableOptimization Whether to disable shader optimization during compilation
         * @param name Name of the shader
         */
        Shader(const std::string &filepath);
        
        /**
         * @brief Virtual destructor.
         * 
         * Cleans up Vulkan shader module resources.
         */
        virtual ~Shader() override;

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

		//virtual const std::unordered_map<std::string, ShaderBuffer>& GetShaderBuffers() const = 0;
		//virtual const std::unordered_map<std::string, ShaderResourceDeclaration>& GetResources() const = 0;

		/**
		 * @brief Register a callback to be invoked when shader is reloaded.
		 * 
		 * @param callback Function to call when shader is recompiled or reloaded
		 */
		virtual void AddShaderReloadedCallback(const ShaderReloadedCallback& callback);
        
        /**
         * @brief Get the name of the shader.
         * 
         * @return const std::string& The shader name, typically derived from the filename
         */
        [[nodiscard]] virtual const std::string &GetName() const;

	    /**
	     * @brief Get the base directory path for shader assets.
	     * 
	     * @return const char* Path to the shader directory
	     */
	    GLOBAL constexpr const char* GetShaderDirectoryPath() { return "assets/shaders/"; }

	    /**
	     * @brief Create a Vulkan shader module from compiled bytecode.
	     * 
	     * @param code Vector containing the compiled shader bytecode
	     * @return VkShaderModule The created Vulkan shader module
	     */
	    [[nodiscard]] VkShaderModule CreateShaderModule(const std::vector<char> &code) const;

	private:
        /** @brief Vulkan shader module handle */
        VkShaderModule shaderModule = VK_NULL_HANDLE;
        
        /** @brief List of callbacks to invoke when shader is reloaded */
        std::vector<ShaderReloadedCallback> reloadCallbacks;
        //ShaderModuleErrorCallback shaderModuleErrorCallback = nullptr;
		std::string name;
	};

} // namespace SceneryEditorX

/// -------------------------------------------------------

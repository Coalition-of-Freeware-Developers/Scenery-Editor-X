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
#include <SceneryEditorX/vulkan/vk_device.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace ShaderStage
	{
		enum Stage
		{
		    Vertex = 0x00000001,
		    Geometry = 0x00000008,
		    Fragment = 0x00000010,
		    Compute = 0x00000020,
		    AllGraphics = 0x0000001F,
		    All = 0x7FFFFFFF,
		};
	
	}

	// -------------------------------------------------------

	class Shader
	{
	public:
		using ShaderReloadedCallback = std::function<void()>;

        Shader() = default;
        Shader(const std::string &filepath, bool forceCompile = false, bool disableOptimization = false);
        virtual ~Shader();

		void LoadFromShaderPack(const std::string& filepath, bool forceCompile = false, bool disableOptimization = false);
        static Ref<Shader> CreateFromString(const std::string &source);

		//virtual const std::unordered_map<std::string, ShaderBuffer>& GetShaderBuffers() const = 0;
		//virtual const std::unordered_map<std::string, ShaderResourceDeclaration>& GetResources() const = 0;

		virtual void AddShaderReloadedCallback(const ShaderReloadedCallback& callback);
        [[nodiscard]] virtual const std::string &GetName() const;

	    static constexpr const char* GetShaderDirectoryPath()
        {
            return "assets/shaders/";
        }

	    [[nodiscard]] VkShaderModule CreateShaderModule(const std::vector<char> &code) const;

	private:
        Ref<VulkanDevice> device;
        VkShaderModule shaderModule = VK_NULL_HANDLE;
        std::vector<ShaderReloadedCallback> reloadCallbacks;
	};

} // namespace SceneryEditorX

// -------------------------------------------------------

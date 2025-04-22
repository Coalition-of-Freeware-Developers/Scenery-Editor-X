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

#include <SceneryEditorX/renderer/vk_device.h>

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
		static Ref<Shader> Create(const std::string& filepath, bool forceCompile = false, bool disableOptimization = false);
		static Ref<Shader> LoadFromShaderPack(const std::string& filepath, bool forceCompile = false, bool disableOptimization = false);
		static Ref<Shader> CreateFromString(const std::string& source);

		//virtual const std::unordered_map<std::string, ShaderBuffer>& GetShaderBuffers() const = 0;
		//virtual const std::unordered_map<std::string, ShaderResourceDeclaration>& GetResources() const = 0;

		virtual void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) = 0;
        virtual const std::string &GetName() const = 0;

	    static constexpr const char* GetShaderDirectoryPath()
        {
            return "assets/shaders/";
        }

		VkShaderModule CreateShaderModule(const std::vector<char> &code) const;
	private:
        Ref<VulkanDevice> device;

	};

} // namespace SceneryEditorX

// -------------------------------------------------------

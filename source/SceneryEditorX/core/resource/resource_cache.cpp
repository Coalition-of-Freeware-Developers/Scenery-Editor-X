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
 * resource_cache.cpp
 * -------------------------------------------------------
 * Created: 10/01/2025
 * -------------------------------------------------------
 */
#include "resource_cache.h"
#include "SceneryEditorX/core/platform/filesystem/file_manager.hpp"
#include <tracy/Tracy.hpp>

// -----------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		std::array<std::string, 6> m_standard_resource_directories;
		char s_Project_Directory[256] = {};
		//std::vector<Ref<Resource>> m_resources;
		std::mutex s_Mutex;
		bool s_Use_RootShaderDirectory = false;
		//std::unordered_map<IconType, Ref<Texture>> m_default_icons;
	} // namespace
	
	void ResourceCache::Initialize()
	{
	    // create project directory
	    SetProjectDirectory("project\\");
	
	    // add engine standard resource directories
	    const std::string data_dir = std::string(GetDataDirectory()) + "\\";
	    AddResourceDirectory(ResourceDirectory::Environment, std::string(s_Project_Directory) + "environment");
	    AddResourceDirectory(ResourceDirectory::Fonts, data_dir + "fonts");
	    AddResourceDirectory(ResourceDirectory::Icons, data_dir + "icons");
	    AddResourceDirectory(ResourceDirectory::ShaderCompiler, data_dir + "shader_compiler");
	    AddResourceDirectory(ResourceDirectory::Shaders, data_dir + "shaders");
	    AddResourceDirectory(ResourceDirectory::Textures, data_dir + "textures");
	}
	
	/*
	void ResourceCache::Shutdown()
	{
	    uint32_t resource_count = static_cast<uint32_t>(m_resources.size());
	    m_resources.clear();
	    if (resource_count != 0)
	    {
	        SEDX_CORE_INFO("%d resources have been cleared", resource_count);
	    }
	}
	*/
	
	/*
	void ResourceCache::LoadDefaultResources()
	{
	    const std::string dataDir = std::string(GetDataDirectory()) + "\\";
	
	    m_default_icons[IconType::Console]			= Load<Texture>(dataDir + "icons\\console.png");
	    m_default_icons[IconType::File]				= Load<Texture>(dataDir + "icons\\file.png");
	    m_default_icons[IconType::Folder]			= Load<Texture>(dataDir + "icons\\folder.png");
	    m_default_icons[IconType::Audio]			= Load<Texture>(dataDir + "icons\\audio.png");
	    m_default_icons[IconType::Model]			= Load<Texture>(dataDir + "icons\\model.png");
	    m_default_icons[IconType::World]			= Load<Texture>(dataDir + "icons\\world.png");
	    m_default_icons[IconType::Material]			= Load<Texture>(dataDir + "icons\\material.png");
	    m_default_icons[IconType::Shader]			= Load<Texture>(dataDir + "icons\\shader.png");
	    m_default_icons[IconType::Xml]				= Load<Texture>(dataDir + "icons\\xml.png");
	    m_default_icons[IconType::Dll]				= Load<Texture>(dataDir + "icons\\dll.png");
	    m_default_icons[IconType::Txt]				= Load<Texture>(dataDir + "icons\\txt.png");
	    m_default_icons[IconType::Ini]				= Load<Texture>(dataDir + "icons\\ini.png");
	    m_default_icons[IconType::Exe]				= Load<Texture>(dataDir + "icons\\exe.png");
	    m_default_icons[IconType::Font]				= Load<Texture>(dataDir + "icons\\font.png");
	    m_default_icons[IconType::Screenshot]		= Load<Texture>(dataDir + "icons\\screenshot.png");
	    m_default_icons[IconType::Gear]				= Load<Texture>(dataDir + "icons\\gear.png");
	    m_default_icons[IconType::Play]				= Load<Texture>(dataDir + "icons\\play.png");
	    m_default_icons[IconType::Profiler]			= Load<Texture>(dataDir + "icons\\timer.png");
	    m_default_icons[IconType::ResourceCache]	= Load<Texture>(dataDir + "icons\\resource_viewer.png");
	    m_default_icons[IconType::RenderDoc]		= Load<Texture>(dataDir + "icons\\renderdoc.png");
	    m_default_icons[IconType::Shader]			= Load<Texture>(dataDir + "icons\\code.png");
	    m_default_icons[IconType::Texture]			= Load<Texture>(dataDir + "icons\\texture.png");
	    m_default_icons[IconType::Minimize]			= Load<Texture>(dataDir + "icons\\window_minimise.png");
	    m_default_icons[IconType::Maximize]			= Load<Texture>(dataDir + "icons\\window_maximise.png");
	    m_default_icons[IconType::Close]			= Load<Texture>(dataDir + "icons\\window_close.png");
	    m_default_icons[IconType::Hybrid]			= Load<Texture>(dataDir + "icons\\hybrid.png");
	    m_default_icons[IconType::Audio]			= Load<Texture>(dataDir + "icons\\audio.png");
	    m_default_icons[IconType::Terrain]			= Load<Texture>(dataDir + "icons\\terrain.png");
	    m_default_icons[IconType::Entity]			= Load<Texture>(dataDir + "icons\\entity.png");
	    m_default_icons[IconType::Light]			= Load<Texture>(dataDir + "icons\\light.png");
	    m_default_icons[IconType::Camera]			= Load<Texture>(dataDir + "icons\\camera.png");
	    m_default_icons[IconType::Physics]			= Load<Texture>(dataDir + "icons\\physics.png");
	    m_default_icons[IconType::Compressed]		= Load<Texture>(dataDir + "icons\\compressed.png");
	}
	*/
	
	/*
	void ResourceCache::UnloadDefaultResources()
	{
	    m_default_icons.clear();
	}
	*/
	
	/*
	Ref<IResource> &ResourceCache::GetByName(const std::string &name, const ResourceType type)
	{
        std::lock_guard<std::mutex> guard(m_mutex);
	    for (Ref<IResource> &resource : m_resources)
	    {
	        if (name == resource->GetObjectName())
	            return resource;
	    }
	    static Ref<IResource> empty;
	    return empty;
	}
	*/

    /*
    std::vector<Ref<IResource>> ResourceCache::GetByType(const ResourceType type /*= ResourceType::Unknown#1#)
	{
        std::lock_guard<std::mutex> guard(m_mutex);
        std::vector<Ref<IResource>> resources;
	    for (Ref<IResource> &resource : m_resources)
	    {
	        if (resource->GetResourceType() == type || type == ResourceType::MaxEnum)
	        {
	            resources.emplace_back(resource);
	        }
	    }
	    return resources;
	}
	*/
	
	/*
	uint64_t ResourceCache::GetMemoryUsage(ResourceType type /*= Resource_Unknown#1#)
	{
        std::lock_guard<std::mutex> guard(m_mutex);
	    uint64_t size = 0;
	    for (Ref<IResource> &resource : m_resources)
	    {
	        if (resource->GetResourceType() == type || type == ResourceType::MaxEnum)
	        {
	            if (SpartanObject *object = dynamic_cast<SpartanObject *>(resource.get()))
	            {
	                size += object->GetObjectSize();
	            }
	        }
	    }
	    return size;
	}
	*/
	
	/*
	uint32_t ResourceCache::GetResourceCount(const ResourceType type)
	{
	    return static_cast<uint32_t>(GetByType(type).size());
	}
	*/
	
	void ResourceCache::AddResourceDirectory(const ResourceDirectory type, const std::string &directory)
	{
	    m_standard_resource_directories[static_cast<uint32_t>(type)] = directory;
	}

    /*
    std::string ResourceCache::GetResourceDirectory(const ResourceDirectory resource_directory_type)
	{
        std::string directory = m_standard_resource_directories[static_cast<uint32_t>(resource_directory_type)];
	    if (s_Use_Root_Shader_Directory)
	    {
	        if (resource_directory_type == ResourceDirectory::Shaders)
	        {
	            directory = "..\\" + directory;
	        }
	    }
	    return directory;
	}
	*/
	
	/*
	void ResourceCache::SetProjectDirectory(const char *directory)
	{
	    if (!IO::FileSystem::Exists(directory))
	    {
            IO::FileSystem::CreateDirectory(directory);
	    }
	
	    strcpy_s(m_project_directory, sizeof(m_project_directory), directory);
	    m_project_directory[sizeof(m_project_directory) - 1] = '\0'; // ensure null-termination
	}
	*/

    std::string ResourceCache::GetProjectDirectoryAbsolute()
	{
	    return IO::FileSystem::GetWorkingDirectory() + "/" + s_Project_Directory;
	}
	
	const char *ResourceCache::GetProjectDirectory()
	{
	    return s_Project_Directory;
	}
	
	const char *ResourceCache::GetDataDirectory()
	{
	    return "Data";
	}

    /*
    std::vector<Ref<IResource>> &ResourceCache::GetResources()
	{
	    return m_resources;
	}
	*/

    std::mutex &ResourceCache::GetMutex()
	{
	    return s_Mutex;
	}
	
	/*
	bool ResourceCache::GetUseRootShaderDirectory()
	{
	    return s_Use_Root_Shader_Directory;
	}
	
	void ResourceCache::SetUseRootShaderDirectory(const bool _use_root_shader_directory)
	{
	    s_Use_Root_Shader_Directory = _use_root_shader_directory;
	}
	*/
	
	/*
	Texture *ResourceCache::GetIcon(IconType type)
	{
	    auto it = m_default_icons.find(type);
	
	    if (it != m_default_icons.end())
	        return it->second.get();
	
	    return m_default_icons[IconType::File].get();
	}
	*/
}

// -----------------------------------------------------------

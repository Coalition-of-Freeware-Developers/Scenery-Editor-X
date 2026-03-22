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
#include "SceneryEditorX/renderer/vulkan/image_resource.h"
#include <SceneryEditorX/filesystem/file_manager.hpp>
#include <SceneryEditorX/renderer/vulkan/enums.h>

// -----------------------------------------------------------

namespace SceneryEditorX
{

	static std::array<std::string, 6> s_StandardResourceDir;
	static char s_ProjectDir[256] = {};
	static std::vector<Ref<SharedResource>> s_Resources;
	static std::vector<Ref<RefCounted>> s_ResourceReferences;
	static std::mutex s_Mutex;
	static bool s_Use_RootShaderDirectory = false;
	std::unordered_map<IconType, Ref<ImageResource>> m_DefaultIcons;

	void ResourceCache::Init()
	{
		// create project directory
		SetProjectDirectory("..\\project\\");

		// add engine standard resource directories
		// Note: fonts may live under the project's 'resources' folder rather than the engine 'Data' folder.
		// Prefer checking the repo 'resources' path for fonts so editor UI can reliably load bundled fonts.
		const std::string dataDir = std::string(GetResourceDirectory()) + "\\";
		AddResourceDirectory(ResourceDirectory::Environment, std::string(s_ProjectDir) + "environment");

		// Fonts live under the repository 'assets' folder — prefer that so editor finds bundled fonts
		AddResourceDirectory(ResourceDirectory::Fonts, std::string("assets\\fonts"));

		// Use dataDir as the base for engine-provided resources and append subfolders
		AddResourceDirectory(ResourceDirectory::Icons, dataDir + "icons");
		AddResourceDirectory(ResourceDirectory::Shaders, dataDir + "shaders");
		AddResourceDirectory(ResourceDirectory::Textures, dataDir + "textures");
	}

	void ResourceCache::Shutdown()
	{
		uint32_t resourceCount = static_cast<uint32_t>(s_Resources.size());
		s_Resources.clear();
		s_ResourceReferences.clear();
		if (resourceCount != 0)
		{
			SEDX_CORE_TRACE_TAG("ResourceCache", "%d resources have been cleared", resourceCount);
		}
	}

	void ResourceCache::UnloadDefaultResources()
	{
		/*m_default_icons.clear();*/
	}

	/*
	void ResourceCache::LoadDefaultResources()
	{
		const std::string dataDir = std::string(GetResourceDirectory()) + "\\";
	
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

	IResource *ResourceCache::GetByName(const std::string &name, const ResourceType type)
	{
		std::scoped_lock guard(s_Mutex);
		for (const Ref<SharedResource> &resource : s_Resources)
		{
			if (resource && name == resource->GetObjectName() && (type == ResourceType::MaxEnum || resource->GetResourceType() == type))
				return resource.Get();
		}

		return nullptr;
	}

	std::vector<IResource *> ResourceCache::GetByType(const ResourceType type /*= ResourceType::Unknown*/)
	{
		std::scoped_lock guard(s_Mutex);
		std::vector<IResource *> resources;
		for (const Ref<SharedResource> &resource : s_Resources)
		{
			if (resource && (resource->GetResourceType() == type || type == ResourceType::MaxEnum))
			{
				resources.emplace_back(resource.Get());
			}
		}
		return resources;
	}

	uint64_t ResourceCache::GetMemoryUsage(ResourceType type /*= Resource_Unknown*/)
	{
		std::scoped_lock guard(s_Mutex);
		uint64_t size = 0;
		for (const Ref<SharedResource> &resource : s_Resources)
		{
			if (resource && (resource->GetResourceType() == type || type == ResourceType::MaxEnum))
			{
			   size += resource->GetObjectSize();
			}
		}
		return size;
	}
	
	uint32_t ResourceCache::GetResourceCount(const ResourceType type)
	{
		return static_cast<uint32_t>(GetByType(type).size());
	}
	
	void ResourceCache::AddResourceDirectory(const ResourceDirectory type, const std::string &directory)
	{
		s_StandardResourceDir[static_cast<uint32_t>(type)] = directory;
	}

	std::string ResourceCache::GetResourceDirectory(const ResourceDirectory resourceDirType)
	{
		std::string directory = s_StandardResourceDir[static_cast<uint32_t>(resourceDirType)];
		if (s_Use_RootShaderDirectory)
		{
			if (resourceDirType == ResourceDirectory::Shaders)
			{
				directory = "..\\" + directory;
			}
		}
		return directory;
	}
	
	void ResourceCache::SetProjectDirectory(const char *directory)
	{
		if (directory == nullptr || directory[0] == '\0')
		{
			s_ProjectDir[0] = '\0';
			return;
		}

		IO::FileSystem::CreateDir(std::filesystem::path(directory));

		std::strncpy(s_ProjectDir, directory, sizeof(s_ProjectDir) - 1);
		s_ProjectDir[sizeof(s_ProjectDir) - 1] = '\0';
	}

	std::string ResourceCache::GetProjectDirectoryAbsolute()
	{
		return IO::FileSystem::GetWorkingDirectory() + "/" + s_ProjectDir;
	}
	
	const char *ResourceCache::GetProjectDirectory()
	{
		return s_ProjectDir;
	}
	
	const char *ResourceCache::GetResourceDirectory()
	{
		return "resources";
	}

	std::vector<Ref<SharedResource>> &ResourceCache::GetResources()
	{
		return s_Resources;
	}

	std::vector<Ref<RefCounted>> &ResourceCache::GetResourceReferences()
	{
		return s_ResourceReferences;
	}

	std::mutex &ResourceCache::GetMutex()
	{
		return s_Mutex;
	}

	bool ResourceCache::GetUseRootShaderDirectory()
	{
		return s_Use_RootShaderDirectory;
	}
	
	void ResourceCache::SetUseRootShaderDirectory(const bool useRootShaderDirectory)
	{
		s_Use_RootShaderDirectory = useRootShaderDirectory;
	}

	ImageResource *ResourceCache::GetIcon(IconType type)
	{
		auto it = m_DefaultIcons.find(type);
	
		if (it != m_DefaultIcons.end())
			return it->second.Get();
	
		return m_DefaultIcons[IconType::File].Get();
	}

}

// -----------------------------------------------------------

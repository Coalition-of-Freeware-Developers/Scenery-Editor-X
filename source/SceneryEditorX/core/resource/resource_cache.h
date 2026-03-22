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
 * resource_cache.h
 * -------------------------------------------------------
 * Created: 10/01/2025
 * -------------------------------------------------------
 */
#pragma once
#include "iresource.h"
#include "SceneryEditorX/utils/inheritance.h"

#include <algorithm>
#include <type_traits>
#include <SceneryEditorX/renderer/vulkan/enums.h>
#include <SceneryEditorX/utils/pointers.h>

// -----------------------------------------------------------

namespace SceneryEditorX
{
	class ImageResource;

	/**
	 * @enum ResourceType
	 * @brief Represents different types of resource directories used in the application.
	 */
	enum class ResourceDirectory : uint8_t
	{
		Environment,
		Fonts,
		Icons,
		Shaders,
		Textures
	};

	/**
	 * @enum IconType
	 * @brief Represents different types of icons used in the application.
	 */
	enum class IconType : uint8_t
	{
		Undefined,
		Console,
		File,
		Folder,
		Model,
		World,
		Material,
		Shader,
		Edx,
		Obj,
		Apt,
		For,
		Xml,
		Cfg,
		Dll,
		Txt,
		Ini,
		Exe,
		Font,
		Screenshot,
		Gear,
		Play,
		Profiler,
		ResourceCache,
		RenderDoc,
		Texture,
		Logo,
		Minimize,
		Maximize,
		Close,
		Entity,
		Hybrid,
		Audio,
		Terrain,
		Light,
		Camera,
		Physics,
		Compressed,
		MaxEnum
	};

	/**
	 * @class ResourceCache
	 * @brief Manages the caching and retrieval of resources in the application.
	 */
	class ResourceCache
	{
	public:
		/* @brief Initializes the resource cache. */
		static void Init();

		/* @brief Shuts down the resource cache. */
		static void Shutdown();

		/* @brief Loads the default resources into the cache. */
		static void LoadDefaultResources();

		/* @brief Unloads the default resources from the cache. */
		static void UnloadDefaultResources();

		/**
		 * @brief Retrieves a resource by its name and type.
		 * @param name The name of the resource.
		 * @param type The type of the resource.
		 * @return A reference to the requested resource.
		 */
		static IResource *GetByName(const std::string &name, ResourceType type);

		/**
		 * @brief Retrieves a resource by its name and type.
		 * @tparam T The type of the resource.
		 * @param name The name of the resource.
		 * @return A reference to the requested resource.
		 */
		template <class T>
		static Ref<T> GetByName(const std::string &name)
		{
				static_assert(std::is_base_of_v<SharedResource, T>, "Type must inherit from SharedResource (RefCounted + IResource)");

			IResource *raw = GetByName(name, IResource::TypeToEnum<T>());
			return raw ? Ref<T>(static_cast<T *>(raw)) : nullptr;
		}

		/**
		 * @brief Retrieves resources by their type.
		 * @param type The type of the resources.
		 * @return A vector of references to the requested resources.
		 */
		static std::vector<IResource *> GetByType(ResourceType type = ResourceType::MaxEnum);

		/**
		 * @brief Retrieves a resource by its file path.
		 * @tparam T The type of the resource.
		 * @param path The file path of the resource.
		 * @return A reference to the requested resource.
		 */
		template <class T>
		static Ref<T> GetByPath(const std::string &path)
		{
			static_assert(std::is_base_of_v<SharedResource, T>, "Type must inherit from SharedResource (RefCounted + IResource)");

			for (const Ref<SharedResource> &resource : GetResources())
			{
			    if (resource && path == resource->GetResourceFilePath())
					return resource.As<T>();
			}
			return nullptr;
		}

		/**
		 * @brief Caches a resource, or replaces it with an existing cached resource if it already exists.
		 * @tparam T The type of the resource.
		 * @param resource The resource to be cached.
		 * @return A reference to the cached resource.
		 */
		template <class T>
		static Ref<T> Cache(const Ref<T> resource)
		{
			static_assert(std::is_base_of_v<SharedResource, T>, "Type must inherit from SharedResource (RefCounted + IResource)");

			if (!resource)
				return nullptr;
	
			if (resource->GetResourceFilePath().empty())
			{
				SEDX_CORE_ERROR("Resource \"%s\" has an empty file path and cannot be cached.", resource->GetObjectName().c_str());
				return nullptr;
			}
	
			// return cached resource if it already exists
			Ref<T> existing = GetByPath<T>(resource->GetResourceFilePath());
			if (existing != nullptr)
				return existing;
	
			// if not, cache it and return the cached resource
			std::scoped_lock guard(GetMutex());
			GetResources().emplace_back(resource);
			return resource;
		}

		/**
		 * @brief Loads a resource and adds it to the resource cache.
		 * @tparam T The type of the resource.
		 * @param filePath The file path of the resource.
		 * @param flags Optional flags for loading the resource.
		 * @return A reference to the loaded resource.
		 */
		template <class T>
		static Ref<T> Load(const std::string &filePath, uint32_t flags = 0)
		{
			static_assert(std::is_base_of_v<SharedResource, T>, "Type must inherit from SharedResource (RefCounted + IResource)");

			if (!IO::FileSystem::Exists(filePath))
			{
				SEDX_CORE_ERROR("\"%s\" doesn't exist.", filePath.c_str());
				return nullptr;
			}
	
			// return cached resource if it already exists
			const std::string name = IO::FileSystem::GetFileNameWithoutExtensionFromFilePath(filePath);
			if (Ref<T> existing = GetByPath<T>(filePath); existing != nullptr)
				return existing;
	
			// create new resource
			Ref<T> resource = CreateRef<T>();
			if (flags != 0)
			{
				resource->SetFlags(flags);
			}
			resource->SetResourceFilePath(filePath);
			resource->LoadFromFile(filePath);
			return Cache<T>(resource); // cache and return
		}

		/**
		 * @brief Removes a resource from the cache.
		 * @tparam T The type of the resource.
		 * @param resource The resource to be removed.
		 */
		template <class T>
		static void Remove(Ref<T> &resource)
		{
			static_assert(std::is_base_of_v<SharedResource, T>, "Type must inherit from SharedResource (RefCounted + IResource)");

			if (!resource)
				return;

			std::scoped_lock guard(GetMutex());
			const uint64_t objectId = resource->GetObjectId();
			GetResources().erase(std::remove_if(GetResources().begin(), GetResources().end(),
			 [objectId](const Ref<SharedResource> &cachedResource)
				{
					return cachedResource && cachedResource->GetObjectId() == objectId;
				}),
				GetResources().end());
		}

		/**
		 * @brief Retrieves the memory usage of resources.
		 * @param type The type of the resources.
		 * @return The memory usage of the requested resources.
		 */
		static uint64_t GetMemoryUsage(ResourceType type = ResourceType::MaxEnum);

		/**
		 * @brief Retrieves the count of resources.
		 * @param type The type of the resources.
		 * @return The count of the requested resources.
		 */
		static uint32_t GetResourceCount(ResourceType type = ResourceType::MaxEnum);

		/**
		 * @brief Adds a resource directory for a specific type.
		 * @param type The type of the resource directory.
		 * @param directory The path of the resource directory.
		 */
		static void AddResourceDirectory(ResourceDirectory type, const std::string &directory);

		/**
		 * @brief Retrieves the directory path for a specific resource type.
		 * @param type The type of the resource directory.
		 * @return The path of the requested resource directory.
		 */
		static std::string GetResourceDirectory(ResourceDirectory type);

		/**
		 * @brief Sets the project directory.
		 * @param directory The path of the project directory.
		 */
		static void SetProjectDirectory(const char *directory);

		/**
		 * @brief Retrieves the absolute path of the project directory.
		 * @return The absolute path of the project directory.
		 */
		static std::string GetProjectDirectoryAbsolute();

		/**
		 * @brief Retrieves the project directory.
		 * @return The path of the project directory.
		 */
		static const char *GetProjectDirectory();

		/**
		 * @brief Retrieves the data directory.
		 * @return The path of the data directory.
		 */
		static const char *GetResourceDirectory();

		/**
		 * @brief Retrieves the list of resources.
		 * @return A reference to the vector of resources.
		 */
		static std::vector<Ref<SharedResource>> &GetResources();

		/**
		 * @brief 
		 * @return 
		 */
		static std::vector<Ref<RefCounted>> &GetResourceReferences();

		/**
		 * @brief Retrieves the mutex for resource management.
		 * @return A reference to the mutex.
		 */
		static std::mutex &GetMutex();

		/**
		 * @brief Retrieves whether the root shader directory is used.
		 * @return True if the root shader directory is used, false otherwise.
		 */
		static bool GetUseRootShaderDirectory();

		/**
		 * @brief Sets whether the root shader directory is used.
		 * @param useRootShaderDirectory True to use the root shader directory, false otherwise.
		 */
		static void SetUseRootShaderDirectory(const bool useRootShaderDirectory);

		/**
		 * @brief 
		 * @param type 
		 * @return 
		 */
		static ImageResource *GetIcon(IconType type);
	};
}

// -----------------------------------------------------------

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
 * iresource.h
 * -------------------------------------------------------
 * Created: 16/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "iobject.h"
#include <SceneryEditorX/filesystem/file_manager.hpp>
#include <SceneryEditorX/renderer/vulkan/enums.h>

// -----------------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @enum ResourceType
	 * @brief Represents different types of resources used in the application.
	 */
	enum class ResourceState : uint8_t
	{
		LoadingFromDrive,
		PreparingForGpu,
		PreparedForGpu,
		MaxEnum
	};

	/**
	 * @class IResource
	 * @brief Represents a generic resource in the application, providing common functionality for all resource types.
	 */
	class IResource : public IObject
	{
	public:
		IResource(ResourceType type);
		virtual ~IResource() = default;

		/**
		 * @brief Sets the file path for the resource.
		 * @param path The file path to set for the resource.
		 */
		void SetResourceFilePath(const std::string &path)
		{
			m_Resource_FilePath = IO::FileSystem::GetRelativePath(path);
			m_ObjectName = IO::FileSystem::GetFileNameWithoutExtensionFromFilePath(m_Resource_FilePath);
		}

		/**
		 * @brief Sets the name of the resource.
		 * @param name The name to set for the resource.
		 */
		void SetResourceName(const std::string &name)
		{
			m_ObjectName = name;
			m_Resource_FilePath = IO::FileSystem::GetDirectoryFromFilePath(m_Resource_FilePath) + name;
		}

		/**
		 * @brief Gets the type of the resource.
		 * @return The type of the resource.
		 */
		ResourceType GetResourceType() const
		{
			return m_ResourceType;
		}

		/**
		 * @brief Gets the type of the resource as a C-style string.
		 * @return The type of the resource as a C-style string.
		 */
		const char *GetResourceTypeCstr() const
		{
			return typeid(*this).name();
		}

		/**
		 * @brief Gets the file path of the resource.
		 * @return The file path of the resource.
		 */
		const std::string &GetResourceFilePath() const
		{
			return m_Resource_FilePath;
		}

		/**
		 * @brief Gets the directory of the resource file.
		 * @return The directory of the resource file.
		 */
		std::string GetResourceDirectory() const
		{
			return IO::FileSystem::GetDirectoryFromFilePath(m_Resource_FilePath);
		}

		/**
		 * @brief Sets a flag for the resource.
		 * @param flag The flag to set.
		 * @param enabled Whether to enable or disable the flag.
		 */
		void SetFlag(const uint32_t flag, bool enabled = true)
		{
			if (enabled)
			{
				m_Flags |= flag;
			}
			else
			{
				m_Flags &= ~flag;
			}
		}

		/**
		 * @brief Gets the flags of the resource.
		 * @return The flags of the resource.
		 */
		virtual uint32_t GetFlags() const { return m_Flags; }

		/**
		 * @brief Sets the flags for the resource.
		 * @param flags The flags to set.
		 */
		virtual void SetFlags(const uint32_t flags) { m_Flags = flags; }

		/**
		 * @brief Saves the resource to a file.
		 * @param filePath The file path to save the resource to.
		 */
		virtual void SaveToFile(const std::string &filePath) {}

		/**
		 * @brief Loads the resource from a file.
		 * @param filePath The file path to load the resource from.
		 */
		virtual void LoadFromFile(const std::string &filePath) {}

		/**
		 * @brief Converts a type to its corresponding ResourceType enum value.
		 * @tparam T The type to convert.
		 * @return The corresponding ResourceType enum value.
		 */
		template <typename T>
		static ResourceType TypeToEnum();

		/**
		 * @brief Gets the current state of the resource.
		 * @return The current state of the resource.
		 */
		ResourceState GetResourceState() const { return m_ResourceState; }
	
	protected:
		ResourceType m_ResourceType = ResourceType::MaxEnum;
		std::atomic<ResourceState> m_ResourceState = ResourceState::MaxEnum;
		uint32_t m_Flags = 0;
	
	private:
		std::string m_Resource_FilePath;
	};

}

// -----------------------------------------------------------------

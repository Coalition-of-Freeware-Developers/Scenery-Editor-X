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
#include "SceneryEditorX/core/platform/filesystem/file_manager.hpp"
#include "SceneryEditorX/renderer/vulkan/enums.h"

// -----------------------------------------------------------------

namespace SceneryEditorX
{
	enum class ResourceState : uint8_t
	{
	    LoadingFromDrive,
	    PreparingForGpu,
	    PreparedForGpu,
	    MaxEnum
	};
	
	class IResource : public IObject
	{
	public:
	    IResource(ResourceType type);
	    virtual ~IResource() = default;
	
	    void SetResourceFilePath(const std::string &path)
	    {
	        m_Resource_FilePath = IO::FileSystem::GetRelativePath(path);
	        m_ObjectName = IO::FileSystem::GetFileNameWithoutExtensionFromFilePath(m_Resource_FilePath);
	    }
	
	    void SetResourceName(const std::string &name)
	    {
            m_ObjectName = name;
	        m_Resource_FilePath = IO::FileSystem::GetDirectoryFromFilePath(m_Resource_FilePath) + name;
	    }
	
	    ResourceType GetResourceType() const
	    {
	        return m_ResourceType;
	    }
	    const char *GetResourceTypeCstr() const
	    {
	        return typeid(*this).name();
	    }
	    const std::string &GetResourceFilePath() const
	    {
	        return m_Resource_FilePath;
	    }
	    const std::string GetResourceDirectory() const
	    {
	        return IO::FileSystem::GetDirectoryFromFilePath(m_Resource_FilePath);
	    }
	
	    // flags
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

	    uint32_t GetFlags() const { return m_Flags; }
	    void SetFlags(const uint32_t flags) { m_Flags = flags; }
	
	    // io
	    virtual void SaveToFile(const std::string &filePath) {}
	    virtual void LoadFromFile(const std::string &filePath) {}
	
	    // type
	    template <typename T>
	    static ResourceType TypeToEnum();
	
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

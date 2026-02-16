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
 * asset_manager.cpp
 * -------------------------------------------------------
 * Created: 12/02/2026
 * -------------------------------------------------------
 */
#include "asset_manager.h"
#include <tracy/Tracy.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{

	bool AssetManager::AddAsset(VmaAllocator allocator, VkCommandPool cmdPool, VkQueue queue, const std::string &modelFile, const std::vector<std::string> &textureFiles, const VmaAllocationCreateInfo &modelAllocInfo)
	{
	    m_AssetSets.emplace_back(CreateScope<Asset>());
	    if (!m_AssetSets.back()->Load(allocator, cmdPool, queue, modelFile, textureFiles, modelAllocInfo))
	    {
	        m_AssetSets.pop_back();
	        return false;
	    }
	    return true;
	}

    /**
	 * @brief Destroy all assets managed by this AssetManager instance.
	 * 
	 * This method iterates through all loaded assets and properly destroys their
	 * Vulkan resources using the device's memory allocator, then clears the asset list.
	 */
    void AssetManager::DestroyAll()
    {
        SEDX_CORE_INFO_TAG("AssetManager", "Destroying all assets ({} total)", m_AssetSets.size());

        Ref<Device> device = RenderContext::Get()->GetDevice();
        SEDX_CORE_ASSERT(device.IsValid(), "Device must be valid to destroy assets");

        VmaAllocator allocator = device->GetMemoryAllocator()->GetAllocator();

        for (auto &asset : m_AssetSets)
        {
            if (asset)
            {
                asset->Destroy(allocator);
            }
        }

        m_AssetSets.clear();

        SEDX_CORE_INFO_TAG("AssetManager", "All assets destroyed");
    }


} // namespace SceneryEditorX

// -------------------------------------------------------

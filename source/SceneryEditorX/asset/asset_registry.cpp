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
 * asset_registry.cpp
 * -------------------------------------------------------
 * Created: 10/03/2026
 * -------------------------------------------------------
 */
#include "asset_registry.h"
#include "SceneryEditorX/core/application/application.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	const AssetMetadata& AssetRegistry::Get(const AssetHandle handle) const
	{
		SEDX_CORE_ASSERT(m_AssetRegistry.contains(handle));
		LOG_ASSET("Retrieving const handle {}", handle);
		return m_AssetRegistry.at(handle);
	}

	void AssetRegistry::Set(const AssetHandle handle, const AssetMetadata& metadata)
	{
		SEDX_CORE_ASSERT(metadata.handle == handle);
		SEDX_CORE_ASSERT(handle != 0);
		SEDX_CORE_ASSERT(Application::IsMainThread(), "AssetRegistry::Set() has been called from other than the main thread!");
		m_AssetRegistry[handle] = metadata;
	}

	bool AssetRegistry::Contains(const AssetHandle handle) const
	{
		LOG_ASSET("Contains handle {}", handle);
		return m_AssetRegistry.contains(handle);
	}
	
	size_t AssetRegistry::Remove(const AssetHandle handle)
	{
		LOG_ASSET("Removing handle {}", handle);
		return m_AssetRegistry.erase(handle);
	}

	void AssetRegistry::Clear()
	{
		LOG_ASSET("Clearing registry");
		m_AssetRegistry.clear();
	}

}

// -------------------------------------------------------

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
 * runtime_asset_system.cpp
 * -------------------------------------------------------
 * Created: 19/03/2026
 * -------------------------------------------------------
 */
#include "runtime_asset_system.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	RuntimeAssetSystem::RuntimeAssetSystem() : m_Thread("Asset Thread")
	{
		m_Thread.Dispatch([this]() { AssetThreadFunc(); });
	}

	RuntimeAssetSystem::~RuntimeAssetSystem()
	{
		StopAndWait();
	}

	void RuntimeAssetSystem::Stop()
	{
		m_Running = false;
		m_AssetLoadingQueueCV.notify_one();
	}

	void RuntimeAssetSystem::StopAndWait()
	{
		Stop();
		m_Thread.Join();
	}

	void RuntimeAssetSystem::AssetThreadFunc()
	{
		//SEDX_PROFILE_THREAD("Asset Thread");

		while (m_Running)
		{
			//SEDX_PROFILE_SCOPE("Asset Thread Queue");

			// Go through queue and see what needs loading
			bool queueEmptyOrStop = false;
			while (!queueEmptyOrStop)
			{
				RuntimeAssetLoadRequest alr;
				{
					std::scoped_lock lock(m_AssetLoadingQueueMutex);
					if (m_AssetLoadingQueue.empty() || !m_Running)
					{
						queueEmptyOrStop = true;
					}
					else
					{
						alr = m_AssetLoadingQueue.front();
						m_AssetLoadingQueue.pop();
					}
				}

				// If queueEmptyOrStop then request will be invalid (Handle == 0)
				// We check handles here (instead of just breaking straight away on queueEmptyOrStop)
				// to deal with the edge case that other thread might queue requests for invalid assets.
				// This way, we just pop those requests and ignore them.
				if (alr.handle != 0 && alr.sceneHandle != 0)
				{
					//TryLoadData(alr);
					SEDX_CORE_TRACE_TAG("AssetSystem", "Processing asset load request {}", ToString(alr.handle));
					SEDX_CORE_TRACE_TAG("AssetSystem", "Attempted to execute a non-implemented function.");
					return;
				}
			}

			std::unique_lock<std::mutex> lock(m_AssetLoadingQueueMutex);
			// need to check conditions again, since other thread could have changed them between releasing the lock (in the while loop above)
			// and re-acquiring the lock here
			if (m_AssetLoadingQueue.empty() && m_Running)
			{
				m_AssetLoadingQueueCV.wait(lock, [this]
					{
					return !m_Running || !m_AssetLoadingQueue.empty();
				});
			}
		}
	}

	/*
	Ref<Asset> RuntimeAssetSystem::TryLoadData(const RuntimeAssetLoadRequest& alr)
	{
		SEDX_CORE_INFO_TAG("AssetSystem", "Loading asset {}", alr.handle);
		Ref<Asset> asset;
		// TODO: we need to handle the case where the asset is already loaded (e.g. if multiple assets depend on the same asset, or if the same asset is used in multiple scenes). 
		// For now, we just load it again, which is inefficient and could cause issues if the asset is not immutable.
		return asset;
	}
	*/

	void RuntimeAssetSystem::QueueAssetLoad(const RuntimeAssetLoadRequest& request)
	{
		{
			std::scoped_lock lock(m_AssetLoadingQueueMutex);
			m_AssetLoadingQueue.push(request);
		}
		m_AssetLoadingQueueCV.notify_one();
	}

	Ref<Asset> RuntimeAssetSystem::GetAsset(const AssetHandle &sceneHandle, const AssetHandle &assetHandle)
	{
		{
			std::scoped_lock lock(m_AMLoadedAssetsMutex);
			if (auto it = m_AMLoadedAssets.find(assetHandle); it != m_AMLoadedAssets.end())
			{
				return it->second;
			}
		}
		/*return TryLoadData({ sceneHandle, assetHandle });*/
		SEDX_CORE_TRACE_TAG("AssetSystem", "Attempted to execute a non-implemented function.");
		return nullptr;
	}

	bool RuntimeAssetSystem::RetrieveReadyAssets(std::vector<Ref<Asset>>& outAssetList)
	{
		if (m_LoadedAssets.empty())
		{
			return false;
		}

		std::scoped_lock lock(m_LoadedAssetsMutex);
		outAssetList = m_LoadedAssets;
		m_LoadedAssets.clear();
		return true;
	}

	void RuntimeAssetSystem::UpdateLoadedAssetList(const std::unordered_map<AssetHandle, Ref<Asset>>& loadedAssets)
	{
		std::scoped_lock<std::mutex> lock(m_AMLoadedAssetsMutex);
		m_AMLoadedAssets = loadedAssets;
	}

}

// -------------------------------------------------------

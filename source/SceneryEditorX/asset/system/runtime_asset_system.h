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
 * runtime_asset_system.h
 * -------------------------------------------------------
 * Created: 19/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <atomic>
#include <mutex>
#include <queue>
#include <SceneryEditorX/asset/asset_metadata.h>
#include <SceneryEditorX/core/threading/thread.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @class RuntimeAssetLoadRequest
	 * @brief A request to load an asset on the asset thread.  
	 * This is the "thread local" version of AssetMetadata, which is used for communication between main thread and asset thread.
	 */
	class RuntimeAssetSystem : public RefCounted
	{
	public:
		RuntimeAssetSystem();
		virtual ~RuntimeAssetSystem();

		// Queue an asset for later loading (on asset thread)
		void QueueAssetLoad(const RuntimeAssetLoadRequest& request);

		// Get an asset immediately (on asset thread).
		// If the asset needs to be loaded, it will be loaded into "ready assets" and transfered back to main thread
		// at next asset sync.
		Ref<Asset> GetAsset(const AssetHandle &sceneHandle, const AssetHandle &assetHandle);

		// Retrieve assets that have been loaded (from and earlier request)
		bool RetrieveReadyAssets(std::vector<Ref<Asset>>& outAssetList);

		// Replace the currently loaded asset collection with the given loadedAssets.
		// This is effectively takes a "thread local" snapshot of the asset manager's loaded assets.
		void UpdateLoadedAssetList(const std::unordered_map<AssetHandle, Ref<Asset>>& loadedAssets);

		//void SetAssetPack(Ref<AssetPack> assetPack) { m_AssetPack = assetPack; }

		void Stop();
		void StopAndWait();

	private:
		// The asset thread's mainline
		void AssetThreadFunc();

		//Ref<Asset> TryLoadData(const RuntimeAssetLoadRequest& request);

	private:
		Thread m_Thread;
		std::atomic<bool> m_Running = true; // not false. This ensures that if Stop() is called after the thread is dispatched but before it actually starts running, then the thread is correctly stopped.

		//Ref<AssetPack> m_AssetPack;

		std::queue<RuntimeAssetLoadRequest> m_AssetLoadingQueue;
		std::mutex m_AssetLoadingQueueMutex;
		std::condition_variable m_AssetLoadingQueueCV;

		std::vector<Ref<Asset>> m_LoadedAssets; // Assets that have been loaded asynchronously and are waiting for sync back to Asset Manager
		std::mutex m_LoadedAssetsMutex;

		std::unordered_map<AssetHandle, Ref<Asset>> m_AMLoadedAssets;  // All currently loaded assets (synced from Asset Manager)
		std::mutex m_AMLoadedAssetsMutex;
	};

}

// -------------------------------------------------------

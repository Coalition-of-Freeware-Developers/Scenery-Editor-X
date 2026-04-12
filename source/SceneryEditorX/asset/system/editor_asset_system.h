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
 * editor_asset_system.h
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
	 * @class EditorAssetSystem
	 * @brief The EditorAssetSystem is responsible for managing asset loading and monitoring for changes to asset files in the editor. 
	 * It runs on a separate thread to avoid blocking the main thread during asset loading operations. 
	 * The system allows queuing assets for loading, retrieving loaded assets, and monitoring for changes to asset files to trigger reloads when necessary.
	 */
	class EditorAssetSystem : public RefCounted
	{
	public:
		EditorAssetSystem();
		~EditorAssetSystem();

		// Queue an asset to be loaded on asset thread later
		void QueueAssetLoad(const AssetMetadata& request);

		// Get an asset immediately (on asset thread).
		// If the asset needs to be loaded, it will be loaded into "ready assets" and transferred back to main thread
		// at next asset sync.
		Ref<Asset> GetAsset(const AssetMetadata& request);

		// Retrieve assets that have been loaded (from and earlier request)
		bool RetrieveReadyAssets(std::vector<EditorAssetLoadResponse>& outAssetList);

		// Replace the currently loaded asset collection with the given loadedAssets.
		// This effectively takes a "thread local" snapshot of the asset manager's loaded assets.
		void UpdateLoadedAssetList(const std::unordered_map<AssetHandle, Ref<Asset>>& loadedAssets);

		void Stop();
		void StopAndWait();

		// Monitor for updated assets, and if any are found queue them for reload
		void AssetMonitorUpdate();

	private:
		// The asset thread's mainline
		void AssetThreadFunc();

		std::filesystem::path GetFileSystemPath(const AssetMetadata& metadata);

		void EnsureAllLoadedCurrent();
		void EnsureCurrent(AssetHandle assetHandle);
		Ref<Asset> TryLoadData(AssetMetadata metadata);

		Thread m_Thread;
		std::atomic<bool> m_Running = true;  // not false. This ensures that if Stop() is called after the thread is dispatched but before it actually starts running, then the thread is correctly stopped.

		std::queue<AssetMetadata> m_AssetLoadingQueue;
		std::mutex m_AssetLoadingQueueMutex;
		std::condition_variable m_AssetLoadingQueueCV;

		std::vector<EditorAssetLoadResponse> m_LoadedAssets; // Assets that have been loaded asynchronously and are waiting for sync back to Asset Manager
		std::mutex m_LoadedAssetsMutex;

		std::unordered_map<AssetHandle, Ref<Asset>> m_AMLoadedAssets;  // All currently loaded assets (synced from Asset Manager)
		std::mutex m_AMLoadedAssetsMutex;

		// Asset Monitoring
		float m_AssetUpdatePerf = 0.0f;
	};

}

// -------------------------------------------------------

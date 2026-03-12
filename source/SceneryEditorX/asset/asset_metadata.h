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
 * asset_metadata.h
 * -------------------------------------------------------
 * Created: 10/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include "asset.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @enum AssetStatus
	 * @brief Represents the status of an asset, indicating whether it is ready, invalid, or currently loading.
	 */
	enum class AssetStatus : uint8_t
	{
		None	= 0,
		Ready	= 1, 
		Invalid = 2, 
		Loading = 3
	};

	/**
	 * @struct AssetMetadata
	 * @brief Metadata for an asset, including its handle, type, file path, status, and other relevant information.
	 */
	struct AssetMetadata
	{
		AssetHandle handle;
		AssetType type;
		std::filesystem::path filePath;

		AssetStatus status = AssetStatus::None;

		uint64_t fileLastWriteTime = 0; // TODO: this is the last write time of the file WE LOADED
		bool isDataLoaded = false;

		[[nodiscard]] bool IsValid() const { return handle != 0; }

		/// UUID copy-assignment is deleted, so provide an explicit operator= that
		/// copy-constructs a temporary UUID then move-assigns it.
		AssetMetadata& operator=(const AssetMetadata& other)
		{
			if (this != &other)
			{
				handle            = AssetHandle(other.handle);
				type              = other.type;
				filePath          = other.filePath;
				status            = other.status;
				fileLastWriteTime = other.fileLastWriteTime;
				isDataLoaded      = other.isDataLoaded;
			}
			return *this;
		}
	};

}

// -------------------------------------------------------

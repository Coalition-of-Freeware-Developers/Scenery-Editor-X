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
 * model_importer.h
 * -------------------------------------------------------
 * Created: 19/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/asset/asset_metadata.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @class ModelImporter
	 * @brief ModelImporter is responsible for importing 3D models into the engine.
	 */
	class ModelImporter
	{
	public:
		static void Init();
		static bool TryLoadData(const AssetMetadata& metadata, const Ref<Asset>& asset);
		static void RegisterDependencies(const AssetMetadata& metadata);

		static void Serialize(const Ref<Asset>& asset);
		static void Serialize(const AssetMetadata& metadata, Ref<Asset>& asset);

	private:
		//static std::unordered_map<AssetType, Scope<AssetSerializer>> s_Serializers;
	};

}

// -------------------------------------------------------

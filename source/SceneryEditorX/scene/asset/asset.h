/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asset.h
* -------------------------------------------------------
* Created: 2/4/2025
* -------------------------------------------------------
*/

#pragma once
#include <SceneryEditorX/scene/scene_ref.h>
#include <SceneryEditorX/scene/asset/asset_enums.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	using AssetHandle = UUID; //Uncomment this line when the UUID class is implemented

	class Asset : public RefCounted
	{
	public:
        //AssetHandle Handle = 0;
        uint16_t Flags = (uint16_t)AssetFlag::None;

        virtual ~Asset()
        {
        }

        static AssetType GetStaticType() { return AssetType::None; }
        virtual AssetType GetAssetType() const { return AssetType::None; }

		//virtual bool operator==(const Asset& other) const
		//{
		//	return Handle == other.Handle;
		//}
		
		//virtual bool operator!=(const Asset& other) const
		//{
		//	return !(*this == other);
		//}

	private:
        //friend class EditorAssetManager;
        //friend class TextureSerializer;

		bool IsValid() const { return ((Flags & (uint16_t)AssetFlag::Missing) | (Flags & (uint16_t)AssetFlag::Invalid)) == 0; }

		bool IsFlagSet(AssetFlag flag) const { return (uint16_t)flag & Flags; }
		void SetFlag(AssetFlag flag, bool value = true)
		{
			if (value)
				Flags |= (uint16_t)flag;
			else
				Flags &= ~(uint16_t)flag;
		}

	};

} // namespace SceneryEditorX

// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* editor_events.h
* -------------------------------------------------------
* Created: 2/4/2025
* -------------------------------------------------------
*/

#pragma once

#include <SceneryEditorX/core/events.h>
#include <SceneryEditorX/scene/asset/asset.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class EditorEvent : public Event
	{
    public:
        EditorEvent() = default;
        EVENT_CLASS_CATEGORY(EventCategory::EventCategoryEditor)
	};

	// -------------------------------------------------------

	class AssetReload : public Event
    {
    public:
        AssetReload(AssetHandle assetHandle) : AssetHandle(assetHandle)
        {
        }

        EVENT_CLASS_TYPE(AssetReload)
        EVENT_CLASS_CATEGORY(EventCategory::EventCategoryEditor)

		AssetHandle AssetHandle;
    };

} // namespace SceneryEditorX

// -------------------------------------------------------

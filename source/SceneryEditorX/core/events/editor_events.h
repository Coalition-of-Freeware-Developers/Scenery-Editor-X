/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* editor_events.h
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/core/events/event_system.h>

/// -------------------------------------------------------------------

namespace SceneryEditorX
{
	class EditorExitPlayModeEvent : public Event
	{
	public:
		EditorExitPlayModeEvent() = default;

        [[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "EditorExitPlayModeEvent";
			return ss.str();
		}

		EVENT_CLASS_TYPE(EditorExitPlayMode)
		EVENT_CLASS_CATEGORY(EventCategoryEditor)
	};

    /// -------------------------------------------------------------------

	class AssetReloadedEvent : public Event
	{
	public:
        explicit AssetReloadedEvent(const AssetHandle assetHandle) : AssetHandle(assetHandle) {}

        [[nodiscard]] virtual std::string ToString() const override
		{
			std::stringstream ss;
			ss << "AssetReloadedEvent";
			return ss.str();
		}

		EVENT_CLASS_TYPE(AssetReloaded)
	    EVENT_CLASS_CATEGORY(EventCategoryEditor)

		AssetHandle AssetHandle;
	};

}

/// -------------------------------------------------------------------

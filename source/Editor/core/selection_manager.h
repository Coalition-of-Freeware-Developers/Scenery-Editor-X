/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* selection_manager.h
* -------------------------------------------------------
* Created: 11/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/identifiers/uuid.h>
#include <SceneryEditorX/scene/entity.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	class SelectionManager
	{
	public:
		GLOBAL void Select(UUID contextID, UUID itemID);
		GLOBAL bool IsSelected(UUID itemID);
		GLOBAL bool IsSelected(UUID contextID, UUID itemID);
		GLOBAL bool IsEntityOrAncestorSelected(const Entity entity);
		GLOBAL bool IsEntityOrAncestorSelected(UUID contextID, const Entity entity);
		GLOBAL void Deselect(UUID selectionID);
		GLOBAL void Deselect(UUID contextID, UUID itemID);
		GLOBAL void DeselectAll();
		GLOBAL void DeselectAll(UUID contextID);
		GLOBAL UUID GetSelection(UUID contextID, size_t index);

		GLOBAL size_t GetSelectionCount(UUID contextID);
		inline GLOBAL const std::vector<UUID>& GetSelections(UUID contextID) { return s_Contexts[contextID]; }

	private:
		inline INTERNAL std::unordered_map<UUID, std::vector<UUID>> s_Contexts;
	};
}

/// -------------------------------------------------------

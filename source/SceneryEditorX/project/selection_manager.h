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
 * selection_manager.h
 * -------------------------------------------------------
 * Created: 22/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <unordered_map>

// ---------------------------------------------------------

namespace SceneryEditorX
{
	class Entity;

	/**
	 * @enum SelectionContext
	 * @brief Defines the context in which an entity selection is made. 
	 */
	enum class SelectionContext : uint8_t
	{
		Global				= 0, 
		Scene				= 1, 
		ContentBrowser		= 2, 
	};

	/**
	 * @class SelectionManager
	 * @brief Manages entity selections across different contexts.
	 */
	class SelectionManager
	{
	public:
		/**
		 * @brief Selects an entity in the given context.
		 * @param context The selection context.
		 * @param selectionID The ID of the entity to select.
		 */
		static void Select(SelectionContext context, UUID selectionID);

		/**
		 * @brief Checks if an entity is selected in any context.
		 * @param selectionID ID of the entity to check for selection.
		 * @return True if the entity is selected, false otherwise.
		 */
		static bool IsSelected(const UUID &selectionID);

		/**
		 * @brief Checks if an entity is selected in a specific context.
		 * @param context The selection context.
		 * @param selectionID ID of the entity to check for selection.
		 * @return True if the entity is selected in the specified context, false otherwise.
		 */
		static bool IsSelected(SelectionContext context, const UUID &selectionID);

		/**
		 * @brief Checks if an entity or any of its ancestors is selected in any context.
		 * @param entity The entity to check for selection.
		 * @return True if the entity or any of its ancestors is selected, false otherwise.
		 */
		static bool IsEntityOrAncestorSelected(const Entity &entity);

		/**
		 * @brief Checks if an entity or any of its ancestors is selected in a specific context.
		 * @param context The selection context.
		 * @param entity The entity to check for selection.
		 * @return True if the entity or any of its ancestors is selected in the specified context, false otherwise.
		 */
		static bool IsEntityOrAncestorSelected(SelectionContext context, const Entity &entity);

		/**
		 * @brief Deselects an entity in any context.
		 * @param selectionID ID of the entity to deselect.
		 */
		static void Deselect(UUID selectionID);

		/**
		 * @brief Deselects an entity in a specific context.
		 * @param context The selection context.
		 * @param selectionID ID of the entity to deselect.
		 */
		static void Deselect(SelectionContext context, const UUID &selectionID);

		/**
		 * @brief Deselects all entities in any context.
		 */
		static void DeselectAll();

		/**
		 * @brief Deselects all entities in a specific context.
		 * @param context The selection context.
		 */
		static void DeselectAll(SelectionContext context);

		/**
		 * @brief Gets the ID of the selected entity at the specified index in a specific context.
		 * @param context The selection context.
		 * @param index The index of the selected entity.
		 * @return The ID of the selected entity at the specified index.
		 */
		static UUID GetSelection(SelectionContext context, size_t index);

		/**
		 * @brief Gets the number of selected entities in a specific context.
		 * @param contextID The selection context.
		 * @return The number of selected entities in the specified context.
		 */
		static size_t GetSelectionCount(SelectionContext contextID);

		/**
		 * @brief Gets the selected entities in a specific context.
		 * @param context The selection context.
		 * @return A reference to the vector of selected entity IDs in the specified context.
		 */
		inline static const std::vector<UUID>& GetSelections(SelectionContext context) { return s_Contexts[context]; }

	private:
		// Map of selection contexts to their selected entity IDs
		inline static std::unordered_map<SelectionContext, std::vector<UUID>> s_Contexts; 
	};

}

// -------------------------------------------------------

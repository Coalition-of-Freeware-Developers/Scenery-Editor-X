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
	 * @brief 
	 */
	enum class SelectionContext : uint8_t
	{
		Global				= 0, 
		Scene				= 1, 
		ContentBrowser		= 2, 
	};

	/**
	 * @class SelectionManager
	 * @brief 
	 */
	class SelectionManager
	{
	public:
		static void Select(SelectionContext context, UUID selectionID);
		static bool IsSelected(const UUID &selectionID);
		static bool IsSelected(SelectionContext context, const UUID &selectionID);
		static bool IsEntityOrAncestorSelected(const Entity &entity);
		static bool IsEntityOrAncestorSelected(SelectionContext context, const Entity &entity);
		static void Deselect(UUID selectionID);
		static void Deselect(SelectionContext context, const UUID &selectionID);
		static void DeselectAll();
		static void DeselectAll(SelectionContext context);
		static UUID GetSelection(SelectionContext context, size_t index);

		static size_t GetSelectionCount(SelectionContext contextID);
		inline static const std::vector<UUID>& GetSelections(SelectionContext context) { return s_Contexts[context]; }

	private:
		inline static std::unordered_map<SelectionContext, std::vector<UUID>> s_Contexts;
	};

}

// -------------------------------------------------------

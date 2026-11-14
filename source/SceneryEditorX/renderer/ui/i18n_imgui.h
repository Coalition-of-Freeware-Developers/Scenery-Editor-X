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
 * i18n_imgui.h
 * -------------------------------------------------------
 * Created: 13/11/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/i18n/tr.h>
#include <string>
#include <string_view>

// -------------------------------------------------------

/**
 * Translated Dear ImGui labels that keep their IDs. A label is also a widget's ID, so translating "Open" to
 * "Öffnen" would change the ID and lose the docking layout (editor.ini), tree / table state and window positions.
 * TrL returns "<translation>###<original label>" instead: since Dear ImGui 1.92.6 "a###b" hashes exactly like "b",
 * and only "b" is stored in editor.ini, so every ID and saved layout stays as it is in English.
 *
 *   ImGui::Begin(TrL("Preferences###EditorPreferences"));   -> "Einstellungen###EditorPreferences"
 *   ImGui::MenuItem(TrL("Reset Layout"));                   -> "Layout zuruecksetzen###Reset Layout"
 *
 * Use TrL for anything whose label is an ID (windows, menus, buttons, checkboxes, tabs, tree nodes, selectables,
 * popups, slider and input labels); Tr for display-only text. A popup opened by name keeps the English name:
 * OpenPopup("Delete?") still finds BeginPopupModal(TrL("Delete?")).
 *
 * Labels are cached per language, so steady frames don't allocate. UI thread only.
 */
namespace SceneryEditorX
{
	/**
	 * @brief A translated ImGui label with the original's ID. English (or an untranslated label) comes back unchanged.
	 * Takes a literal or a Msg (translated) or a run-time string (returned as it is). One overload only: a Msg one
	 * next to it would make TrL("literal") ambiguous.
	 */
	const char *TrL(const Text &label);

	namespace I18n
	{
		/**
		 * @brief "<shown>###<ID part of original>": the part after the original's "###" when it has one, else the whole
		 * original, so ImGui hashes the result like the original.
		 */
		std::string MakeLabel(std::string_view shown, std::string_view original);

		/* @brief Translates Dear ImGui's own strings (table and docking context menus). Call after each language change. */
		void ApplyImGuiLocalization();
	}
}

// -------------------------------------------------------

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
 * menu_bar.h
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX 
{ 
	class EditorLayer; 
	class UILayer;

	// -------------------------------------------------------
	
	class MenuBar
	{
	public:
		/**
		 * @brief Initialize the menu bar with the given editor
		 * @param editor The editor instance to associate with the menu bar
		 */
		static void Initialize(UILayer* editor);

		/**
		 * @brief Update the menu bar each frame
		 */
		static void Tick();

		/**
		 * @brief Set the internal editor pointer used by menu bar windows
		 * @param editor The editor instance to associate with the menu bar
		 */
		static void SetEditor(UILayer* editor);
	
		/**
		 * @brief Show the world save dialog
		 */
		static void ShowWorldSaveDialog();

		/**
		 * @brief Show the world load dialog
		 */
		static void ShowWorldLoadDialog();

		/**
		 * @brief Get the horizontal padding for the menu bar
		 * @return The horizontal padding value
		 */
		static float GetPaddingX() { return 14.0f; }

		/**
		 * @brief Get the vertical padding for the menu bar
		 * @return The vertical padding value
		 */
		static float GetPaddingY() { return 8.0f; }
	private:
		Ref<UILayer> m_Editor;
	};
}

// -------------------------------------------------------

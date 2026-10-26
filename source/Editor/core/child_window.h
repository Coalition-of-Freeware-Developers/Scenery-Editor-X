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
 * child_window.h
 * -------------------------------------------------------
 * Created: 20/03/2026
 * -------------------------------------------------------
 */
#pragma once

// ---------------------------------------------------------

namespace SceneryEditorX::UI
{

	/**
	 * @class ChildWindow
	 * @brief Base class for child windows in the UI.
	 * Provides common functionality for managing child windows, such as centering and visibility. 
	 */
	class ChildWindow
	{
	public:
		/**
		 * @brief Constructs a child window with the given name.
		 */
		virtual ~ChildWindow() = default;

		/**
		 * @brief Initializes the child window with the given name.
		 * @param name The name of the child window.
		 */
		static void Init(const std::string &name);

		/**
		 * @brief Centers the child window on the screen. 
		 * @note: This should be called after the window is created and its size is known. 
		 */
		virtual void CenterWindow();

		/**
		 * @brief Called each frame to update the layer.
		 * Implement layer logic that needs to execute each frame.
		 */
		virtual void Tick() = 0;

		/**
		 * @brief Child window visibility toggle. 
		 * This can be used to show/hide the window based on user interaction or application state. 
		 * @return True if the child window is visible, false otherwise.
		 */
		virtual bool IsVisible() = 0;

		/**
		 * @brief Shows the child window. 
		 * This should set the visibility state to true and trigger any necessary updates to display the window.
		 * @return True if the child window was successfully shown, false otherwise.
		 */
		virtual bool *ShowWindow() = 0;

	private:
		std::string m_DebugName; // The name of the layer.
	};

}

// ---------------------------------------------------------


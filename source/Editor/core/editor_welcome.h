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
 * editor_welcome.h
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX::EditorWelcome
{
	/**
	 * The first-launch setup (pick a navigation keymap: Blender, 3ds Max or Maya) and the Preferences window it leads
	 * to (Edit > Preferences): navigation keymap and options, and the controls of the chosen keymap.
	 *
	 * The welcome shows once, when the preferences say no keymap was chosen yet. Runs started with command-line flags
	 * (self-tests, captures, scripted exports) skip it unless --welcome asks for it.
	 */

	/* @brief Reads the command line (--welcome) and decides whether the welcome shows. Call once at startup. */
	void Init();

	/* @brief Draws the welcome modal and the Preferences window when they are open. Every frame. */
	void Draw();

	/* @brief Opens the Preferences window (on its navigation page). */
	void OpenPreferences();

	/* @brief Opens the welcome again (Help menu). */
	void OpenWelcome();
}

// -------------------------------------------------------

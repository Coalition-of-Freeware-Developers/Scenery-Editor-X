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
 * settings_panels.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/renderer/ui/editor_panel.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * The three tabbed panels in the lower-right dock (Substance's "Texture Set Settings / List / Shader Settings").
	 * Every control is bound to live engine state; nothing here is a placeholder.
	 */

	/**
	 * @class SceneSettingsPanel
	 * @brief "SCENE SETTINGS": active scene info plus the environment state X-Plane lighting depends on (time of day, wind).
	 */
	class SceneSettingsPanel : public UI::EditorPanel
	{
	public:
		SceneSettingsPanel();
		void OnUIRender(bool &isOpen) override;
	};

	/**
	 * @class CameraSettingsPanel
	 * @brief "CAMERA": navigation mode, projection, lens and exposure of the viewport camera.
	 */
	class CameraSettingsPanel : public UI::EditorPanel
	{
	public:
		CameraSettingsPanel();
		void OnUIRender(bool &isOpen) override;
	};

	/**
	 * @class RendererStatsPanel
	 * @brief "RENDERER": frame timing, resolutions, swapchain and GPU memory, and UI backend status.
	 */
	class RendererStatsPanel : public UI::EditorPanel
	{
	public:
		RendererStatsPanel();
		void OnUIRender(bool &isOpen) override;
	};

}

// -------------------------------------------------------

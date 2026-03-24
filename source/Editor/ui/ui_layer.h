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
 * ui_layer.h
 * -------------------------------------------------------
 * Created: 23/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include "ui_widget.h"
#include <Editor/core/child_window.h>
#include <SceneryEditorX/core/layers/layer.h>

// -------------------------------------------------------

/**
 * @class UILayer
 * @brief 
 */
class UILayer : public SceneryEditorX::Layer
{
public:
	UILayer();
	UILayer(const std::string &name);

	void Tick() override;
	void OnAttach() override;
	void OnDetach() override;
	void BeginWindow();
	void OnRender() override;

	static void SetDarkThemeColors();
	static void SetDarkThemeV2Colors();

	void AllowInputEvents(bool allowInput);

	inline static ImFont* fontNormal = nullptr;
	inline static ImFont* fontBold   = nullptr;

private:
	std::vector<SceneryEditorX::Ref<Widget>> m_Widgets;
	std::vector<UI::ChildWindow> m_ChildWindows;
};

// -------------------------------------------------------


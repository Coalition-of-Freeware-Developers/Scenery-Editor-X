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

namespace SceneryEditorX { class EditorLayer; }
class UILayer;

// -------------------------------------------------------

class MenuBar
{
public:
    static void Initialize(SceneryEditorX::EditorLayer* editor);
    // Convenience overload to allow UI layer to initialize the menu bar when
    // refactoring moves the call site. This forwards to the EditorLayer-based
    // initializer using a reinterpret_cast. The cast is kept explicit to
    // document the mismatch in types and avoid accidental implicit conversions.
    static void Initialize(UILayer* uiLayer)
    {
        Initialize(reinterpret_cast<SceneryEditorX::EditorLayer*>(uiLayer));
    }
    static void Tick();

    static void ShowWorldSaveDialog();
    static void ShowWorldLoadDialog();

    static float GetPaddingX() { return 14.0f; }
    static float GetPaddingY() { return 8.0f; }
private:
    SceneryEditorX::Ref<SceneryEditorX::EditorLayer> m_Editor;
};

// -------------------------------------------------------

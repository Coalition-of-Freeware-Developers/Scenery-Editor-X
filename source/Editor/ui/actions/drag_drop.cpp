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
 * drag_drop.cpp
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#include "drag_drop.h"
#include <Editor/ui/source/imgui/imgui.h>
#include <SceneryEditorX/core/resource/asset_resource.h>

// -------------------------------------------------------

using namespace SceneryEditorX;


DragDropPayload::DragDropPayload(const DragPayloadType type, const DataVariant data, const char* pathRelative) : type(type), data(data), m_RelativePath(pathRelative)
{
}

void DragDropPayload::CreateDragDropPayload(const DragDropPayload& payload)
{
	ImGui::SetDragDropPayload(DragDropTypes[(int)payload.type].data(), &payload, sizeof(payload), ImGuiCond_Once);
}

DragDropPayload* DragDropPayload::ReceiveDragDropPayload(DragPayloadType type)
{
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payloadImgui = ImGui::AcceptDragDropPayload(DragDropTypes[(int)type].data()))
		{
			return static_cast<DragDropPayload*>(payloadImgui->Data);
		}
		ImGui::EndDragDropTarget();
	}

	return nullptr;
}


// -------------------------------------------------------

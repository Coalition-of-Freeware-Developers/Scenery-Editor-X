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
 * drag_drop.h
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <variant>

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @enum DragPayloadType
	 * @brief Enum representing different types of drag-and-drop payloads in the editor.
	 */
	enum class DragPayloadType : uint8_t
	{
		Texture,
		Entity,
		Model,
		Audio,
		Material,
		Lua,
		Prefab,
		Undefined
	};
	
	constexpr std::string_view DragDropTypes[] = {
		"Texture",
		"Entity",
		"Model",
		"Audio",
		"Material",
		"Lua",
		"Prefab",
		"Undefined",
	};
	
	/**
	 * @class DragDropPayload
	 * @brief Class representing a drag-and-drop payload and operations in the editor.
	 */
	class DragDropPayload
	{
	public:
		typedef std::variant<const char *, uint64_t> DataVariant;
	
		DragDropPayload(const DragPayloadType type = DragPayloadType::Undefined, const DataVariant data = nullptr,  const char *pathRelative = nullptr);
	
		static void CreateDragDropPayload(const DragDropPayload &payload);
		static DragDropPayload *ReceiveDragDropPayload(DragPayloadType type);
		DataVariant GetData() { return data; };
	
	private:
		DragPayloadType type;
		DataVariant data;               // full/absolute path (for backward compatibility)
		const char* m_RelativePath;     // relative path
	};
	
}

// -------------------------------------------------------

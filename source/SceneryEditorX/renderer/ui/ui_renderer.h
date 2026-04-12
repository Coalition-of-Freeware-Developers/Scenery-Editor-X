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
 * ui_renderer.h
 * -------------------------------------------------------
 * Created: 09/04/2026
 * -------------------------------------------------------
 */
#pragma once
#include "source/imgui/imgui.h"
#include <SceneryEditorX/renderer/vulkan/command_list.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @class UIRenderer
	 * @brief Responsible for rendering ImGui draw data in the 
	 * Scenery Editor X application Vulkan renderer.
	 */
	class UIRenderer
	{
	public:
		UIRenderer() = default;

		/**
		 * @brief Initializes the UI renderer.
		 */
		void Init();
		//bool UpdateFontTexture();

	private:
		/**
		 * @brief Updates the geometry for the UI renderer.
		 * @param drawData ImGui draw data to update the geometry with.
		 * @return True if the geometry was successfully updated, false otherwise.
		 */
		bool UpdateGeometry(ImDrawData* drawData);

		Ref<CommandList> m_CommandList; // Command list used for recording ImGui draw calls, managed externally to allow for synchronization with the main rendering loop.

		std::vector<ImDrawVert> m_VertexBufferData; // Separate buffers to avoid reallocations when vertex/index counts differ.
		std::vector<ImDrawIdx> m_IndexBufferData;	// Separate buffers to avoid reallocations when vertex/index counts differ.

		uint64_t m_FrameCounter = 0; // Used for synchronization and resource management.
	};
	
}

// -------------------------------------------------------

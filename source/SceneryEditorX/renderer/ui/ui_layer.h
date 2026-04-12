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
 * Created: 09/04/2026
 * -------------------------------------------------------
 */
// ReSharper disable CppInconsistentNaming
#pragma once
#include "ui_renderer.h"
#include "ui_widget.h"

#include <SceneryEditorX/core/layers/layer.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class UILayer
	 * @brief A layer responsible for managing the ImGui user interface rendering and input handling.
	 */
	class UILayer : public Layer
	{
	public:
		UILayer() = default;

		/**
		 * @brief Creates a new instance of the UILayer.
		 * @return A pointer to the newly created UILayer instance.
		 */
		static UILayer *Create() { return new UILayer(); }

		/**
		 * @brief Gets the UIRenderer instance associated with this UILayer.
		 * @return A pointer to the UIRenderer instance.
		 */
		UIRenderer *Get();

		/**
		 * @brief Destroys the UILayer instance.
		 */
		virtual ~UILayer() = default;

		/**
		 * @brief Called when the UILayer is attached to the application. 
		 * Use this for initialization of UI resources.
		 */
		virtual void OnAttach() override;

		/**
		 * @brief Called when the UILayer is attached to the application.
		 */
		virtual void OnDetach() override;

		/**
		 * @brief Called each frame to update the UILayer. 
		 * Implement UI logic that needs to execute each frame.
		 */
		void Tick() override;

		/**
		 * @brief Begins the UI rendering process.
		 */
		void BeginRendering();

		/**
		 * @brief Ends the UI rendering process.
		 */
		void EndRendering();

		/**
		 * @brief Allows or disallows input events for the UI layer.
		 * @param allowEvents True to allow input events, false to disallow.
		 */
		void AllowInputEvents(bool allowEvents);

	    /**
		 * @brief Retrieves a pointer to the first widget of type T in the editor's widget list. Returns nullptr if no such widget is found.
		 * @tparam T The type of the widget to retrieve.
		 * @return A pointer to the first widget of type T, or nullptr if no such widget is found.
		 */
		template<typename T>
		T* GetWidget()
		{
			for (const auto& widget : m_Widgets)
			{
				if (T* widgetT = dynamic_cast<T*>(widget.Get()))
				{
					return widgetT;
				}
			}

			return nullptr;
		}

	private:
		Scope<UIRenderer> m_UIRenderer; // Renderer responsible for drawing the UI elements.
		ImFont *m_Font_Normal;          // Pointer to the normal font used in the UI.
		ImFont *m_Font_Bold;            // Pointer to the bold font used in the UI.
		float m_Time = 0.0f;            // Time accumulator for UI animations or updates.
	    std::vector<Ref<Widget>> m_Widgets;

		std::string m_IniFilePath;		// Owns the imgui ini path string so io.IniFilename never dangles (C26815).

	};
	
}

// -------------------------------------------------------

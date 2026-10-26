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
#include "editor_panel.h"
#include "ui_renderer.h"
#include <SceneryEditorX/core/layers/layer.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class UILayer
	 * @brief Owns the Dear ImGui context and its SDL3 platform / Vulkan renderer backends.
	 *
	 * The Application pushes this layer as an overlay and wraps every layer's OnUIRender() between
	 * BeginRendering() and EndRendering(). Docking and multi-viewport (floating panels outside the main
	 * window) are enabled. Themes are applied by the client layer after this layer attaches.
	 */
	class UILayer : public Layer
	{
	public:
		/**
		 * @param enableViewports Allow panels to be dragged out of the main window into their own OS windows.
		 */
		explicit UILayer(bool enableViewports = true);

		/**
		 * @brief Creates a new instance of the UILayer.
		 * @return A pointer to the newly created UILayer instance.
		 */
		static UILayer *Create() { return new UILayer(); }

		/**
		 * @brief Gets the UIRenderer instance associated with this UILayer.
		 * @return A pointer to the UIRenderer instance (unused by the Vulkan backend, kept for older panels).
		 */
		UIRenderer *Get();

		virtual ~UILayer() override = default;

		/* @brief Creates the ImGui context, loads the fonts and initializes the SDL3 and Vulkan backends. */
		virtual void OnAttach() override;

		/* @brief Shuts the backends down and destroys the ImGui context. */
		virtual void OnDetach() override;

		/* @brief Ticks the panels registered directly with this layer. */
		void Tick() override;

		/* @brief Starts a new ImGui frame. Must be paired with EndRendering(). */
		void BeginRendering();

		/* @brief Finalizes the ImGui frame for the render thread and renders secondary platform windows. */
		void EndRendering();

		/**
		 * @brief Allows or blocks keyboard/mouse input from reaching ImGui (e.g. while a modal engine tool owns input).
		 * @param allowEvents True to allow input events, false to disallow.
		 */
		void AllowInputEvents(bool allowEvents);

		/* @brief True between a successful OnAttach() and OnDetach(). */
		[[nodiscard]] bool IsInitialized() const { return m_Initialized; }

		/**
		 * @brief Retrieves a pointer to the first widget of type T in the editor's widget list. Returns nullptr if no such widget is found.
		 * @tparam T The type of the widget to retrieve.
		 * @return A pointer to the first widget of type T, or nullptr if no such widget is found.
		 */
		template<typename T>
		T* GetPanel()
		{
			for (const auto& widget : m_UIPanels)
			{
				if (T* panel = dynamic_cast<T*>(widget.Get()))
					return panel;
			}

			return nullptr;
		}

	private:
		/* @brief Loads the default UI fonts; FontAwesome is merged into the default font for inline icons. */
		void LoadFonts();

		Scope<UIRenderer> m_UIRenderer;					// Legacy hook for older panels; rendering goes through UI::RenderMainViewport().
		std::vector<Ref<UI::EditorPanel>> m_UIPanels;	// List of editor panels managed by the UILayer.
		std::string m_IniFilePath;						// Owns the imgui ini path string so io.IniFilename never dangles (C26815).
		bool m_EnableViewports = true;
		bool m_AllowInput = true;
		bool m_Initialized = false;
	};

}

// -------------------------------------------------------

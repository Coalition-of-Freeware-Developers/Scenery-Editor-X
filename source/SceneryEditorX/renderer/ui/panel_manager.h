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
 * panel_manager.h
 * -------------------------------------------------------
 * Created: 10/04/2026
 * -------------------------------------------------------
 */
#pragma once
#include "editor_panel.h"
#include "SceneryEditorX/core/identifiers/hash.h"
#include <array>
#include <unordered_map>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @struct PanelData
	 * @brief Contains data for a single editor panel.
	 */
	struct PanelData
	{
		const char *ID = "";					// Unique string identifier for the panel
		const char *Name = "";					// Display name for the panel (can be localized)
		Ref<UI::EditorPanel> Panel = nullptr;	// Reference to the actual panel instance
		bool IsOpen = false;					// Whether the panel is currently open (visible) in the UI
	};

	/**
	 * @enum PanelCategory
	 * @brief Represents the different categories of editor panels.
	 */
	enum class PanelCategory
	{
		Editor,
		Viewer,
		Settings,
		_COUNT
	};

	/**
	 * @class PanelManager
	 * @brief Manages all editor panels in the Scenery Editor X application.
	 */
	class PanelManager
	{
	public:
		PanelManager() = default;

		/**
		 * @brief Destroys the PanelManager and releases all associated resources.
		 */
		~PanelManager();

		/**
		 * @brief Retrieves the PanelData associated with the given panelID.
		 * @param panelID the unique identifier of the panel to retrieve
		 * @return A pointer to the PanelData associated with the given panelID, or nullptr if not found
		 */
	    PanelData* GetPanelData(uint64_t panelID);

		/**
		 * @brief Retrieves the PanelData associated with the given panelID.
		 * @param panelID the unique identifier of the panel to retrieve
		 * @return A pointer to the PanelData associated with the given panelID, or nullptr if not found
		 */
	    const PanelData* GetPanelData(uint64_t panelID) const;

		/**
		 * @brief Removes the panel associated with the given string ID.
		 * @param strID the unique string identifier of the panel to remove
		 */
		void RemovePanel(const char* strID);

		/**
		 * @brief Renders the UI for all panels managed by the PanelManager.
		 */
		void OnUIRender();

		/**
		 * @brief Handles events for all panels managed by the PanelManager.
		 * @param e the event to handle
		 */
		void OnEvent(Event& e);

		/**
		 * @brief Sets the scene context for all panels managed by the PanelManager.
		 * @param context the scene context to set
		 */
		void SetSceneContext(const Ref<Scene>& context);

		/**
		 * @brief Notifies the PanelManager that the project has changed.
		 * @param project the new project context
		 */
		void OnProjectChanged(const Ref<Project>& project);

		/**
		 * @brief Serializes the state of all panels managed by the PanelManager.
		 */
		void Serialize() const;

		/**
		 * @brief Deserializes the state of all panels managed by the PanelManager.
		 */
		void Deserialize();

		/**
		 * @brief Retrieves the panels associated with the given category.
		 * @param category the category of panels to retrieve
		 * @return A reference to the unordered_map containing the panels of the specified category
		 */
		std::unordered_map<uint64_t, PanelData>& GetPanels(PanelCategory category) { return m_Panels[static_cast<size_t>(category)]; }

		/**
		 * @brief Retrieves the panels associated with the given category.
		 * @param category the category of panels to retrieve
		 * @return A reference to the unordered_map containing the panels of the specified category
		 */
		const std::unordered_map<uint64_t, PanelData>& GetPanels(PanelCategory category) const { return m_Panels[static_cast<size_t>(category)]; }

		/**
		 * @brief Adds a panel to the PanelManager.
		 * @tparam TPanel The type of the panel to add. Must inherit from UI::EditorPanel.
		 * @param category The category of the panel to add.
		 * @param panelData The data of the panel to add.
		 * @return A reference to the added panel.
		 */
		template<typename TPanel>
		Ref<TPanel> AddPanel(PanelCategory category, const PanelData& panelData)
		{
			static_assert(std::is_base_of<UI::EditorPanel, TPanel>::value, "PanelManager::AddPanel requires TPanel to inherit from EditorPanel");

			auto& panelMap = m_Panels[static_cast<size_t>(category)];

			// Generate a 64-bit FNV-1a hash for the panel ID
			uint64_t id = Hash::GenerateFNV1A(panelData.ID);
			if (panelMap.contains(id))
			{
				SEDX_CORE_ERROR_TAG("PanelManager", "A panel with the id '{0}' has already been added.", panelData.ID);
				return nullptr;
			}

			panelMap[id] = panelData;
			return panelMap[id].Panel.As<TPanel>();
		}

		/**
		 * @template TPanel The type of the panel to add. Must inherit from UI::EditorPanel.
		 * @brief Adds a panel to the PanelManager.
		 * @tparam TPanel The type of the panel to add. Must inherit from UI::EditorPanel.
		 * @tparam TArgs The types of the arguments to forward to the panel's constructor.
		 * @param category The category of the panel to add.
		 * @param strID The unique string identifier of the panel to add.
		 * @param isOpenByDefault Whether the panel should be open by default.
		 * @param args args to forward to the panel's constructor
		 * @return A reference to the added panel.
		 */
		template<typename TPanel, typename... TArgs>
		Ref<TPanel> AddPanel(PanelCategory category, const char* strID, bool isOpenByDefault, TArgs&&... args)
		{
			return AddPanel<TPanel>(category, PanelData{ strID, strID, Ref<TPanel>::Create(std::forward<TArgs>(args)...), isOpenByDefault });
		}

		/**
		 * @brief Adds a panel to the PanelManager.
		 * @tparam TPanel The type of the panel to add. Must inherit from UI::EditorPanel.
		 * @tparam TArgs The types of the arguments to forward to the panel's constructor.
		 * @param category The category of the panel to add.
		 * @param strID The unique string identifier of the panel to add.
		 * @param displayName The display name of the panel to add.
		 * @param isOpenByDefault Whether the panel should be open by default.
		 * @param args args to forward to the panel's constructor
		 * @return A reference to the added panel.
		 */
		template<typename TPanel, typename... TArgs>
		Ref<TPanel> AddPanel(PanelCategory category, const char* strID, const char* displayName, bool isOpenByDefault, TArgs&&... args)
		{
			return AddPanel<TPanel>(category, PanelData{ strID, displayName, Ref<TPanel>::Create(std::forward<TArgs>(args)...), isOpenByDefault });
		}

		/**
		 * @brief Retrieves a panel from the PanelManager.
		 * @tparam TPanel The type of the panel to retrieve. Must inherit from UI::EditorPanel.
		 * @param strID The unique string identifier of the panel to retrieve.
		 * @return A reference to the retrieved panel, or nullptr if not found.
		 */
		template<typename TPanel>
		Ref<TPanel> GetPanel(const char* strID)
		{
			static_assert(std::is_base_of<UI::EditorPanel, TPanel>::value, "PanelManager::AddPanel requires TPanel to inherit from EditorPanel");


			uint64_t id = Hash::GenerateFNV1A(strID);

			for (const auto& panelMap : m_Panels)
			{
				if (!panelMap.contains(id))
					continue;

				return panelMap.at(id).Panel.As<TPanel>();
			}

			SEDX_CORE_ERROR_TAG("PanelManager", "Couldn't find panel with id '{0}'", strID);
			return nullptr;
		}

	private:
		// Panels are stored in separate maps based on their category for efficient retrieval and organization.
		std::array<std::unordered_map<uint64_t, PanelData>, (size_t)PanelCategory::_COUNT> m_Panels;
	};

}

// -------------------------------------------------------

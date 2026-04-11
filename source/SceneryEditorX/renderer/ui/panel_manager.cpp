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
 * panel_manager.cpp
 * -------------------------------------------------------
 * Created: 10/04/2026
 * -------------------------------------------------------
 */
#include "panel_manager.h"
#include "SceneryEditorX/logging/asserts.h"
#include <nlohmann/json.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	PanelManager::~PanelManager()
	{
		SEDX_CORE_TRACE_TAG("PanelManager", "Destroying PanelManager");
		for (auto& map : m_Panels)
		{
			map.clear();
		}
	}
	
	PanelData *PanelManager::GetPanelData(uint64_t panelID)
	{
		// Validate the panel ID if needed (no-op here)
		for (auto& panelMap : m_Panels)
		{
			if (!panelMap.contains(panelID))
				continue;

			return &panelMap.at(panelID);
		}

		SEDX_CORE_ERROR_TAG("PanelManager", "Couldn't find panel with ID: {}", panelID);
		return nullptr;
	}
	
	const PanelData *PanelManager::GetPanelData(uint64_t panelID) const
	{
		for (auto& panelMap : m_Panels)
		{
			if (!panelMap.contains(panelID))
				continue;

			return &panelMap.at(panelID);
		}

		return nullptr;
	}
	
	void PanelManager::RemovePanel(const char *strID)
	{
		SEDX_CORE_ASSERT(strID != nullptr, "Invalid panel ID");

		uint64_t id = Hash::GenerateFNV1A(strID);
		for (auto& panelMap : m_Panels)
		{
			if (!panelMap.contains(id))
				continue;

			panelMap.erase(id);
			return;
		}

		SEDX_CORE_ERROR_TAG("PanelManager", "Couldn't find panel with id '{0}'", strID);
	}
	
	void PanelManager::OnUIRender()
	{
		for (auto& panelMap : m_Panels)
		{
			for (auto &panelData : panelMap | std::views::values)
			{
				bool closedThisFrame = false;

				if (panelData.IsOpen)
				{
					SEDX_CORE_TRACE_TAG("PanelManager", "Rendering panel: {}", panelData.Name);
					panelData.Panel->OnUIRender(panelData.IsOpen);
					closedThisFrame = !panelData.IsOpen;
				}

				if (closedThisFrame)
				{
					SEDX_CORE_TRACE_TAG("PanelManager", "Panel closed this frame");
					Serialize();
				}
			}
		}
	}
	
	void PanelManager::OnEvent(Event &e)
	{
		for (auto& panelMap : m_Panels)
		{
			for (auto &panelData : panelMap | std::views::values)
			{
				panelData.Panel->OnEvent(e);
			}
		}
	}
	
	void PanelManager::SetSceneContext(const Ref<Scene> &context)
	{
		SEDX_CORE_ASSERT(context.Get(), "Invalid scene context");

		for (auto& panelMap : m_Panels)
		{
			for (auto &panelData : panelMap | std::views::values)
			{
				panelData.Panel->SetSceneContext(context);
			}
		}

		SEDX_CORE_TRACE_TAG("PanelManager", "Scene context set");

	}
	
	void PanelManager::OnProjectChanged(const Ref<Project> &project)
	{
		for (auto& panelMap : m_Panels)
		{
			for (auto &panelData : panelMap | std::views::values)
			{
				panelData.Panel->OnProjectChanged(project);
			}
		}

		Deserialize();
	}
	
	void PanelManager::Serialize() const
	{
		SEDX_CORE_TRACE_TAG("PanelManager", "Serializing panel layout");
		nlohmann::json root;
		root["Panels"] = nlohmann::json::array();

		for (const auto &panelMap : m_Panels)
		{
			for (const auto& [panelID, panel] : panelMap)
			{
				nlohmann::json entry;
				entry["ID"] = panelID;
				entry["Name"] = panel.Name;
				entry["IsOpen"] = panel.IsOpen;
				root["Panels"].push_back(std::move(entry));
			}
		}

		std::ofstream fout(IO::FileSystem::GetAppDataRoaming() / "EditorLayout.json");
		fout << root.dump(4);
		fout.close();

		SEDX_CORE_TRACE_TAG("PanelManager", "Finished serializing panel layout");
	}
	
	void PanelManager::Deserialize()
	{
		SEDX_CORE_TRACE_TAG("PanelManager", "Deserializing panel layout");
		std::filesystem::path layoutPath = IO::FileSystem::GetAppDataRoaming() / "EditorLayout.json";
		if (!IO::FileSystem::Exists(layoutPath))
			return;

		std::ifstream stream(layoutPath);
		SEDX_CORE_ASSERT(stream, "Failed to open layout file");

		std::stringstream ss;
		ss << stream.rdbuf();

		try
		{
			auto data = nlohmann::json::parse(ss.str());

			if (!data.contains("Panels") || !data["Panels"].is_array())
			{
				SEDX_CORE_ERROR_TAG("PanelManager","Failed to load EditorLayout.json from {} because the file appears to be corrupted!", layoutPath.parent_path().string());
				return;
			}

			for (const auto &panelNode : data["Panels"])
			{
				uint64_t id = panelNode.value("ID", 0ULL);
				PanelData* panelData = GetPanelData(id);
				SEDX_CORE_TRACE_TAG("PanelManager", "Deserializing panel: {}", panelData ? panelData->Name : "Unknown");

				if (panelData == nullptr)
					continue;

				panelData->IsOpen = panelNode.value("IsOpen", panelData->IsOpen);
			}
		}
		catch (const std::exception &e)
		{
			SEDX_CORE_ERROR_TAG("PanelManager", "Failed to parse EditorLayout.json: {0}", e.what());
			return;
		}
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

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
 * airport_panel.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <Editor/core/editor_context.h>
#include <SceneryEditorX/renderer/ui/editor_panel.h>
#include <SceneryEditorX/xplane/xp_airport_index.h>
#include <SceneryEditorX/xplane/xp_gateway.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class AirportPanel
	 * @brief "AIRPORTS" panel: find airports in the X-Plane install (Global Airports and every custom scenery pack),
	 * open them in the scene, create new ones, save them into scenery packs, and pull the latest submissions from the
	 * X-Plane Scenery Gateway.
	 *
	 * Tabs: Installed (the local index), Gateway (search the Gateway's airport list, browse an airport's submissions
	 * and install one), Airport (the open document: contents, metadata and validation).
	 */
	class AirportPanel : public UI::EditorPanel
	{
	public:
		explicit AirportPanel(EditorContext *context);

		void OnUIRender(bool &isOpen) override;

	private:
		void DrawStatusBar();
		void DrawInstalledTab();
		void DrawGatewayTab();
		void DrawAirportTab();
		void DrawNewAirportPopup();
		void DrawInstallPopup();
		void RefreshInstalledResults(const XPlane::AirportIndex &index);
		void RefreshGatewayResults();
		void SelectGatewayAirport(const std::string &code);

		EditorContext *m_Context = nullptr;

		// Installed tab
		ImGuiTextFilter m_InstalledFilter;
		int m_SourceFilter = -1;					// Source index, or -1 for all
		int m_TypeFilter = -1;						// AirportType, or -1 for all
		bool m_Only3D = false;
		std::vector<uint32_t> m_InstalledResults;
		std::string m_InstalledKey;
		uint64_t m_InstalledGeneration = ~0ull;
		int m_SelectedInstalled = -1;				// Index into m_InstalledResults
		bool m_SelectAirportTab = false;

		// Gateway tab
		ImGuiTextFilter m_GatewayFilter;
		bool m_GatewayOnlyWithScenery = true;
		bool m_GatewayOnly3D = false;
		std::vector<uint32_t> m_GatewayResults;		// Indices into GatewayClient::GetAirports()
		std::string m_GatewayKey;
		std::shared_ptr<const std::vector<XPlane::GatewayAirport>> m_GatewayList;
		std::string m_GatewaySelectedCode;
		std::shared_ptr<XPlane::GatewayRequest> m_AirportsRequest;
		std::shared_ptr<XPlane::GatewayRequest> m_DetailsRequest;
		std::shared_ptr<const XPlane::GatewayAirportDetails> m_Details;
		std::string m_DetailsError;
		int m_SelectedScenery = 0;					// sceneryId selected in the submissions table

		// New airport popup
		char m_NewId[16] = "";
		char m_NewName[64] = "";
		double m_NewLatitude = 47.0;
		double m_NewLongitude = -122.0;
		int m_NewType = 0;							// 0 land, 1 seaplane, 2 heliport
		int m_NewElevation = 0;
		std::string m_NewPackageRoot;
		char m_NewPackageName[128] = "";
		std::string m_NewError;

		// Install popup (Gateway download target)
		int m_InstallSceneryId = 0;
		std::string m_InstallLabel;
		std::string m_InstallRoot;
		char m_InstallName[128] = "";
		bool m_InstallRegister = true;
	};

}

// -------------------------------------------------------

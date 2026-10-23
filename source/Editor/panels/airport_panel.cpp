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
 * airport_panel.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "airport_panel.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <portable-file-dialogs.h>
#include <Editor/core/editor_airports.h>
#include <Editor/core/editor_scene.h>
#include <Editor/core/editor_theme.h>
#include <Editor/core/editor_ui.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/source/imgui/misc/cpp/imgui_stdlib.h>
#include <SceneryEditorX/xplane/xp_airport_manager.h>
#include <SceneryEditorX/xplane/xp_airport_scene.h>
#include <SceneryEditorX/xplane/xp_gateway_types.h>
#include <SceneryEditorX/xplane/xp_library_manager.h>
#include <SceneryEditorX/xplane/xp_text.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	using namespace XPlane;

	namespace
	{
		constexpr const char *NEW_AIRPORT_POPUP = "New Airport###AirportNew";
		constexpr const char *INSTALL_POPUP = "Install Gateway Scenery###AirportInstall";

		ImVec4 Color(const ImVec4 &palette)
		{
			return EditorTheme::ToStyle(palette);
		}

		const char *TypeIcon(const AirportType type)
		{
			switch (type)
			{
				case AirportType::Seaplane:	return SEDX_ICON_SHIP;
				case AirportType::Heliport:	return SEDX_ICON_MAP_MARKER;
				default:					return SEDX_ICON_PLANE;
			}
		}

		std::string FormatTimestamp(const int64_t unixSeconds)
		{
			if (unixSeconds <= 0)
				return "never";
			const std::time_t time = static_cast<std::time_t>(unixSeconds);
			std::tm local{};
#ifdef _WIN32
			localtime_s(&local, &time);
#else
			localtime_r(&time, &local);
#endif
			char buffer[32];
			std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &local);
			return buffer;
		}

		std::string FormatBytes(const uint64_t bytes)
		{
			char buffer[32];
			if (bytes >= 1024u * 1024u)
				snprintf(buffer, sizeof(buffer), "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
			else
				snprintf(buffer, sizeof(buffer), "%.0f KB", static_cast<double>(bytes) / 1024.0);
			return buffer;
		}

		/* @brief Spinner + text for a request in flight. */
		void DrawRequestStatus(const GatewayRequest &request, const char *what)
		{
			static const char *spinner = "|/-\\";
			const float progress = request.GetProgress();
			ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%c %s %s", spinner[static_cast<int>(ImGui::GetTime() * 8.0) & 3], GetGatewayRequestStateName(request.GetState()), what);
			ImGui::SameLine();
			if (progress >= 0.0f)
				ImGui::ProgressBar(progress, ImVec2(160.0f * ImGui::GetStyle().FontScaleDpi, 0.0f));
			else
				ImGui::TextDisabled("%s", FormatBytes(request.GetBytesReceived()).c_str());
		}

		bool Contains(const std::string &haystack, const std::string &needle)
		{
			return needle.empty() || ToLower(haystack).find(needle) != std::string::npos;
		}
	}

	// -------------------------------------------------------

	AirportPanel::AirportPanel(EditorContext *context) : m_Context(context)
	{
		m_Title = "AIRPORTS";
	}

	void AirportPanel::OnUIRender(bool &isOpen)
	{
		if (EditorUI::BeginPanel("AIRPORTS###Airports", &isOpen))
		{
			DrawStatusBar();

			if (ImGui::BeginTabBar("##AirportTabs"))
			{
				if (ImGui::BeginTabItem("Installed"))
				{
					DrawInstalledTab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Gateway"))
				{
					DrawGatewayTab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Airport", nullptr, m_SelectAirportTab ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
				{
					DrawAirportTab();
					ImGui::EndTabItem();
				}
				m_SelectAirportTab = false;
				ImGui::EndTabBar();
			}

			DrawNewAirportPopup();
			DrawInstallPopup();
		}
		EditorUI::EndPanel();
	}

	// -------------------------------------------------------

	void AirportPanel::DrawStatusBar()
	{
		const bool scanning = AirportManager::GetScanState() == ScanState::Scanning;
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_PLUS, "New Airport...").c_str()))
		{
			m_NewPackageRoot = AirportManager::GetDefaultPackageRoot().string();
			m_NewError.clear();
			ImGui::OpenPopup(NEW_AIRPORT_POPUP);
		}
		EditorUI::ItemTooltip("Create a scenery pack with a new airport");

		ImGui::SameLine();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_FOLDER_OPEN, "Open apt.dat...").c_str()))
			EditorAirports::BrowseForAirport(m_Context);
		EditorUI::ItemTooltip("Open an airport data file from anywhere");

		ImGui::SameLine();
		ImGui::BeginDisabled(scanning);
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_REFRESH, "Rescan").c_str()))
			AirportManager::Rescan();
		ImGui::EndDisabled();
		EditorUI::ItemTooltip("Index the apt.dat files of the X-Plane install again");

		ImGui::SameLine();
		if (scanning)
		{
			static const char *spinner = "|/-\\";
			ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%c Scanning airports...", spinner[static_cast<int>(ImGui::GetTime() * 8.0) & 3]);
		}
		else
		{
			ImGui::TextDisabled("%s", AirportManager::GetScanMessage().c_str());
			if (AirportManager::GetLastScanMilliseconds() > 0.0)
				EditorUI::ItemTooltip(("Scanned in " + std::to_string(static_cast<int>(AirportManager::GetLastScanMilliseconds())) + " ms").c_str());
		}
	}

	// -------------------------------------------------------

	void AirportPanel::RefreshInstalledResults(const AirportIndex &index)
	{
		const std::string key = std::string(m_InstalledFilter.InputBuf) + '|' + std::to_string(m_SourceFilter) + '|' + std::to_string(m_TypeFilter) + '|' + std::to_string(m_Only3D);
		const uint64_t generation = AirportManager::GetGeneration();
		if (key == m_InstalledKey && generation == m_InstalledGeneration)
			return;
		m_InstalledKey = key;
		m_InstalledGeneration = generation;
		m_SelectedInstalled = -1;

		AirportIndex::SearchOptions options;
		options.maxResults = 1000;
		options.sourceFilter = m_SourceFilter;
		options.only3D = m_Only3D;
		if (m_TypeFilter >= 0)
			options.type = static_cast<AirportType>(m_TypeFilter == 0 ? 1 : (m_TypeFilter == 1 ? 16 : 17));
		index.Search(m_InstalledFilter.InputBuf, options, m_InstalledResults);
	}

	void AirportPanel::DrawInstalledTab()
	{
		const std::shared_ptr<const AirportIndex> indexPtr = AirportManager::GetIndex();
		const AirportIndex &index = *indexPtr;
		const std::vector<AirportSource> &sources = index.GetSources();
		if (m_SourceFilter >= static_cast<int>(sources.size()))
			m_SourceFilter = -1;

		EditorUI::SearchFilter("##AirportSearch", m_InstalledFilter, ImGui::GetContentRegionAvail().x * 0.35f);
		EditorUI::ItemTooltip("Code (ICAO, IATA, FAA, local), name, city or country; several words narrow it down");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(170.0f * ImGui::GetStyle().FontScaleDpi);
		const std::string sourceLabel = m_SourceFilter < 0 ? "All scenery packs" : sources[static_cast<size_t>(m_SourceFilter)].packageName;
		if (ImGui::BeginCombo("##SourceFilter", sourceLabel.c_str()))
		{
			if (ImGui::Selectable("All scenery packs", m_SourceFilter < 0))
				m_SourceFilter = -1;
			for (int i = 0; i < static_cast<int>(sources.size()); ++i)
			{
				const AirportSource &source = sources[static_cast<size_t>(i)];
				const std::string label = source.packageName + "  (" + std::to_string(source.airportCount) + ")";
				if (ImGui::Selectable(label.c_str(), m_SourceFilter == i))
					m_SourceFilter = i;
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s%s", source.aptDat.string().c_str(), source.enabledInSim ? "" : "\nDisabled in scenery_packs.ini");
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120.0f * ImGui::GetStyle().FontScaleDpi);
		const char *typeLabels[] = { "All types", "Airports", "Seaplane bases", "Heliports" };
		int typeIndex = m_TypeFilter + 1;
		if (ImGui::Combo("##TypeFilter", &typeIndex, typeLabels, 4))
			m_TypeFilter = typeIndex - 1;
		ImGui::SameLine();
		ImGui::Checkbox("3D only", &m_Only3D);
		EditorUI::ItemTooltip("Airports with 3D scenery (gui_label 3D)");

		RefreshInstalledResults(index);

		const std::vector<AirportEntry> &airports = index.GetAirports();
		ImGui::TextDisabled("%zu of %zu airports", m_InstalledResults.size(), airports.size());
		if (m_InstalledResults.size() >= 1000)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("(first 1000; type more to narrow the search)");
		}

		const float detailsHeight = m_SelectedInstalled >= 0 ? 110.0f * ImGui::GetStyle().FontScaleDpi : 0.0f;
		constexpr ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;
		if (ImGui::BeginTable("##InstalledAirports", 6, flags, ImVec2(0.0f, ImGui::GetContentRegionAvail().y - detailsHeight)))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 70.0f * ImGui::GetStyle().FontScaleDpi);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 3.0f);
			ImGui::TableSetupColumn("3D", ImGuiTableColumnFlags_WidthFixed, 28.0f * ImGui::GetStyle().FontScaleDpi);
			ImGui::TableSetupColumn("Rwy", ImGuiTableColumnFlags_WidthFixed, 36.0f * ImGui::GetStyle().FontScaleDpi);
			ImGui::TableSetupColumn("Location", ImGuiTableColumnFlags_WidthStretch, 2.0f);
			ImGui::TableSetupColumn("Scenery pack", ImGuiTableColumnFlags_WidthStretch, 2.0f);
			ImGui::TableHeadersRow();

			ImGuiListClipper clipper;
			clipper.Begin(static_cast<int>(m_InstalledResults.size()));
			while (clipper.Step())
			{
				for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
				{
					const AirportEntry &entry = airports[m_InstalledResults[static_cast<size_t>(row)]];
					const AirportSource &source = sources[entry.source];
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::PushID(row);
					const bool selected = m_SelectedInstalled == row;
					if (ImGui::Selectable(EditorUI::IconLabel(TypeIcon(entry.type), entry.id.c_str()).c_str(), selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
					{
						m_SelectedInstalled = row;
						if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
						{
							if (EditorAirports::OpenAirport(entry, m_Context))
								m_SelectAirportTab = true;
						}
					}
					if (ImGui::BeginPopupContextItem("##InstalledMenu"))
					{
						m_SelectedInstalled = row;
						if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_FOLDER_OPEN, "Open").c_str()))
						{
							if (EditorAirports::OpenAirport(entry, m_Context))
								m_SelectAirportTab = true;
						}
						if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_EXTERNAL_LINK, "Show on the Gateway website").c_str()))
							EditorAirports::OpenGatewayPage(entry.icao.empty() ? entry.id : entry.icao);
						if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_CLOUD_DOWNLOAD, "Look up on the Gateway").c_str()))
						{
							SelectGatewayAirport(entry.icao.empty() ? entry.id : entry.icao);
						}
						ImGui::EndPopup();
					}
					ImGui::PopID();

					ImGui::TableNextColumn();
					ImGui::TextUnformatted(entry.name.c_str());
					ImGui::TableNextColumn();
					if (entry.is3D)
						ImGui::TextColored(Color(EditorTheme::Palette::Accent), "3D");
					ImGui::TableNextColumn();
					const int runways = entry.runwayCount + entry.waterRunwayCount + entry.helipadCount;
					if (runways > 0)
						ImGui::Text("%d", runways);
					ImGui::TableNextColumn();
					std::string location = entry.city;
					if (!entry.country.empty())
						location += (location.empty() ? "" : ", ") + entry.country;
					ImGui::TextUnformatted(location.c_str());
					ImGui::TableNextColumn();
					if (!source.enabledInSim)
						ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
					ImGui::TextUnformatted(source.packageName.c_str());
					if (!source.enabledInSim)
						ImGui::PopStyleColor();
				}
			}
			ImGui::EndTable();
		}

		if (m_SelectedInstalled >= 0 && m_SelectedInstalled < static_cast<int>(m_InstalledResults.size()))
		{
			const AirportEntry &entry = airports[m_InstalledResults[static_cast<size_t>(m_SelectedInstalled)]];
			const AirportSource &source = sources[entry.source];
			ImGui::Separator();
			ImGui::TextUnformatted(EditorUI::IconLabel(TypeIcon(entry.type), entry.GetLabel().c_str()).c_str());
			const std::string codes = entry.GetCodes();
			if (!codes.empty())
			{
				ImGui::SameLine();
				ImGui::TextDisabled("%s", codes.c_str());
			}
			if (entry.hasPosition)
				ImGui::TextDisabled("%.5f, %.5f  %s  %s", entry.latitude, entry.longitude, GetAirportTypeName(entry.type), entry.is3D ? "3D" : "2D");
			ImGui::TextDisabled("%s, line %u", source.aptDat.string().c_str(), entry.line);
			EditorUI::ItemTooltip(source.aptDat.string().c_str());

			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_FOLDER_OPEN, "Open").c_str()))
			{
				if (EditorAirports::OpenAirport(entry, m_Context))
					m_SelectAirportTab = true;
			}
			EditorUI::ItemTooltip(source.source == PackageSource::GlobalScenery ? "Global Airports is read-only: Save As puts your edits in a pack of your own" : "Load the airport into the scene");
			ImGui::SameLine();
			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_CLOUD_DOWNLOAD, "Gateway").c_str()))
				SelectGatewayAirport(entry.icao.empty() ? entry.id : entry.icao);
			EditorUI::ItemTooltip("Look the airport up on the Scenery Gateway");
			ImGui::SameLine();
			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_EXTERNAL_LINK, "Website").c_str()))
				EditorAirports::OpenGatewayPage(entry.icao.empty() ? entry.id : entry.icao);
		}
	}

	// -------------------------------------------------------

	void AirportPanel::RefreshGatewayResults()
	{
		const std::shared_ptr<const std::vector<GatewayAirport>> list = GatewayClient::GetAirports();
		const std::string key = std::string(m_GatewayFilter.InputBuf) + '|' + std::to_string(m_GatewayOnlyWithScenery) + '|' + std::to_string(m_GatewayOnly3D);
		if (list == m_GatewayList && key == m_GatewayKey)
			return;
		m_GatewayList = list;
		m_GatewayKey = key;
		m_GatewayResults.clear();
		if (!list)
			return;

		// Every word must match the code or the name; exact code matches come first.
		std::vector<std::string> words;
		{
			std::vector<std::string_view> tokens;
			Tokenize(m_GatewayFilter.InputBuf, tokens);
			for (const std::string_view token : tokens)
				words.push_back(ToLower(token));
		}

		std::vector<std::pair<int, uint32_t>> hits;
		for (uint32_t i = 0; i < list->size(); ++i)
		{
			const GatewayAirport &airport = (*list)[i];
			if (m_GatewayOnlyWithScenery && !airport.HasScenery())
				continue;
			if (m_GatewayOnly3D && airport.sceneryType < 2)
				continue;
			int rank = 2;
			bool matches = true;
			const std::string code = ToLower(airport.code);
			for (const std::string &word : words)
			{
				if (code == word)
					rank = std::min(rank, 0);
				else if (code.starts_with(word))
					rank = std::min(rank, 1);
				else if (!Contains(airport.name, word))
				{
					matches = false;
					break;
				}
			}
			if (matches)
				hits.emplace_back(words.empty() ? 2 : rank, i);
			if (hits.size() > 20000)
				break;
		}
		std::ranges::stable_sort(hits, [list](const auto &a, const auto &b)
		{
			if (a.first != b.first)
				return a.first < b.first;
			return (*list)[a.second].code < (*list)[b.second].code;
		});
		for (size_t i = 0; i < hits.size() && i < 1000; ++i)
			m_GatewayResults.push_back(hits[i].second);
	}

	void AirportPanel::SelectGatewayAirport(const std::string &code)
	{
		if (code.empty())
			return;
		m_GatewaySelectedCode = code;
		m_Details.reset();
		m_DetailsError.clear();
		m_SelectedScenery = 0;
		m_DetailsRequest = GatewayClient::FetchAirport(code);
		snprintf(m_GatewayFilter.InputBuf, sizeof(m_GatewayFilter.InputBuf), "%s", code.c_str());
		m_GatewayFilter.Build();
	}

	void AirportPanel::DrawGatewayTab()
	{
		// Airport list status
		const std::shared_ptr<const std::vector<GatewayAirport>> list = GatewayClient::GetAirports();
		if (m_AirportsRequest && m_AirportsRequest->IsFinished())
		{
			if (!m_AirportsRequest->Succeeded())
				EDITOR_ERROR_TAG("Gateway", "Airport list: {}", m_AirportsRequest->GetError());
			m_AirportsRequest.reset();
		}

		if (m_AirportsRequest)
			DrawRequestStatus(*m_AirportsRequest, "the airport list");
		else if (list)
			ImGui::TextDisabled("%zu Gateway airports, list from %s", list->size(), FormatTimestamp(GatewayClient::GetAirportsTimestamp()).c_str());
		else
			ImGui::TextDisabled("The Gateway airport list hasn't been downloaded yet (about 20 MB)");
		ImGui::SameLine();
		ImGui::BeginDisabled(m_AirportsRequest != nullptr);
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_CLOUD_DOWNLOAD, list ? "Refresh list" : "Download list").c_str()))
			m_AirportsRequest = GatewayClient::FetchAirports(true);
		ImGui::EndDisabled();
		EditorUI::ItemTooltip("Fetch https://gateway.x-plane.com/apiv1/airports (kept for a day)");

		// Search
		EditorUI::SearchFilter("##GatewaySearch", m_GatewayFilter, ImGui::GetContentRegionAvail().x * 0.4f);
		EditorUI::ItemTooltip("Airport code or name");
		ImGui::SameLine();
		ImGui::Checkbox("Has scenery", &m_GatewayOnlyWithScenery);
		ImGui::SameLine();
		ImGui::Checkbox("3D", &m_GatewayOnly3D);
		EditorUI::ItemTooltip("Airports with 3D submissions");
		RefreshGatewayResults();

		const float lowerHeight = (m_GatewaySelectedCode.empty() ? 0.0f : 230.0f) * ImGui::GetStyle().FontScaleDpi;
		constexpr ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;
		if (ImGui::BeginTable("##GatewayAirports", 5, flags, ImVec2(0.0f, std::max(80.0f, ImGui::GetContentRegionAvail().y - lowerHeight))))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Code", ImGuiTableColumnFlags_WidthFixed, 80.0f * ImGui::GetStyle().FontScaleDpi);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 3.0f);
			ImGui::TableSetupColumn("Approved", ImGuiTableColumnFlags_WidthFixed, 64.0f * ImGui::GetStyle().FontScaleDpi);
			ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch, 1.5f);
			ImGui::TableSetupColumn("Position", ImGuiTableColumnFlags_WidthStretch, 1.5f);
			ImGui::TableHeadersRow();

			if (list)
			{
				ImGuiListClipper clipper;
				clipper.Begin(static_cast<int>(m_GatewayResults.size()));
				while (clipper.Step())
				{
					for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
					{
						const GatewayAirport &airport = (*list)[m_GatewayResults[static_cast<size_t>(row)]];
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::PushID(row);
						if (ImGui::Selectable(airport.code.c_str(), airport.code == m_GatewaySelectedCode, ImGuiSelectableFlags_SpanAllColumns))
							SelectGatewayAirport(airport.code);
						ImGui::PopID();
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(airport.name.c_str());
						if (airport.deprecated)
						{
							ImGui::SameLine();
							ImGui::TextDisabled("(deprecated%s%s)", airport.deprecatedInFavorOf.empty() ? "" : ", use ", airport.deprecatedInFavorOf.c_str());
						}
						ImGui::TableNextColumn();
						if (airport.approvedSceneryCount > 0)
							ImGui::Text("%d", airport.approvedSceneryCount);
						if (airport.recommendedSceneryId != 0)
						{
							ImGui::SameLine();
							ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%s", SEDX_ICON_STAR);
							if (ImGui::IsItemHovered())
								ImGui::SetTooltip("Recommended submission %d (what X-Plane ships)", airport.recommendedSceneryId);
						}
						ImGui::TableNextColumn();
						if (!airport.checkedOutBy.empty())
						{
							ImGui::TextDisabled("Locked by %s", airport.checkedOutBy.c_str());
						}
						else
							ImGui::TextUnformatted(airport.status.c_str());
						ImGui::TableNextColumn();
						ImGui::TextDisabled("%.3f, %.3f", airport.latitude, airport.longitude);
					}
				}
			}
			ImGui::EndTable();
		}

		if (m_GatewaySelectedCode.empty())
			return;

		// Selected airport: its submissions
		ImGui::Separator();
		if (m_DetailsRequest && m_DetailsRequest->IsFinished())
		{
			if (m_DetailsRequest->Succeeded())
				m_Details = m_DetailsRequest->airport;
			else
				m_DetailsError = m_DetailsRequest->GetError();
			m_DetailsRequest.reset();
		}

		if (m_DetailsRequest)
		{
			DrawRequestStatus(*m_DetailsRequest, m_GatewaySelectedCode.c_str());
			return;
		}
		if (!m_DetailsError.empty())
		{
			ImGui::TextColored(Color(EditorTheme::Palette::Error), "%s: %s", m_GatewaySelectedCode.c_str(), m_DetailsError.c_str());
			ImGui::SameLine();
			if (ImGui::SmallButton("Retry"))
				SelectGatewayAirport(m_GatewaySelectedCode);
			return;
		}
		if (!m_Details)
			return;

		const GatewayAirportDetails &details = *m_Details;
		ImGui::TextUnformatted(EditorUI::IconLabel(SEDX_ICON_GLOBE, (details.airport.code + "  " + details.airport.name).c_str()).c_str());
		ImGui::SameLine();
		ImGui::TextDisabled("%zu submissions, %d approved", details.scenery.size(), details.airport.approvedSceneryCount);
		ImGui::SameLine();
		if (ImGui::SmallButton(EditorUI::IconLabel(SEDX_ICON_EXTERNAL_LINK, "Website").c_str()))
			EditorAirports::OpenGatewayPage(details.airport.code);

		const GatewayScenery *recommended = details.GetRecommended();
		const EditorAirports::PendingInstall *pending = EditorAirports::GetPendingInstall();
		const bool installing = pending && pending->request && !pending->request->IsFinished();

		ImGui::BeginDisabled(installing || !recommended);
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_DOWNLOAD, "Install recommended...").c_str()) && recommended)
		{
			m_InstallSceneryId = recommended->sceneryId;
			m_InstallLabel = details.airport.code + " " + details.airport.name + " (submission " + std::to_string(recommended->sceneryId) + " by " + recommended->userName + ")";
			m_InstallRoot = AirportManager::GetDefaultPackageRoot().string();
			snprintf(m_InstallName, sizeof(m_InstallName), "%s", MakeGatewayPackageName(details.airport.code, details.airport.name).c_str());
			ImGui::OpenPopup(INSTALL_POPUP);
		}
		ImGui::EndDisabled();
		EditorUI::ItemTooltip(recommended ? "Download the submission X-Plane ships and open it as a scenery pack" : "This airport has no recommended submission");
		ImGui::SameLine();
		ImGui::BeginDisabled(installing || m_SelectedScenery == 0);
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_DOWNLOAD, "Install selected...").c_str()))
		{
			m_InstallSceneryId = m_SelectedScenery;
			m_InstallLabel = details.airport.code + " " + details.airport.name + " (submission " + std::to_string(m_SelectedScenery) + ")";
			m_InstallRoot = AirportManager::GetDefaultPackageRoot().string();
			snprintf(m_InstallName, sizeof(m_InstallName), "%s", MakeGatewayPackageName(details.airport.code, details.airport.name).c_str());
			ImGui::OpenPopup(INSTALL_POPUP);
		}
		ImGui::EndDisabled();

		if (pending && pending->request)
		{
			ImGui::SameLine();
			if (!pending->request->IsFinished())
				DrawRequestStatus(*pending->request, pending->label.c_str());
			else if (!pending->error.empty())
				ImGui::TextColored(Color(EditorTheme::Palette::Error), "%s", pending->error.c_str());
			else if (!pending->packageFolder.empty())
				ImGui::TextDisabled("Installed to %s", pending->packageFolder.filename().string().c_str());
		}

		constexpr ImGuiTableFlags sceneryFlags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp;
		if (ImGui::BeginTable("##GatewayScenery", 6, sceneryFlags, ImVec2(0.0f, ImGui::GetContentRegionAvail().y)))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 70.0f * ImGui::GetStyle().FontScaleDpi);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 36.0f * ImGui::GetStyle().FontScaleDpi);
			ImGui::TableSetupColumn("Artist", ImGuiTableColumnFlags_WidthStretch, 1.5f);
			ImGui::TableSetupColumn("Uploaded", ImGuiTableColumnFlags_WidthFixed, 84.0f * ImGui::GetStyle().FontScaleDpi);
			ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 84.0f * ImGui::GetStyle().FontScaleDpi);
			ImGui::TableSetupColumn("Comments", ImGuiTableColumnFlags_WidthStretch, 3.0f);
			ImGui::TableHeadersRow();

			// Newest first
			for (size_t n = details.scenery.size(); n-- > 0;)
			{
				const GatewayScenery &scenery = details.scenery[n];
				const bool isRecommended = recommended && recommended->sceneryId == scenery.sceneryId;
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::PushID(scenery.sceneryId);
				const std::string label = (isRecommended ? std::string(SEDX_ICON_STAR) + " " : std::string()) + std::to_string(scenery.sceneryId);
				if (ImGui::Selectable(label.c_str(), m_SelectedScenery == scenery.sceneryId, ImGuiSelectableFlags_SpanAllColumns))
					m_SelectedScenery = scenery.sceneryId;
				ImGui::PopID();
				ImGui::TableNextColumn();
				if (scenery.Is3D())
					ImGui::TextColored(Color(EditorTheme::Palette::Accent), "3D");
				else
					ImGui::TextUnformatted(scenery.type.c_str());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(scenery.userName.c_str());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(GatewayScenery::ShortDate(scenery.dateUploaded).c_str());
				ImGui::TableNextColumn();
				if (scenery.IsDeclined())
					ImGui::TextColored(Color(EditorTheme::Palette::Error), "%s", scenery.status.c_str());
				else if (scenery.IsApproved())
					ImGui::TextColored(Color(EditorTheme::Palette::Accent), "%s", scenery.status.c_str());
				else
					ImGui::TextUnformatted(scenery.status.c_str());
				ImGui::TableNextColumn();
				std::string comments = scenery.artistComments;
				if (!scenery.moderatorComments.empty())
					comments += (comments.empty() ? "" : "  |  Moderator: ") + scenery.moderatorComments;
				ImGui::TextUnformatted(comments.c_str());
				if (!comments.empty() && ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", comments.c_str());
			}
			ImGui::EndTable();
		}
	}

	// -------------------------------------------------------

	void AirportPanel::DrawAirportTab()
	{
		AirportDocument *document = EditorAirports::GetDocument();
		if (!document)
		{
			EditorUI::EmptyState("No airport open.\nDouble-click one under Installed, pull one from the Gateway, or create a new one.");
			return;
		}
		AptAirport &airport = document->airport;

		// Title row
		ImGui::TextUnformatted(EditorUI::IconLabel(TypeIcon(airport.type), document->GetTitle().c_str()).c_str());
		ImGui::SameLine();
		if (document->IsReadOnly())
		{
			ImGui::TextColored(Color(EditorTheme::Palette::Warning), "read-only");
			EditorUI::ItemTooltip("This file belongs to X-Plane. Save As writes your edits into a scenery pack of your own.");
			ImGui::SameLine();
		}
		ImGui::TextDisabled("%s", document->GetPackageName().c_str());
		if (document->HasFile())
			EditorUI::ItemTooltip(document->GetAptDatPath().string().c_str());

		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_FLOPPY_O, "Save").c_str()))
			EditorAirports::SaveAirport();
		EditorUI::ItemTooltip(document->IsReadOnly() ? "Read-only source: asks for a scenery pack folder" : "Write the pack's apt.dat");
		ImGui::SameLine();
		if (ImGui::Button("Save As..."))
			EditorAirports::SaveAirportAs();
		EditorUI::ItemTooltip("Write the airport into another scenery pack");
		ImGui::SameLine();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_TIMES, "Close").c_str()))
			EditorAirports::CloseAirport(m_Context);
		EditorUI::ItemTooltip("Unload the airport from the scene");
		ImGui::SameLine();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_CROSSHAIRS, "Frame").c_str()))
			EditorAirports::FrameAirport();
		ImGui::SameLine();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_FOLDER, "Reveal").c_str()))
			EditorAirports::RevealPackage();
		EditorUI::ItemTooltip("Show the scenery pack folder");

		if (ImGui::BeginChild("##AirportDocument"))
		{
			// Header fields
			if (EditorUI::SectionHeader("AIRPORT"))
			{
				if (EditorUI::BeginPropertyGrid("##AirportHeader"))
				{
					EditorUI::PropertyLabel("ID");
					ImGui::InputText("##Id", &airport.id, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank);
					if (ImGui::IsItemDeactivatedAfterEdit())
						EditorAirports::MarkEdited(false);
					EditorUI::ItemTooltip("Up to 7 characters, unique across X-Plane (the ICAO code when there is one)");

					EditorUI::PropertyLabel("Name");
					ImGui::InputText("##Name", &airport.name);
					if (ImGui::IsItemDeactivatedAfterEdit())
						EditorAirports::MarkEdited(false);

					EditorUI::PropertyLabel("Type");
					int type = airport.type == AirportType::Land ? 0 : (airport.type == AirportType::Seaplane ? 1 : 2);
					const char *types[] = { "Airport", "Seaplane base", "Heliport" };
					if (ImGui::Combo("##Type", &type, types, 3))
					{
						airport.type = type == 0 ? AirportType::Land : (type == 1 ? AirportType::Seaplane : AirportType::Heliport);
						EditorAirports::MarkEdited(false);
					}

					EditorUI::PropertyLabel("Elevation");
					ImGui::InputInt("##Elevation", &airport.elevationFt);
					if (ImGui::IsItemDeactivatedAfterEdit())
						EditorAirports::MarkEdited(false);
					EditorUI::ItemTooltip("Feet above sea level (X-Plane 12 takes it from the terrain; kept for older versions)");

					double lat = 0.0, lon = 0.0;
					if (airport.GetDatum(lat, lon))
					{
						EditorUI::PropertyLabel("Datum");
						ImGui::Text("%.6f, %.6f", lat, lon);
						EditorUI::ItemTooltip("datum_lat / datum_lon metadata, or the center of the runways");
					}

					EditorUI::PropertyLabel("Contents");
					const SceneAirports::Stats stats = SceneAirports::GetStats();
					ImGui::Text("%zu elements, %u triangles", airport.GetElementCount(), stats.triangles);
					EditorUI::EndPropertyGrid();
				}
			}

			// Metadata (1302 rows)
			if (EditorUI::SectionHeader("METADATA", false))
			{
				if (ImGui::BeginTable("##Metadata", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
				{
					ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch, 1.0f);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 2.0f);
					ImGui::TableSetupColumn("##Remove", ImGuiTableColumnFlags_WidthFixed, 24.0f * ImGui::GetStyle().FontScaleDpi);
					int remove = -1;
					for (int i = 0; i < static_cast<int>(airport.metadata.size()); ++i)
					{
						AptMetadata &entry = airport.metadata[static_cast<size_t>(i)];
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::PushID(i);
						ImGui::SetNextItemWidth(-FLT_MIN);
						ImGui::InputText("##Key", &entry.key);
						if (ImGui::IsItemDeactivatedAfterEdit())
							EditorAirports::MarkEdited(entry.key.starts_with("datum"));
						ImGui::TableNextColumn();
						ImGui::SetNextItemWidth(-FLT_MIN);
						ImGui::InputText("##Value", &entry.value);
						if (ImGui::IsItemDeactivatedAfterEdit())
							EditorAirports::MarkEdited(entry.key.starts_with("datum"));
						ImGui::TableNextColumn();
						if (ImGui::SmallButton(SEDX_ICON_TRASH))
							remove = i;
						ImGui::PopID();
					}
					ImGui::EndTable();
					if (remove >= 0)
					{
						airport.metadata.erase(airport.metadata.begin() + remove);
						EditorAirports::MarkEdited(false);
					}
				}
				if (ImGui::SmallButton(EditorUI::IconLabel(SEDX_ICON_PLUS, "Add").c_str()))
					ImGui::OpenPopup("##AddMetadata");
				if (ImGui::BeginPopup("##AddMetadata"))
				{
					for (const char *key : { "icao_code", "iata_code", "faa_code", "local_code", "city", "state", "country", "region_code", "datum_lat", "datum_lon",
						"transition_alt", "transition_level", "gui_label", "flatten", "drive_on_left", "local_authority", "tower_service_type", "allows_circuits", "is_oilrig" })
					{
						if (!airport.HasMetadata(key) && ImGui::MenuItem(key))
						{
							airport.SetMetadata(key, "");
							EditorAirports::MarkEdited(false);
						}
					}
					ImGui::EndPopup();
				}
			}

			// Contents by kind, with visibility toggles
			if (EditorUI::SectionHeader("CONTENTS"))
			{
				struct Row
				{
					const char *label;
					size_t count;
					AirportElementKind kind;
				};
				const Row rows[] =
				{
					{ "Runways", airport.runways.size(), AirportElementKind::Runway },
					{ "Water runways", airport.waterRunways.size(), AirportElementKind::WaterRunway },
					{ "Helipads", airport.helipads.size(), AirportElementKind::Helipad },
					{ "Pavement", airport.pavements.size(), AirportElementKind::Pavement },
					{ "Lines and lights", airport.linearFeatures.size(), AirportElementKind::LinearFeature },
					{ "Boundary", airport.boundaries.size(), AirportElementKind::Boundary },
					{ "Signs", airport.signs.size(), AirportElementKind::Sign },
					{ "Approach lights", airport.lightFixtures.size(), AirportElementKind::LightFixture },
					{ "Startup locations", airport.startups.size() + airport.legacyStartups.size(), AirportElementKind::Startup },
					{ "Windsocks", airport.windsocks.size(), AirportElementKind::Windsock },
					{ "Taxi routes", airport.taxiNetwork ? airport.taxiNetwork->edges.size() : 0, AirportElementKind::TaxiRoutes },
					{ "Taxi nodes", airport.taxiNetwork ? airport.taxiNetwork->nodes.size() : 0, AirportElementKind::TaxiNodes },
					{ "Ground service", airport.truckParkings.size() + airport.truckDestinations.size(), AirportElementKind::TruckParking },
					{ "Jetways", airport.jetways.size(), AirportElementKind::Jetway },
				};
				if (ImGui::BeginTable("##Contents", 3, ImGuiTableFlags_SizingStretchProp))
				{
					ImGui::TableSetupColumn("##Visible", ImGuiTableColumnFlags_WidthFixed, 24.0f * ImGui::GetStyle().FontScaleDpi);
					ImGui::TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthStretch, 2.0f);
					ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthStretch, 1.0f);
					for (const Row &row : rows)
					{
						if (row.count == 0)
							continue;
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::PushID(row.label);
						bool visible = SceneAirports::IsKindVisible(row.kind);
						if (ImGui::Checkbox("##Visible", &visible))
						{
							SceneAirports::SetKindVisible(row.kind, visible);
							if (row.kind == AirportElementKind::Startup)
								SceneAirports::SetKindVisible(AirportElementKind::LegacyStartup, visible);
							if (row.kind == AirportElementKind::TruckParking)
								SceneAirports::SetKindVisible(AirportElementKind::TruckDestination, visible);
						}
						EditorUI::ItemTooltip("Show in the viewport");
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(row.label);
						ImGui::TableNextColumn();
						ImGui::Text("%zu", row.count);
						ImGui::PopID();
					}
					ImGui::EndTable();
				}

				// Frequencies and flows aren't drawn; list them here
				if (!airport.frequencies.empty() && ImGui::TreeNode("Frequencies", "Frequencies (%zu)", airport.frequencies.size()))
				{
					int remove = -1;
					for (int i = 0; i < static_cast<int>(airport.frequencies.size()); ++i)
					{
						AptFrequency &frequency = airport.frequencies[static_cast<size_t>(i)];
						ImGui::PushID(i);
						ImGui::SetNextItemWidth(130.0f * ImGui::GetStyle().FontScaleDpi);
						int type = static_cast<int>(frequency.type);
						const char *typeNames[] = { "ATIS / AWOS", "Unicom / CTAF", "Clearance", "Ground", "Tower", "Approach", "Departure" };
						if (ImGui::Combo("##FreqType", &type, typeNames, 7))
						{
							frequency.type = static_cast<AptFrequency::Type>(type);
							EditorAirports::MarkEdited(false);
						}
						ImGui::SameLine();
						ImGui::SetNextItemWidth(90.0f * ImGui::GetStyle().FontScaleDpi);
						double mhz = frequency.frequencyKhz / 1000.0;
						ImGui::InputDouble("##Freq", &mhz, 0.0, 0.0, "%.3f");
						if (ImGui::IsItemDeactivatedAfterEdit())
						{
							frequency.frequencyKhz = static_cast<int>(std::lround(mhz * 1000.0));
							frequency.legacy = false;
							EditorAirports::MarkEdited(false);
						}
						ImGui::SameLine();
						ImGui::SetNextItemWidth(-60.0f * ImGui::GetStyle().FontScaleDpi);
						ImGui::InputText("##FreqName", &frequency.name);
						if (ImGui::IsItemDeactivatedAfterEdit())
							EditorAirports::MarkEdited(false);
						ImGui::SameLine();
						if (ImGui::SmallButton(SEDX_ICON_TRASH))
							remove = i;
						ImGui::PopID();
					}
					if (remove >= 0)
					{
						airport.frequencies.erase(airport.frequencies.begin() + remove);
						EditorAirports::MarkEdited(false);
					}
					ImGui::TreePop();
				}
				if (ImGui::SmallButton(EditorUI::IconLabel(SEDX_ICON_PLUS, "Add frequency").c_str()))
				{
					AptFrequency frequency;
					frequency.type = AptFrequency::Type::Tower;
					frequency.frequencyKhz = 118000;
					frequency.name = airport.id + " TWR";
					airport.frequencies.push_back(frequency);
					EditorAirports::MarkEdited(false);
				}

				if (!airport.flows.empty() && ImGui::TreeNode("Flows", "Traffic flows (%zu)", airport.flows.size()))
				{
					for (const AptFlow &flow : airport.flows)
					{
						ImGui::BulletText("%s: %zu wind, %zu time rules, %zu runway uses", flow.name.c_str(), flow.windRules.size(), flow.timeRules.size(), flow.runwayUses.size());
					}
					ImGui::TreePop();
				}
				if (!airport.unknownRows.empty())
					ImGui::TextDisabled("%zu rows with unknown codes are kept as they are", airport.unknownRows.size());
			}

			// Validation
			const std::vector<std::string> &problems = EditorAirports::GetValidationProblems();
			if (EditorUI::SectionHeader(problems.empty() ? "VALIDATION" : "VALIDATION (PROBLEMS)"))
			{
				if (problems.empty())
				{
					ImGui::TextColored(Color(EditorTheme::Palette::Accent), "%s No problems found", SEDX_ICON_CHECK_CIRCLE);
				}
				else
				{
					for (const std::string &problem : problems)
					{
						ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%s", SEDX_ICON_EXCLAMATION_TRIANGLE);
						ImGui::SameLine();
						ImGui::TextWrapped("%s", problem.c_str());
					}
				}
			}
		}
		ImGui::EndChild();
	}

	// -------------------------------------------------------

	void AirportPanel::DrawNewAirportPopup()
	{
		const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(520.0f * ImGui::GetStyle().FontScaleDpi, 0.0f), ImGuiCond_Appearing);
		if (!ImGui::BeginPopupModal(NEW_AIRPORT_POPUP, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			return;

		if (EditorUI::BeginPropertyGrid("##NewAirport", 140.0f))
		{
			EditorUI::PropertyLabel("Airport ID");
			ImGui::InputText("##NewId", m_NewId, sizeof(m_NewId), ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank);
			EditorUI::ItemTooltip("Up to 7 characters: the ICAO code, or the Gateway's code for airports without one");

			EditorUI::PropertyLabel("Name");
			ImGui::InputText("##NewName", m_NewName, sizeof(m_NewName));

			EditorUI::PropertyLabel("Type");
			const char *types[] = { "Airport", "Seaplane base", "Heliport" };
			ImGui::Combo("##NewType", &m_NewType, types, 3);

			EditorUI::PropertyLabel("Latitude");
			ImGui::InputDouble("##NewLat", &m_NewLatitude, 0.0, 0.0, "%.6f");
			EditorUI::PropertyLabel("Longitude");
			ImGui::InputDouble("##NewLon", &m_NewLongitude, 0.0, 0.0, "%.6f");
			EditorUI::ItemTooltip("Where the airport datum (and the first runway) goes; WGS84 degrees");

			EditorUI::PropertyLabel("Elevation (ft)");
			ImGui::InputInt("##NewElevation", &m_NewElevation);

			EditorUI::PropertyLabel("Create in");
			ImGui::InputText("##NewRoot", &m_NewPackageRoot);
			EditorUI::ItemTooltip("Folder the scenery pack is created in (Custom Scenery by default)");
			ImGui::SameLine();
			if (ImGui::Button("..."))
			{
				const std::string folder = pfd::select_folder("Folder for the new scenery pack", m_NewPackageRoot).result();
				if (!folder.empty())
					m_NewPackageRoot = folder;
			}

			EditorUI::PropertyLabel("Pack folder name");
			ImGui::InputTextWithHint("##NewPackage", MakeGatewayPackageName(m_NewId, m_NewName).c_str(), m_NewPackageName, sizeof(m_NewPackageName));
			EditorUI::EndPropertyGrid();
		}

		if (!m_NewError.empty())
			ImGui::TextColored(Color(EditorTheme::Palette::Error), "%s", m_NewError.c_str());

		const bool valid = m_NewId[0] != '\0' && std::strlen(m_NewId) <= 7 && !m_NewPackageRoot.empty();
		ImGui::BeginDisabled(!valid);
		if (ImGui::Button("Create", ImVec2(120.0f, 0.0f)))
		{
			const AirportType type = m_NewType == 0 ? AirportType::Land : (m_NewType == 1 ? AirportType::Seaplane : AirportType::Heliport);
			if (EditorAirports::CreateAirport(m_NewId, m_NewName, m_NewLatitude, m_NewLongitude, type, m_NewElevation, m_NewPackageRoot, m_NewPackageName, m_Context))
			{
				m_SelectAirportTab = true;
				ImGui::CloseCurrentPopup();
			}
			else
				m_NewError = "Could not create the airport (see the console)";
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	void AirportPanel::DrawInstallPopup()
	{
		const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(560.0f * ImGui::GetStyle().FontScaleDpi, 0.0f), ImGuiCond_Appearing);
		if (!ImGui::BeginPopupModal(INSTALL_POPUP, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			return;

		ImGui::TextWrapped("%s", m_InstallLabel.c_str());
		ImGui::Spacing();
		if (EditorUI::BeginPropertyGrid("##Install", 140.0f))
		{
			EditorUI::PropertyLabel("Install in");
			ImGui::InputText("##InstallRoot", &m_InstallRoot);
			ImGui::SameLine();
			if (ImGui::Button("..."))
			{
				const std::string folder = pfd::select_folder("Folder for the scenery pack", m_InstallRoot).result();
				if (!folder.empty())
					m_InstallRoot = folder;
			}
			EditorUI::PropertyLabel("Pack folder name");
			ImGui::InputText("##InstallName", m_InstallName, sizeof(m_InstallName));
			EditorUI::ItemTooltip("An existing pack with this name is kept; the download gets a suffix instead");
			EditorUI::PropertyLabel("scenery_packs.ini");
			ImGui::Checkbox("Add to the top", &m_InstallRegister);
			EditorUI::ItemTooltip("List the pack in Custom Scenery/scenery_packs.ini so X-Plane loads it with top priority");
			EditorUI::EndPropertyGrid();
		}

		const bool valid = !m_InstallRoot.empty() && m_InstallSceneryId != 0;
		ImGui::BeginDisabled(!valid);
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_DOWNLOAD, "Install").c_str(), ImVec2(120.0f, 0.0f)))
		{
			EditorAirports::InstallGatewayScenery(m_InstallSceneryId, m_InstallRoot, m_InstallName, m_InstallRegister, m_Context);
			m_SelectAirportTab = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}

// -------------------------------------------------------

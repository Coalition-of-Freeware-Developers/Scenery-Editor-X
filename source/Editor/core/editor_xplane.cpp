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
 * editor_xplane.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "editor_xplane.h"
#include "editor_context.h"
#include "editor_theme.h"
#include "editor_ui.h"
#include <portable-file-dialogs.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/xplane/xp_airport_manager.h>
#include <SceneryEditorX/xplane/xp_datarefs.h>
#include <SceneryEditorX/xplane/xp_library_manager.h>
#include <SceneryEditorX/xplane/xp_object.h>
#include <SceneryEditorX/xplane/xp_scene.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorXPlane
{
	namespace
	{
		std::string StartFolder()
		{
			const std::filesystem::path install = XPlane::LibraryManager::GetInstallPath();
			return install.empty() ? std::string() : install.string();
		}
	}

	void BrowseForInstall()
	{
		const std::string folder = pfd::select_folder("Select the X-Plane 12 folder", StartFolder()).result();
		if (folder.empty())
			return;

		if (!XPlane::LibraryManager::SetInstallPath(folder))
			EDITOR_ERROR_TAG("X-Plane", "'{}' is not an X-Plane 12 folder: {}", folder, XPlane::InspectInstall(folder).GetProblem());
		else
			XPlane::AirportManager::Rescan(); // The airport index follows the install folder
	}

	void BrowseForTrackedFolder()
	{
		const std::string folder = pfd::select_folder("Select a library folder to track (contains library.txt)", StartFolder()).result();
		if (folder.empty())
			return;

		if (!XPlane::LibraryManager::AddTrackedFolder(folder))
			EDITOR_WARN_TAG("X-Plane", "'{}' is already tracked or isn't a folder", folder);
	}

	void ImportObjFile(EditorContext *context)
	{
		const std::vector<std::string> files = pfd::open_file("Import X-Plane object", StartFolder(),
			{ "X-Plane objects (*.obj)", "*.obj", "All files", "*" }, pfd::opt::multiselect).result();

		for (const std::string &file : files)
			PlaceObjFile(file, std::string(), context);
	}

	uint64_t PlaceLibraryAsset(const std::string &virtualPath, EditorContext *context, const float *position)
	{
		const std::filesystem::path file = XPlane::LibraryManager::Resolve(virtualPath);
		if (file.empty())
		{
			EDITOR_ERROR_TAG("X-Plane", "'{}' isn't exported by any tracked library", virtualPath);
			return 0;
		}

		if (XPlane::GetAssetType(virtualPath) != XPlane::AssetType::Object)
		{
			EDITOR_WARN_TAG("X-Plane", "Only objects (.obj) can be placed for now: {}", virtualPath);
			return 0;
		}

		return PlaceObjFile(file, virtualPath, context, position);
	}

	uint64_t PlaceObjFile(const std::filesystem::path &file, const std::string &virtualPath, EditorContext *context, const float *position)
	{
		XPlane::SceneObjects::PlaceOptions options;
		options.virtualPath = virtualPath;
		if (position)
		{
			std::copy_n(position, 3, options.position);
			options.frameCamera = false;
		}
		if (!virtualPath.empty())
		{
			const size_t slash = virtualPath.find_last_of('/');
			options.name = slash == std::string::npos ? virtualPath : virtualPath.substr(slash + 1);
		}

		// Returns at once; a load that fails in the background is logged then.
		std::string error;
		const uint64_t root = XPlane::SceneObjects::Place(file, options, &error);
		if (root == 0)
		{
			EDITOR_ERROR_TAG("X-Plane", "Could not place '{}': {}", file.string(), error);
			return 0;
		}

		if (context)
			context->selectedEntity = root;
		return root;
	}

	// -------------------------------------------------------

	void DrawDatarefTable(const char *id, const std::vector<XPlane::Obj8DatarefUsage> *only)
	{
		using namespace XPlane;
		std::map<std::string, DatarefControl> &controls = Datarefs::GetControls();

		std::vector<DatarefControl *> rows;
		if (only)
		{
			for (const Obj8DatarefUsage &usage : *only)
			{
				if (DatarefControl *control = Datarefs::Find(usage.dataref.ToString()))
					rows.push_back(control);
			}
		}
		else
		{
			for (auto &[key, control] : controls)
				rows.push_back(&control);
		}

		if (rows.empty())
		{
			ImGui::TextDisabled(only ? "This object isn't animated." : "No datarefs yet: place an animated object.");
			return;
		}

		constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;
		if (!ImGui::BeginTable(id, 3, flags))
			return;

		ImGui::TableSetupColumn("Dataref", ImGuiTableColumnFlags_WidthStretch, 2.0f);
		ImGui::TableSetupColumn("Driver", ImGuiTableColumnFlags_WidthStretch, 0.9f);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.6f);
		ImGui::TableHeadersRow();

		for (DatarefControl *control : rows)
		{
			ImGui::PushID(control->key.c_str());
			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			const size_t slash = control->key.rfind('/');
			ImGui::TextUnformatted(slash == std::string::npos ? control->key.c_str() : control->key.c_str() + slash + 1);
			EditorUI::ItemTooltip(control->key.c_str());

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			const bool builtin = Datarefs::IsBuiltin(control->key);
			if (ImGui::BeginCombo("##Driver", GetDatarefDriverName(control->driver)))
			{
				for (const DatarefDriver driver : { DatarefDriver::Value, DatarefDriver::Sweep, DatarefDriver::Clock, DatarefDriver::Builtin })
				{
					if (driver == DatarefDriver::Builtin && !builtin)
						continue;
					if (ImGui::Selectable(GetDatarefDriverName(driver), control->driver == driver))
						control->driver = driver;
				}
				ImGui::EndCombo();
			}

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			switch (control->driver)
			{
				case DatarefDriver::Value:
					ImGui::SliderFloat("##Value", &control->value, control->minValue, control->maxValue, "%.3f");
					break;
				case DatarefDriver::Sweep:
					ImGui::DragFloat("##Period", &control->period, 0.05f, 0.1f, 600.0f, "%.2f s / cycle");
					break;
				case DatarefDriver::Clock:
					ImGui::DragFloat("##Rate", &control->rate, 0.01f, -100.0f, 100.0f, "x%.2f");
					break;
				case DatarefDriver::Builtin:
					ImGui::BeginDisabled();
					ImGui::SliderFloat("##Live", &control->value, control->minValue, control->maxValue, "%.3f");
					ImGui::EndDisabled();
					break;
			}
			if (control->driver != DatarefDriver::Value && control->driver != DatarefDriver::Builtin)
			{
				char tooltip[96];
				snprintf(tooltip, sizeof(tooltip), "Current value %.3f (range %.3f .. %.3f)", control->value, control->minValue, control->maxValue);
				EditorUI::ItemTooltip(tooltip);
			}

			ImGui::PopID();
		}

		ImGui::EndTable();
	}

	void DrawSimulationControls()
	{
		using namespace XPlane;

		const bool playing = Datarefs::IsPlaying();
		if (ImGui::Button(EditorUI::IconLabel(playing ? SEDX_ICON_PAUSE : SEDX_ICON_PLAY, playing ? "Pause" : "Play").c_str()))
			Datarefs::SetPlaying(!playing);
		EditorUI::ItemTooltip("Run or freeze the sim clock that drives object animation");

		ImGui::SameLine();
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_STEP_BACKWARD, "Reset").c_str()))
			Datarefs::ResetClock();

		ImGui::SameLine();
		ImGui::TextDisabled("t = %.1f s", Datarefs::GetSimTime());

		if (EditorUI::BeginPropertyGrid("##SimClock", 110.0f))
		{
			EditorUI::PropertyLabel("Speed");
			float scale = Datarefs::GetTimeScale();
			if (ImGui::SliderFloat("##Speed", &scale, 0.0f, 8.0f, "x%.2f"))
				Datarefs::SetTimeScale(scale);

			DatarefEnvironment &environment = Datarefs::GetEnvironment();
			EditorUI::PropertyLabel("Local time");
			ImGui::SliderFloat("##LocalTime", &environment.localTimeHours, 0.0f, 24.0f, "%.2f h");
			EditorUI::ItemTooltip("Drives sim/time/local_time_sec, the sun pitch, the viewport's daylight and when LIT textures and lights switch on");

			EditorUI::PropertyLabel("Wind from");
			ImGui::SliderFloat("##WindDir", &environment.windDirectionDeg, 0.0f, 360.0f, "%.0f deg");

			EditorUI::PropertyLabel("Wind speed");
			ImGui::SliderFloat("##WindSpeed", &environment.windSpeedKnots, 0.0f, 40.0f, "%.0f kt");

			EditorUI::PropertyLabel("Night lighting");
			ImGui::Checkbox("Force lights on", &environment.forceLightsOn);
			EditorUI::ItemTooltip("Show LIT textures, light billboards and spill in daylight too");

			EditorUI::EndPropertyGrid();
		}
	}

	bool DrawObjectLoadProgress(const float width)
	{
		using namespace XPlane;
		const ObjectLoadQueue queue = ObjectAsset::GetLoadQueue();
		if (queue.loads.empty())
			return false;

		// Name the load being worked on: the first one past the queue (workers pick them up in order).
		const ObjectLoadProgress *current = &queue.loads.front();
		for (const ObjectLoadProgress &load : queue.loads)
		{
			if (load.state != ObjectLoadState::Queued)
			{
				current = &load;
				break;
			}
		}

		const uint32_t total = std::max(queue.requested, queue.finished + static_cast<uint32_t>(queue.loads.size()));
		char overlay[256];
		snprintf(overlay, sizeof(overlay), "%s Loading %u / %u: %s", SEDX_ICON_HOURGLASS_HALF, std::min(queue.finished + 1, total), total,
			current->path.filename().string().c_str());
		ImGui::ProgressBar(total > 0 ? static_cast<float>(queue.finished) / static_cast<float>(total) : 0.0f, ImVec2(width, 0.0f), overlay);

		if (ImGui::BeginItemTooltip())
		{
			ImGui::TextUnformatted("Objects loading in the background");
			ImGui::Separator();
			for (const ObjectLoadProgress &load : queue.loads)
				ImGui::Text("%s  -  %s (%.1f s)", load.path.filename().string().c_str(), GetObjectLoadStateName(load.state), load.seconds);
			ImGui::EndTooltip();
		}
		return true;
	}

}

// -------------------------------------------------------

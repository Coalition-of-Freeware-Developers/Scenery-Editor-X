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
 * editor_airports.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "editor_airports.h"
#include "editor_context.h"
#include "editor_scene.h"
#include "editor_ui.h"
#include <array>
#include <cstdlib>
#include <portable-file-dialogs.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/platform/platform_context.h>
#include <SceneryEditorX/filesystem/file_system.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/xplane/xp_airport_manager.h>
#include <SceneryEditorX/xplane/xp_gateway_types.h>
#include <SceneryEditorX/xplane/xp_library_manager.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorAirports
{
	namespace fs = std::filesystem;
	using namespace XPlane;

	namespace
	{
		constexpr const char *LOG_TAG = "Airports";
		constexpr const char *UNSAVED_MODAL = "Unsaved airport changes###AirportUnsaved";

		/* @brief What to do once the unsaved-changes question is answered. */
		enum class Pending : uint8_t
		{
			None,
			Close,
			OpenEntry,
			OpenFile
		};

		struct State
		{
			std::unique_ptr<AirportDocument> document;
			std::vector<std::string> validation;
			uint64_t validationRevision = ~0ull;

			std::string startupAirport;			// --xp-airport <id>, opened once the index is ready
			bool startupDone = false;
			std::array<float, 5> startupCamera{};	// --xp-airport-camera x,y,z,yaw,pitch: view from the center after opening it
			bool hasStartupCamera = false;
			int startupSceneryId = 0;			// --xp-gateway <scenery id>: install and open a Gateway submission at startup
			fs::path startupPackageRoot;		// --xp-package-root <folder>: where that pack goes (Custom Scenery when empty)
			bool startupInstallDone = false;

			// Unsaved-changes modal
			Pending pending = Pending::None;
			bool openModal = false;
			std::shared_ptr<const AirportIndex> pendingIndex;
			AirportEntry pendingEntry;
			fs::path pendingPath;
			std::string pendingId;

			// Gateway install in flight
			bool installActive = false;
			PendingInstall install;
			fs::path installRoot;
			std::string installName;
			bool installRegister = true;
			EditorContext *installContext = nullptr;
		};

		State &GetState()
		{
			static State state;
			return state;
		}

		std::string StartFolder()
		{
			const fs::path root = AirportManager::GetDefaultPackageRoot();
			return root.empty() ? std::string() : root.string();
		}

		void LoadIntoScene(EditorContext *context, const bool frame)
		{
			State &state = GetState();
			if (!state.document)
				return;
			const uint64_t root = SceneAirports::Load(*state.document, frame);
			if (context && root != 0)
				context->selectedEntity = root;
			state.validationRevision = ~0ull;
		}

		void FinishOpen(std::unique_ptr<AirportDocument> document, EditorContext *context)
		{
			State &state = GetState();
			state.document = std::move(document);
			LoadIntoScene(context, true);
			EDITOR_INFO_TAG(LOG_TAG, "Opened {} from {}{}", state.document->GetTitle(), state.document->GetPackageName(), state.document->IsReadOnly() ? " (read-only)" : "");
		}

		bool OpenEntryNow(const AirportIndex &index, const AirportEntry &entry, EditorContext *context)
		{
			auto document = std::make_unique<AirportDocument>();
			std::string error;
			if (!AirportDocument::Open(index, entry, *document, &error))
			{
				EDITOR_ERROR_TAG(LOG_TAG, "Could not open {}: {}", entry.id, error);
				return false;
			}
			FinishOpen(std::move(document), context);
			return true;
		}

		bool OpenFileNow(const fs::path &aptDat, const std::string_view airportId, EditorContext *context)
		{
			auto document = std::make_unique<AirportDocument>();
			std::string error;
			const bool readOnly = IsProtectedPackage(aptDat.parent_path().parent_path(), LibraryManager::GetInstallPath());
			if (!AirportDocument::OpenFile(aptDat, airportId, *document, &error, readOnly))
			{
				EDITOR_ERROR_TAG(LOG_TAG, "Could not open {}: {}", aptDat.string(), error);
				return false;
			}
			FinishOpen(std::move(document), context);
			return true;
		}

		/* @brief True when the open airport may be dropped; otherwise queues the action behind the modal. */
		bool ConfirmDiscard(const Pending action)
		{
			State &state = GetState();
			if (!state.document || !state.document->IsDirty())
				return true;
			state.pending = action;
			state.openModal = true;
			return false;
		}

		void RunPending(EditorContext *context)
		{
			State &state = GetState();
			const Pending action = state.pending;
			state.pending = Pending::None;
			switch (action)
			{
				case Pending::Close:
					CloseAirport(context, true);
					break;
				case Pending::OpenEntry:
					if (state.pendingIndex)
						OpenEntryNow(*state.pendingIndex, state.pendingEntry, context);
					break;
				case Pending::OpenFile:
					OpenFileNow(state.pendingPath, state.pendingId, context);
					break;
				default:
					break;
			}
			state.pendingIndex.reset();
		}

		/* @brief Lets the airport index see a pack: a rescan when it's under the install, tracking otherwise. */
		void IndexPackage(const fs::path &packageFolder)
		{
			const fs::path root = AirportManager::GetDefaultPackageRoot();
			std::error_code ec;
			const bool underInstall = !root.empty() && fs::equivalent(packageFolder.parent_path(), root, ec);
			if (!underInstall)
				AirportManager::AddTrackedPackage(packageFolder);
			AirportManager::Rescan();
		}

		fs::path UniquePackageFolder(const fs::path &root, const std::string &name, const int sceneryId)
		{
			fs::path folder = root / name;
			if (!IsAirportPackage(folder))
				return folder;
			// Don't clobber a pack already there (someone's edits): add the submission id.
			folder = root / (name + " (Gateway " + std::to_string(sceneryId) + ")");
			int counter = 2;
			while (IsAirportPackage(folder))
				folder = root / (name + " (Gateway " + std::to_string(sceneryId) + ") " + std::to_string(counter++));
			return folder;
		}

		void FinishInstall(EditorContext *context)
		{
			State &state = GetState();
			PendingInstall &install = state.install;
			const std::shared_ptr<GatewayRequest> request = install.request;
			if (!request || !request->IsFinished())
				return;

			state.installActive = false;
			if (!request->Succeeded())
			{
				install.error = request->GetError().empty() ? std::string(GetGatewayRequestStateName(request->GetState())) : request->GetError();
				EDITOR_ERROR_TAG(LOG_TAG, "Gateway download failed: {}", install.error);
				return;
			}

			const GatewaySceneryPack &pack = *request->scenery;
			fs::path root = state.installRoot.empty() ? AirportManager::GetDefaultPackageRoot() : state.installRoot;
			if (root.empty())
			{
				install.error = "No X-Plane folder is set and no destination was chosen";
				EDITOR_ERROR_TAG(LOG_TAG, "{}", install.error);
				return;
			}
			const std::string name = state.installName.empty() ? MakeGatewayPackageName(pack.icao, pack.airportName) : state.installName;
			install.packageFolder = UniquePackageFolder(root, name, pack.info.sceneryId);

			fs::path aptDat;
			std::string error;
			if (!GatewayClient::InstallScenery(pack, install.packageFolder, &aptDat, &error))
			{
				install.error = error;
				EDITOR_ERROR_TAG(LOG_TAG, "Could not install Gateway scenery {}: {}", pack.info.sceneryId, error);
				return;
			}

			const fs::path xplane = LibraryManager::GetInstallPath();
			if (state.installRegister && !xplane.empty())
			{
				std::error_code ec;
				if (fs::equivalent(install.packageFolder.parent_path(), xplane / "Custom Scenery", ec) && !RegisterSceneryPack(xplane, install.packageFolder, &error))
					EDITOR_WARN_TAG(LOG_TAG, "scenery_packs.ini not updated: {}", error);
			}
			IndexPackage(install.packageFolder);
			EDITOR_INFO_TAG(LOG_TAG, "Installed {} {} (submission {} by {}) to {}", pack.icao, pack.airportName, pack.info.sceneryId, pack.info.userName, install.packageFolder.string());

			if (ConfirmDiscard(Pending::OpenFile))
				OpenFileNow(aptDat, pack.icao, context);
			else
			{
				state.pendingPath = aptDat;
				state.pendingId = pack.icao;
			}
		}
	}

	// -------------------------------------------------------

	void Init()
	{
		AirportManager::Init();
		GatewayClient::Init();

		State &state = GetState();
		if (const PlatformContext *platform = Application::Get().GetPlatformContext())
		{
			const std::vector<std::string> &args = platform->GetCommandLineArgs();
			for (size_t i = 0; i + 1 < args.size(); ++i)
			{
				if (args[i] == "--xp-airport")
					state.startupAirport = args[++i];
				else if (args[i] == "--xp-airport-camera")
				{
					// "x,y,z,yaw,pitch": meters from the airport's center (+X east, +Y up, +Z south) and degrees
					const char *text = args[++i].c_str();
					char *end = nullptr;
					for (float &value : state.startupCamera)
					{
						value = std::strtof(text, &end);
						text = *end == ',' ? end + 1 : end;
					}
					state.hasStartupCamera = true;
				}
				else if (args[i] == "--xp-gateway")
					state.startupSceneryId = std::atoi(args[++i].c_str());
				else if (args[i] == "--xp-package-root")
					state.startupPackageRoot = args[++i];
			}
		}
	}

	void Shutdown()
	{
		State &state = GetState();
		SceneAirports::Shutdown();
		state.document.reset();
		state.install = {};
		state.pendingIndex.reset();
		GatewayClient::Shutdown();
		AirportManager::Shutdown();
	}

	void Tick(EditorContext *context)
	{
		State &state = GetState();

		// Library art (pavement textures, lines, lights, lib/ objects) resolves through the library index: until the
		// first scan has published one, an airport would load bare and need a second build.
		const bool libraryReady = LibraryManager::GetGeneration() > 0 || LibraryManager::GetScanState() == ScanState::Failed || LibraryManager::GetInstallPath().empty();

		// Airport asked for on the command line, once the airport and library indexes are ready
		if (!state.startupDone && !state.startupAirport.empty() && AirportManager::GetGeneration() > 0 && libraryReady)
		{
			state.startupDone = true;
			const std::shared_ptr<const AirportIndex> index = AirportManager::GetIndex();
			if (const AirportEntry *entry = index->FindBest(state.startupAirport))
			{
				if (OpenEntryNow(*index, *entry, context) && state.hasStartupCamera)
				{
					const std::array<float, 5> &view = state.startupCamera;
					SceneAirports::LookFrom(view[0], view[1], view[2], view[3], view[4]);
				}
			}
			else
				EDITOR_ERROR_TAG(LOG_TAG, "--xp-airport: no airport '{}' in the indexed scenery", state.startupAirport);
		}

		// Gateway submission asked for on the command line (verification runs: no clicking needed)
		if (!state.startupInstallDone && state.startupSceneryId != 0)
		{
			state.startupInstallDone = true;
			InstallGatewayScenery(state.startupSceneryId, state.startupPackageRoot, std::string(), false, context);
		}

		if (state.document)
		{
			if (SceneAirports::IsLoaded())
			{
				if (SceneAirports::GetBuiltGeometryRevision() != state.document->GetGeometryRevision())
				{
					// Entities are recreated: keep the selection on the same element.
					AirportElementComponent selected;
					const AirportElementComponent *element = context ? SceneAirports::FindElement(context->selectedEntity) : nullptr;
					if (element)
						selected = *element;
					SceneAirports::Rebuild(*state.document);
					if (element && context)
					{
						const uint64_t entity = selected.kind == AirportElementKind::Airport ? SceneAirports::GetRootId() : SceneAirports::FindEntity(selected.kind, selected.index, selected.subIndex);
						context->selectedEntity = entity != 0 ? entity : SceneAirports::GetRootId();
					}
				}
				else if (SceneAirports::GetBuiltRevision() != state.document->GetRevision())
					SceneAirports::UpdatePlacements(*state.document);

				// A newer library index (first scan finished after the airport opened, a rescan, a library added):
				// rebuild once so the art that couldn't be found before is drawn.
				if (libraryReady && LibraryManager::GetScanState() != ScanState::Scanning && SceneAirports::GetBuiltLibraryGeneration() != LibraryManager::GetGeneration())
				{
					EDITOR_INFO_TAG(LOG_TAG, "The X-Plane library index changed: rebuilding {}", state.document->GetTitle());
					const bool rootSelected = context && context->selectedEntity == SceneAirports::GetRootId();
					SceneAirports::Rebuild(*state.document);
					if (rootSelected)
						context->selectedEntity = SceneAirports::GetRootId();
				}
			}
			if (state.validationRevision != state.document->GetRevision())
			{
				state.validation = ValidateAptAirport(state.document->airport);
				state.validationRevision = state.document->GetRevision();
			}
		}

		if (state.installActive)
			FinishInstall(context);
	}

	AirportDocument *GetDocument()
	{
		return GetState().document.get();
	}

	void MarkEdited(const bool geometryChanged)
	{
		if (AirportDocument *document = GetDocument())
			document->MarkDirty(geometryChanged);
	}

	bool OpenAirport(const AirportEntry &entry, EditorContext *context)
	{
		State &state = GetState();
		const std::shared_ptr<const AirportIndex> index = AirportManager::GetIndex();
		if (!ConfirmDiscard(Pending::OpenEntry))
		{
			state.pendingIndex = index;
			state.pendingEntry = entry;
			return false;
		}
		return OpenEntryNow(*index, entry, context);
	}

	bool OpenAptDat(const fs::path &aptDat, const std::string_view airportId, EditorContext *context)
	{
		State &state = GetState();
		if (!ConfirmDiscard(Pending::OpenFile))
		{
			state.pendingPath = aptDat;
			state.pendingId = std::string(airportId);
			return false;
		}
		return OpenFileNow(aptDat, airportId, context);
	}

	void BrowseForAirport(EditorContext *context)
	{
		const std::vector<std::string> files = pfd::open_file("Open X-Plane airport data", StartFolder(), { "Airport data (apt.dat)", "apt.dat *.dat", "All files", "*" }).result();
		if (files.empty())
			return;
		OpenAptDat(files.front(), std::string_view(), context);
	}

	bool CreateAirport(const std::string &id, const std::string &name, const double latitude, const double longitude, const AirportType type, const int elevationFt,
		const fs::path &packageRoot, const std::string &packageName, EditorContext *context)
	{
		if (!ConfirmDiscard(Pending::None))
		{
			EDITOR_WARN_TAG(LOG_TAG, "Save or close the open airport first");
			return false;
		}

		const fs::path root = packageRoot.empty() ? AirportManager::GetDefaultPackageRoot() : packageRoot;
		if (root.empty())
		{
			EDITOR_ERROR_TAG(LOG_TAG, "Choose a folder for the new scenery pack (no X-Plane folder is set)");
			return false;
		}
		const fs::path folder = root / (packageName.empty() ? MakeGatewayPackageName(id, name) : packageName);
		if (IsAirportPackage(folder))
		{
			EDITOR_ERROR_TAG(LOG_TAG, "{} already holds an apt.dat: open it instead", folder.string());
			return false;
		}

		auto document = std::make_unique<AirportDocument>();
		std::string error;
		if (!AirportDocument::Create(folder, MakeNewAirport(id, name, latitude, longitude, type, elevationFt), *document, &error))
		{
			EDITOR_ERROR_TAG(LOG_TAG, "Could not create {}: {}", folder.string(), error);
			return false;
		}

		const fs::path xplane = LibraryManager::GetInstallPath();
		std::error_code ec;
		if (!xplane.empty() && fs::equivalent(folder.parent_path(), xplane / "Custom Scenery", ec) && !RegisterSceneryPack(xplane, folder, &error))
			EDITOR_WARN_TAG(LOG_TAG, "scenery_packs.ini not updated: {}", error);
		IndexPackage(folder);

		FinishOpen(std::move(document), context);
		EDITOR_INFO_TAG(LOG_TAG, "Created scenery pack {}", folder.string());
		return true;
	}

	bool SaveAirport()
	{
		State &state = GetState();
		if (!state.document)
			return false;
		if (state.document->IsReadOnly() || !state.document->HasFile())
			return SaveAirportAs();

		std::string error;
		if (!state.document->Save(&error))
		{
			EDITOR_ERROR_TAG(LOG_TAG, "Save failed: {}", error);
			return false;
		}
		AirportManager::Rescan();
		EDITOR_INFO_TAG(LOG_TAG, "Saved {} to {}", state.document->airport.id, state.document->GetAptDatPath().string());
		return true;
	}

	bool SaveAirportAs()
	{
		State &state = GetState();
		if (!state.document)
			return false;

		const std::string chosen = pfd::select_folder("Choose the scenery pack folder for " + state.document->airport.id + " (Custom Scenery to create a new one)", StartFolder()).result();
		if (chosen.empty())
			return false;

		// Picking the Custom Scenery folder itself means "make a pack for me in there".
		fs::path folder = chosen;
		const fs::path root = AirportManager::GetDefaultPackageRoot();
		std::error_code ec;
		if (!root.empty() && fs::equivalent(folder, root, ec))
			folder = root / MakeGatewayPackageName(state.document->airport.id, state.document->airport.name);
		else if (IsProtectedPackage(folder, LibraryManager::GetInstallPath()))
		{
			EDITOR_ERROR_TAG(LOG_TAG, "{} belongs to X-Plane; pick a folder in Custom Scenery", folder.string());
			return false;
		}
		return SaveAirportAs(folder);
	}

	bool SaveAirportAs(const fs::path &packageFolder)
	{
		State &state = GetState();
		if (!state.document)
			return false;

		std::string error;
		if (!state.document->SaveAs(packageFolder, &error))
		{
			EDITOR_ERROR_TAG(LOG_TAG, "Save As failed: {}", error);
			return false;
		}

		const fs::path xplane = LibraryManager::GetInstallPath();
		std::error_code ec;
		if (!xplane.empty() && fs::equivalent(packageFolder.parent_path(), xplane / "Custom Scenery", ec) && !RegisterSceneryPack(xplane, packageFolder, &error))
			EDITOR_WARN_TAG(LOG_TAG, "scenery_packs.ini not updated: {}", error);
		IndexPackage(packageFolder);
		EDITOR_INFO_TAG(LOG_TAG, "Saved {} to {}", state.document->airport.id, state.document->GetAptDatPath().string());
		return true;
	}

	bool CloseAirport(EditorContext *context, const bool force)
	{
		State &state = GetState();
		if (!state.document)
			return true;
		if (!force && !ConfirmDiscard(Pending::Close))
			return false;

		const uint64_t root = SceneAirports::GetRootId();
		SceneAirports::Unload();
		if (context && (context->selectedEntity == root || SceneAirports::FindElement(context->selectedEntity)))
			context->selectedEntity = 0;
		EDITOR_INFO_TAG(LOG_TAG, "Closed {}", state.document->airport.id);
		state.document.reset();
		state.validation.clear();
		return true;
	}

	void FrameAirport()
	{
		SceneAirports::Frame();
	}

	void RevealPackage()
	{
		const AirportDocument *document = GetDocument();
		if (document && !document->GetPackageFolder().empty())
			IO::FileSystem::OpenDirectoryInExplorer(document->GetPackageFolder());
	}

	void OpenGatewayPage(const std::string &code)
	{
		if (!code.empty())
			IO::FileSystem::OpenUrl(GetGatewayAirportPageUrl(code));
	}

	void InstallGatewayScenery(const int sceneryId, const fs::path &packageRoot, const std::string &packageName, const bool registerInSim, EditorContext *context)
	{
		State &state = GetState();
		if (state.installActive)
		{
			EDITOR_WARN_TAG(LOG_TAG, "A Gateway download is already running");
			return;
		}
		state.install = {};
		state.install.request = GatewayClient::FetchScenery(sceneryId);
		state.install.label = "Scenery " + std::to_string(sceneryId);
		state.installRoot = packageRoot;
		state.installName = packageName;
		state.installRegister = registerInSim;
		state.installContext = context;
		state.installActive = true;
	}

	const PendingInstall *GetPendingInstall()
	{
		State &state = GetState();
		return state.install.request ? &state.install : nullptr;
	}

	void DrawModals(EditorContext *context)
	{
		State &state = GetState();
		if (state.openModal)
		{
			ImGui::OpenPopup(UNSAVED_MODAL);
			state.openModal = false;
		}

		const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal(UNSAVED_MODAL, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			const std::string title = state.document ? state.document->GetTitle() : std::string("the airport");
			ImGui::Text("%s has unsaved changes.", title.c_str());
			if (state.document && state.document->IsReadOnly())
				ImGui::TextDisabled("It came from %s, so saving writes a copy into a scenery pack of yours.", state.document->GetPackageName().c_str());
			ImGui::Spacing();

			if (ImGui::Button("Save", ImVec2(120.0f, 0.0f)))
			{
				if (SaveAirport())
				{
					ImGui::CloseCurrentPopup();
					RunPending(context);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Don't save", ImVec2(120.0f, 0.0f)))
			{
				ImGui::CloseCurrentPopup();
				if (state.document)
					CloseAirport(context, true);
				RunPending(context);
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
			{
				ImGui::CloseCurrentPopup();
				state.pending = Pending::None;
				state.pendingIndex.reset();
			}
			ImGui::EndPopup();
		}
	}

	const std::vector<std::string> &GetValidationProblems()
	{
		return GetState().validation;
	}

	const AirportElementComponent *GetSelectedElement(const EditorContext *context)
	{
		return context ? SceneAirports::FindElement(context->selectedEntity) : nullptr;
	}
}

// -------------------------------------------------------

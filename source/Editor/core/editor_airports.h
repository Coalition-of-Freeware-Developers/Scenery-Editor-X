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
 * editor_airports.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <SceneryEditorX/xplane/xp_airport_document.h>
#include <SceneryEditorX/xplane/xp_airport_index.h>
#include <SceneryEditorX/xplane/xp_airport_scene.h>
#include <SceneryEditorX/xplane/xp_gateway.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorContext;
}

namespace SceneryEditorX::EditorAirports
{
	/**
	 * Editor actions for X-Plane airports, shared by the menu bar, the AIRPORTS panel and the properties panel: the
	 * one open airport document, its scene representation, and the Gateway downloads in flight. Main thread only.
	 */

	/**
	 * @brief Starts the airport index, the Gateway client and reads the command line: --xp-airport <id> opens an
	 * indexed airport, --xp-gateway <scenery id> installs and opens a Gateway submission (into --xp-package-root
	 * <folder>, or Custom Scenery).
	 */
	void Init();

	/* @brief Unloads the open airport and stops the background services. Call before the renderer shuts down. */
	void Shutdown();

	/* @brief Once per frame: opens the startup airport when the index is ready and rebuilds the scene after edits. */
	void Tick(EditorContext *context);

	/* @brief The open airport, or nullptr. */
	XPlane::AirportDocument *GetDocument();

	/**
	 * @brief Records an edit to the open airport; the scene catches up on the next Tick().
	 * @param geometryChanged False when only a point feature's position or heading changed (its entity is moved
	 * instead of rebuilding every mesh).
	 */
	void MarkEdited(bool geometryChanged = true);

	/**
	 * @brief Records edits to the geometry of existing rows (moved, rotated, reshaped): only those rows are rebuilt on
	 * the next Tick(), not the airport. Rows added or removed need MarkEdited(true).
	 */
	void MarkElementsEdited(const std::vector<XPlane::SceneAirports::ElementRef> &rows);

	/**
	 * @brief Opens an airport by id once the airport and library indexes are ready (as --xp-airport does), so its
	 * library art resolves on the first build. Parity scenarios use it.
	 */
	void RequestOpenWhenReady(const std::string &airportId);

	/* @brief Opens an airport the index found, replacing the open one (asks about unsaved changes via the modal). */
	bool OpenAirport(const XPlane::AirportEntry &entry, EditorContext *context);

	/* @brief Opens the first (or named) airport of an apt.dat file. */
	bool OpenAptDat(const std::filesystem::path &aptDat, std::string_view airportId, EditorContext *context);

	/* @brief Asks for an apt.dat or a scenery pack folder and opens it. */
	void BrowseForAirport(EditorContext *context);

	/**
	 * @brief Creates a new scenery pack holding a new airport and opens it.
	 * @param packageRoot Folder the pack is created in (Custom Scenery when empty).
	 * @param packageName Pack folder name; derived from the id and name when empty.
	 */
	bool CreateAirport(const std::string &id, const std::string &name, double latitude, double longitude, XPlane::AirportType type, int elevationFt,
		const std::filesystem::path &packageRoot, const std::string &packageName, EditorContext *context);

	/**
	 * @brief Opens an airport that belongs to the open project (no apt.dat of its own): edits stay in the document
	 * until the project saves it, and Save Airport saves the project through the handler set below. Replaces the open
	 * airport without asking (the project flow asked already).
	 */
	bool OpenProjectAirport(const XPlane::AptAirport &airport, EditorContext *context, bool frame = true);

	/* @brief True when the open airport belongs to the project (opened with OpenProjectAirport). */
	bool IsProjectAirport();

	/* @brief What saving a project-owned airport does (the project's save). Null clears it. */
	void SetProjectSaveHandler(std::function<bool()> handler);

	/* @brief Saves the open airport into its pack (or, for a project airport, saves the project). False (with a logged reason) for read-only sources. */
	bool SaveAirport();

	/* @brief Saves the open airport into a pack folder chosen in a folder picker (created when needed). */
	bool SaveAirportAs();
	bool SaveAirportAs(const std::filesystem::path &packageFolder);

	/* @brief Closes the open airport and removes it from the scene. False when unsaved changes block it (the modal asks). */
	bool CloseAirport(EditorContext *context, bool force = false);

	/* @brief Moves the camera to the open airport. */
	void FrameAirport();

	/* @brief Reveals the open airport's pack folder in the file manager. */
	void RevealPackage();

	/* @brief Opens the airport's page on the Gateway website in the browser. */
	void OpenGatewayPage(const std::string &code);

	/**
	 * @brief Downloads a Gateway submission and installs it as a scenery pack, then opens it (the request runs in the
	 * background; Tick() finishes the job).
	 * @param packageRoot Folder the pack goes in (Custom Scenery when empty).
	 * @param packageName Folder name; "<ICAO> - <Name>" when empty.
	 * @param registerInSim Also list the pack in scenery_packs.ini.
	 */
	void InstallGatewayScenery(int sceneryId, const std::filesystem::path &packageRoot, const std::string &packageName, bool registerInSim, EditorContext *context);

	/* @brief The Gateway install in progress (null when idle), for the UI. */
	struct PendingInstall
	{
		std::shared_ptr<XPlane::GatewayRequest> request;
		std::filesystem::path packageFolder;
		std::string label;
		std::string error;
	};
	const PendingInstall *GetPendingInstall();

	/* @brief Draws the "unsaved changes" modal when an action asked for it. Call every frame from the layer. */
	void DrawModals(EditorContext *context);

	/* @brief Validation of the open airport, recomputed after edits. */
	const std::vector<std::string> &GetValidationProblems();

	/* @brief Converts a scene selection to the airport element it stands for (or nullptr). */
	const XPlane::AirportElementComponent *GetSelectedElement(const EditorContext *context);
}

// -------------------------------------------------------

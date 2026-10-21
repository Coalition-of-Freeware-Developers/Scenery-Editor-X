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
 * editor_project.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <filesystem>
#include <string>
#include <SceneryEditorX/project/project_document.h>
#include <SceneryEditorX/project/scenery_pack.h>
#include <SceneryEditorX/settings/user_settings.h>
#include <SceneryEditorX/xplane/xp_apt_geometry.h>
#include <SceneryEditorX/xplane/xp_assets.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorContext;
}

namespace SceneryEditorX::EditorProjects
{
	/**
	 * Editor actions for Scenery Editor X projects (.edX / .sex, WorldEditor import / export), shared by the File menu,
	 * the shortcuts and the PROJECT panel. One project is open at a time. Main thread only.
	 *
	 * The scene shows the project: its first airport opens in the airport editor (edits stay in the project), its
	 * objects are placed through the X-Plane object loader, and its other visible placements (draped polygons and
	 * orthophotos, lines, strings, forests, facades, autogen points) are drawn the way a scenery pack's DSF is, one entity
	 * per item, redrawn when they change. Moving, deleting or placing X-Plane objects in the scene
	 * flows back into the project (checked every couple of seconds and before every save). Unsaved work is autosaved
	 * into the project cache (never the project file) and offered back after a crash.
	 */

	/* @brief Sets up the cache and reads --project <file> (opened once the X-Plane library index is ready). */
	void Init(const Ref<UserPreferences> &preferences);

	/* @brief Keeps unsaved changes as recovery data and closes the project. Call before the scene goes away. */
	void Shutdown();

	/* @brief Once per frame: the startup project, scene sync, autosave, the window title. */
	void Tick(EditorContext *context);

	/* @brief The open project, or nullptr. */
	ProjectFile::ProjectDocument *GetDocument();

	/* @brief Replaces the open project's saved library-browser filters as one undo step. False without a project. */
	bool SetLibraryFilters(std::vector<ProjectFile::LibraryFilter> filters, const std::string &label);

	/* @brief True when the project (or its airport in the airport editor) has unsaved changes. */
	bool IsDirty();

	/* @brief "Name *" for the window title, empty without a project. */
	std::string GetTitle();

	/**
	 * @brief Asks about unsaved changes, then shows the New Project form: a scenery pack (no airport needed), an airport
	 * or an asset library; the pack is created and opened from there.
	 */
	void NewProject(EditorContext *context);

	/**
	 * @brief Adds a new airport (the New Project form's airport fields) to the open project. The first airport of a
	 * project goes to the airport editor and centers the scene unless the project has a datum of its own.
	 */
	bool AddNewAirport(const ProjectFile::NewAirportProject &spec, EditorContext *context, std::string *error = nullptr);

	/* @brief Asks where to create an empty project file (library projects, imports) and opens it. */
	void NewEmptyProject(EditorContext *context);

	/* @brief Asks for a project file (.edX, .sex, earth.wed.xml) and opens it. */
	void BrowseOpen(EditorContext *context);

	/**
	 * @brief Opens a project file (asks about unsaved changes first).
	 * @param discardUnsaved The user was asked already (the New Airport Project form): unsaved changes are dropped.
	 */
	bool Open(const std::filesystem::path &path, EditorContext *context, bool discardUnsaved = false);

	/* @brief Opens a project once the X-Plane library index is ready (like --project). */
	void OpenWhenReady(const std::filesystem::path &path);

	/* @brief Saves the project (Save As for one without a file). */
	bool Save();
	bool SaveAs();

	/* @brief Closes the project and removes it from the scene. False when unsaved changes block it (the modal asks). */
	bool Close(EditorContext *context, bool force = false);

	/* @brief Opens the project again from its file, dropping unsaved changes and their recovery data. False without a file. */
	bool Revert(EditorContext *context);

	/* @brief Starts a project from a WorldEditor earth.wed.xml (unsaved; Save offers a .edX beside it). */
	void ImportWed(EditorContext *context);

	/* @brief Writes the project as a WorldEditor earth.wed.xml. */
	void ExportWed();

	/* @brief Adds the airports of an apt.dat to the project. */
	void ImportAptDat(EditorContext *context);

	/* @brief Writes the project's airports as an apt.dat (a scenery pack's "Earth nav data/apt.dat"). */
	void ExportAptDat();

	/* @brief Replaces the project's library with a library.txt (library projects). */
	void ImportLibrary();

	/* @brief Writes the project's library as library.txt (beside the project file by default). */
	void ExportLibrary();

	/* @brief Writes a binary copy (.sex) of the project. */
	void ExportBinary();

	/* @brief Reveals the project folder in the file manager. */
	void RevealFolder();

	/* @brief The folder of the project file (or where an unsaved import will be saved), empty without one. */
	std::filesystem::path GetProjectFolder();

	/* @brief Shows another of the project's airports in the airport editor (edits to the current one are kept). */
	void ShowAirport(const std::string &airportId, EditorContext *context);

	/* @brief The airport the airport editor shows for the project, empty when none. */
	const std::string &GetShownAirport();

	/* @brief The scene entity drawing a project item: an object's root, a polygon's (line's, forest's...) entity; 0 when it isn't drawn. */
	uint64_t GetPlacedEntity(const std::string &itemId);

	/* @brief The project item an entity is part of (empty when none). */
	std::string FindItemForEntity(uint64_t entityId);

	/* @brief Removes an item (and its scene entities) from the project. */
	bool RemoveItem(const std::string &itemId, EditorContext *context);

	/* @brief Where an item sits in the hierarchy: its container's id (empty at the top level) and its position there. */
	bool FindItemLocation(const std::string &itemId, std::string &parentId, size_t &index);

	/* @brief Puts an item (back) into the project at a position and into the scene (objects placed, the rest drawn). */
	bool InsertItem(ProjectFile::SceneryItem item, const std::string &parentId, size_t index);

	/**
	 * @brief Fills the project's library references from the X-Plane library index: every library a placed resource
	 * comes from gets a reference, and each item its "associated-library". Resources the index doesn't know are left alone.
	 * @return Number of items that got a library.
	 */
	size_t AssignLibraries();

	/* @brief Records an edit made through the PROJECT panel (info, libraries, library exports). */
	void MarkEdited(ProjectFile::ProjectDocument::Section section);

	/* @brief The local frame the project is drawn in (+X east, -Z north, meters): where item coordinates land in the scene. */
	const XPlane::GeoProjection &GetProjection();

	/* @brief A road network (.net) a road item names, parsed once (null when it can't be found or read). */
	const XPlane::RoadNetworkAsset *GetRoadNetwork(const std::string &resource);

	/* @brief A facade (.fac) an item names, parsed once per library scan (null when it can't be found or read). */
	const XPlane::FacadeAsset *GetFacade(const std::string &resource);

	/* @brief The file behind a resource: a path relative to the project folder, an absolute one or a library path (empty when not found). */
	std::filesystem::path ResolveResourceFile(const std::string &resource);

	/**
	 * @brief Adds an item to the project as one undo step and selects it: after the active project item (in its
	 * container), else into the project's first airport (where WorldEditor keeps a pack's art), else at the top level.
	 * @return The new item's id (a fresh one when it had none or a taken one), empty without a project.
	 */
	std::string AddItem(ProjectFile::SceneryItem item, EditorContext *context, const std::string &label);

	/* @brief Reads the art the project's scenery is drawn with again (after a .fac, .pol... changed on disk) and redraws it. */
	void ReloadArt();

	/* @brief Brings scene edits (objects moved, turned or deleted) into the project now instead of at the next periodic sync. */
	void NotifySceneEdited();

	/* @brief Problems ValidateProject finds, recomputed after edits. */
	const std::vector<std::string> &GetValidationProblems();

	/* @brief The File menu's project items. */
	void DrawFileMenu(EditorContext *context);

	/* @brief The unsaved-changes and recovery modals. Call every frame from the layer. */
	void DrawModals(EditorContext *context);
}

// -------------------------------------------------------

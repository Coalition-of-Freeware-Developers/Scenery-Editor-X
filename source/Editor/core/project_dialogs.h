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
 * project_dialogs.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorContext;
}

namespace SceneryEditorX::ProjectDialogs
{
	/**
	 * The New Airport Project form and the Export Scenery Pack dialog, plus the editor's VFS mounts:
	 *
	 *  - New Airport Project asks for the ICAO code, name, country (city and state / province / county optional),
	 *    the reference point in decimal degrees (8 places, north and east positive) and the elevation, then creates
	 *    the pack folder (Custom Scenery by default, any folder on request) laid out like the other packs there, with
	 *    the project file inside, and opens it.
	 *  - Export Scenery Pack writes the open project into a pack X-Plane loads (apt.dat, DSF tiles, the pack's art,
	 *    library.txt, optionally earth.wed.xml), lists it in scenery_packs.ini and reports what it wrote.
	 *  - The VFS mounts follow the editor: "xplane" (read-only) and "scenery" from the X-Plane install, "user" and
	 *    "cache" from the platform folders, "project" from the open project's folder.
	 *
	 * Command line (verification runs): --new-airport-project "ICAO;Name;Country;lat;lon;elevation ft[;folder]" creates
	 * and opens a project; --export-scenery [folder] exports the project once it's open and logs the report;
	 * --show-dialog new|export opens a dialog at startup (for screenshots).
	 * Main thread only.
	 */

	/* @brief Reads the command line and mounts the platform folders. */
	void Init();

	/* @brief Once per frame: keeps the mounts current and runs a pending command-line export. */
	void Tick(EditorContext *context);

	/* @brief Opens the New Airport Project form (unsaved changes were dealt with already). */
	void OpenNewProject();

	/* @brief Opens the Export Scenery Pack dialog for the open project. */
	void OpenExport();

	/* @brief Exports the open project again with the last settings (into its own folder the first time). */
	bool QuickExport();

	/* @brief Draws the open dialog. Call every frame. */
	void Draw(EditorContext *context);
}

// -------------------------------------------------------

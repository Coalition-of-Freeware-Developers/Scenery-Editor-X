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
 * project_wed.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "scenery_project.h"
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	/**
	 * WorldEditor (WED 2.x) projects: the earth.wed.xml at the root of a scenery pack, converted to and from a
	 * project. Conventions follow xptools' WED sources: enums are stored by their display names ("Asphalt Worn",
	 * "Tie-Down", "1 Default"), elevations and heights in meters, frequencies in MHz, bezier handles as offsets from
	 * their node, and airports map to apt.dat the way WED's own apt.dat export and import do (split bezier nodes become
	 * co-located apt.dat nodes and back).
	 *
	 * What has no counterpart on the other side, with the warning it produces:
	 *  - WED classes this model doesn't know (roads, terrain, shapes, overlay images, ...) are kept verbatim as
	 *    "wed-raw" items and written back unchanged, as are hidden airport elements (apt.dat has no hidden flag)
	 *  - apt.dat jetways (1500), unknown rows and legacy row-15 startups have no WED class: jetways and unknown rows
	 *    are left out of the earth.wed.xml (they stay in the project), row-15 startups become ramp starts
	 *  - curved taxi routes become straight (apt.dat 1203 shape points aren't modeled yet)
	 */

	struct WedImportOptions
	{
		/**
		 * Painted line / light string names beyond WED's built-in ones, from the library: WED names an apt.dat line code
		 * after its lib/airport/lines/<code>_<name>.lin file (see MakeWedLineName). X-Plane 12's default lines are built in.
		 */
		std::vector<std::pair<std::string, int>> extraLineNames;
	};

	struct WedExportOptions
	{
		std::vector<std::pair<std::string, int>> extraLineNames;
		std::string writer = "Scenery Editor X";	// Written into the <!-- written by ... --> comment
	};

	/**
	 * @brief Converts earth.wed.xml text into a project. The project's name and pack aren't in the file; set them after.
	 * @return False when the text isn't a WED document.
	 */
	bool ParseWed(std::string_view xml, SceneryProject &out, std::vector<std::string> *warnings = nullptr, std::string *error = nullptr,
		const WedImportOptions &options = {});

	/* @brief Formats a project as earth.wed.xml text that WorldEditor opens. */
	std::string WriteWed(const SceneryProject &project, std::vector<std::string> *warnings = nullptr, const WedExportOptions &options = {});

	/**
	 * @brief WED's display name for a numbered library line or light: "30_red_line" gives "Red Line", "62_double_hold_wide_b"
	 * gives "Double Hold Wide (Black)", "107_centerline_twy_G_uni" gives "Centerline Twy (Unidirectional Green)".
	 * @param stem The file name without folder and extension.
	 * @param code Receives the line code (the leading number), 0 when there is none.
	 */
	std::string MakeWedLineName(std::string_view stem, int *code = nullptr);
}

// -------------------------------------------------------

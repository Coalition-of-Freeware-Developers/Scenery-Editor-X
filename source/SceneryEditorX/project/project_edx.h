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
 * project_edx.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "scenery_project.h"
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	/**
	 * .edX: the project as JSON text, written to sit well in a git repository:
	 *
	 *  - deterministic: the same project always produces the same bytes (keys in a fixed order, numbers in their
	 *    shortest round-trip form, LF line endings, a final newline), so saving without changes leaves no diff
	 *  - one record per line: a placed object, a node, a runway end... is one line, so an edit is a one-line diff and
	 *    merges of independent edits rarely conflict
	 *  - stable ids: items keep their id for life (references and diffs follow them across renames and moves)
	 *  - lenient reading: comments are allowed, missing keys take their defaults, a wrong type or an unknown key is a
	 *    warning rather than a failure (hand edits and merges stay openable); only broken JSON fails
	 *
	 * Also reads the early layout from "docs/edx File Format Documentation.md" (a single "Airport" object with
	 * ICAO / City / DatumLat... keys and "Libraries" inside it).
	 */

	/* @brief Formats a project as .edX text. */
	std::string WriteEdx(const SceneryProject &project);

	/**
	 * @brief Parses .edX text.
	 * @param warnings Receives what was ignored or repaired (optional).
	 * @return False when the text isn't JSON or isn't a project (error says why); out is then left empty.
	 */
	bool ParseEdx(std::string_view text, SceneryProject &out, std::vector<std::string> *warnings = nullptr, std::string *error = nullptr);
}

// -------------------------------------------------------

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
 * edit_changes.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "airport_edit.h"
#include <cstdint>
#include <string>
#include <vector>
#include <SceneryEditorX/project/project_document.h>
#include <SceneryEditorX/scene/components/component_sets.h>
#include <SceneryEditorX/xplane/xp_airport_document.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditChanges
{
	/**
	 * One edit made with the viewport tools, across the scene (entity transforms), the open airport (apt.dat rows)
	 * and the open project (items), kept as before / after copies so it can be undone and redone as one step.
	 */

	struct EntityChange
	{
		uint64_t entity = 0;
		TransformComponent before;
		TransformComponent after;
	};

	struct RowChange
	{
		XPlane::AirportElementKind kind = XPlane::AirportElementKind::Airport;
		uint32_t index = 0;
		AirportEdit::Snapshot before;
		AirportEdit::Snapshot after;
	};

	struct ItemChange
	{
		ProjectFile::SceneryItem before;	// Found again by its id
		ProjectFile::SceneryItem after;
	};

	struct ChangeSet
	{
		std::vector<EntityChange> entities;
		std::vector<RowChange> rows;
		std::vector<ItemChange> items;
		const XPlane::AirportDocument *airport = nullptr;	// The documents the rows / items belong to
		std::string airportId;
		const ProjectFile::ProjectDocument *project = nullptr;
		bool syncProject = false;							// Project objects moved: bring them into the project

		[[nodiscard]] bool IsEmpty() const { return entities.empty() && rows.empty() && items.empty(); }
	};

	/* @brief An empty change set bound to the airport and project open now. */
	ChangeSet Begin();

	/* @brief Tells the scene what changed: rows rebuilt (just those), items redrawn, project objects synced. */
	void Publish(const ChangeSet &changes);

	/* @brief Puts the before (or after) state back, then publishes. False when something it names is gone. */
	bool Apply(const ChangeSet &changes, bool after);

	/* @brief Records the (already applied) change set as one undo step. */
	void Record(std::string label, ChangeSet changes);
}

// -------------------------------------------------------

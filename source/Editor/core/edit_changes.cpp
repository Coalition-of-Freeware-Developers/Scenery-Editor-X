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
 * edit_changes.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "edit_changes.h"
#include "editor_airports.h"
#include "editor_history.h"
#include "editor_project.h"
#include "editor_scene.h"
#include <memory>
#include <SceneryEditorX/renderer/gpu_scene.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditChanges
{
	ChangeSet Begin()
	{
		ChangeSet changes;
		changes.airport = EditorAirports::GetDocument();
		changes.airportId = changes.airport ? changes.airport->airport.id : std::string();
		changes.project = EditorProjects::GetDocument();
		return changes;
	}

	void Publish(const ChangeSet &changes)
	{
		if (!changes.rows.empty())
		{
			std::vector<XPlane::SceneAirports::ElementRef> rebuilt;
			bool pointsMoved = false;
			for (const RowChange &change : changes.rows)
			{
				if (AirportEdit::IsPointKind(change.kind))
					pointsMoved = true;		// Their markers follow the rows without a rebuild
				else
					rebuilt.push_back({ change.kind, change.index });
			}
			if (!rebuilt.empty())
				EditorAirports::MarkElementsEdited(rebuilt);
			else if (pointsMoved)
				EditorAirports::MarkEdited(false);
		}

		if (!changes.items.empty())
		{
			if (ProjectFile::ProjectDocument *project = EditorProjects::GetDocument())
				project->Mark(ProjectFile::ProjectDocument::Section::Items);
		}

		if (changes.syncProject)
			EditorProjects::NotifySceneEdited();
	}

	bool Apply(const ChangeSet &changes, const bool after)
	{
		bool ok = true;
		for (const EntityChange &change : changes.entities)
		{
			const entt::entity handle = EditorScene::FindHandle(change.entity);
			TransformComponent *transform = EditorScene::TryGet<TransformComponent>(handle);
			if (!transform)
			{
				ok = false;
				continue;
			}
			*transform = after ? change.after : change.before;
			GpuScene::MarkTransformDirty(handle);
		}

		if (!changes.rows.empty())
		{
			XPlane::AirportDocument *airport = EditorAirports::GetDocument();
			if (!airport || airport != changes.airport || airport->airport.id != changes.airportId)
				ok = false;
			else
			{
				for (const RowChange &change : changes.rows)
					ok &= AirportEdit::Restore(airport->airport, change.kind, change.index, after ? change.after : change.before);
			}
		}

		if (!changes.items.empty())
		{
			ProjectFile::ProjectDocument *project = EditorProjects::GetDocument();
			if (!project || project != changes.project)
				ok = false;
			else
			{
				for (const ItemChange &change : changes.items)
				{
					ProjectFile::SceneryItem *item = project->FindItem(change.before.id);
					if (!item)
					{
						ok = false;
						continue;
					}
					*item = after ? change.after : change.before;
				}
			}
		}

		Publish(changes);
		return ok;
	}

	void Record(std::string label, ChangeSet changes)
	{
		if (changes.IsEmpty())
			return;
		EditorHistory::Command command;
		command.label = std::move(label);
		for (const RowChange &row : changes.rows)
			command.bytes += AirportEdit::SizeOf(row.before) + AirportEdit::SizeOf(row.after);
		for (const ItemChange &item : changes.items)
		{
			for (const ProjectFile::GeoRing &ring : item.before.rings)
				command.bytes += ring.size() * sizeof(ProjectFile::GeoNode) * 2;
			command.bytes += sizeof(ItemChange);
		}
		command.bytes += changes.entities.size() * sizeof(EntityChange);
		auto shared = std::make_shared<ChangeSet>(std::move(changes));
		command.undo = [shared]() { return Apply(*shared, false); };
		command.redo = [shared]() { return Apply(*shared, true); };
		EditorHistory::Push(std::move(command));
	}
}

// -------------------------------------------------------

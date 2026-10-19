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
 * editor_selection.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <SceneryEditorX/xplane/xp_airport_scene.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorContext;
}

namespace SceneryEditorX::EditorSelection
{
	/**
	 * The editor's selection: any number of targets, one of them active (EditorContext::selectedEntity, what the
	 * PROPERTIES panel shows). Targets name what was selected rather than which entity drew it, so a selection
	 * survives the rebuilds that replace entities: an apt.dat row by kind and index, a project item by its id, anything
	 * else by entity UUID. Panels that set EditorContext::selectedEntity directly are followed on the next Sync().
	 * Main thread only.
	 */

	enum class TargetType : uint8_t
	{
		Entity,				// A scene entity (and what is below it)
		AirportElement,		// An apt.dat row of the open airport
		ProjectItem			// An item of the open project (an object, polygon, line, forest...)
	};

	struct Target
	{
		TargetType type = TargetType::Entity;
		uint64_t entity = 0;				// Entity: its UUID; the others: the entity they were last found at
		XPlane::AirportElementKind kind = XPlane::AirportElementKind::Airport;
		uint32_t index = 0;
		std::string itemId;

		static Target ForEntity(uint64_t entity);
		static Target ForElement(XPlane::AirportElementKind kind, uint32_t index, uint64_t entity = 0);
		static Target ForItem(std::string itemId, uint64_t entity = 0);

		/* @brief Same thing selected (type and identity; the cached entity doesn't count). */
		[[nodiscard]] bool SameAs(const Target &other) const;
	};

	enum class Mode : uint8_t
	{
		Replace,
		Add,
		Toggle,
		Remove
	};

	/* @brief Once per frame before the panels: follows selections other panels made and drops targets that are gone. */
	void Sync(EditorContext *context);

	const std::vector<Target> &Get();

	/* @brief The active target (the last one selected), or nullptr. */
	const Target *GetActive();

	bool IsEmpty();
	bool IsSelected(const Target &target);

	void Select(EditorContext *context, const Target &target, Mode mode = Mode::Replace);
	void Select(EditorContext *context, const std::vector<Target> &targets, Mode mode = Mode::Replace);
	void Clear(EditorContext *context);

	/* @brief Moves whenever the selection changes. */
	uint64_t GetRevision();

	/**
	 * @brief What clicking a drawn entity selects: the project item or apt.dat row it belongs to (a light string's
	 * light objects select their pavement or line), the X-Plane object it is part of, or its top-level ancestor.
	 */
	std::optional<Target> FromPick(uint64_t entityId);

	/* @brief The entity itself as a target (an apt.dat row's or project item's own entity becomes that row / item). */
	std::optional<Target> FromEntity(uint64_t entityId);

	/* @brief The entity showing a target now, 0 when it is gone. */
	uint64_t Resolve(const Target &target);

	/* @brief The entities drawing a target (each with everything below it): an apt.dat row's parts and light objects. */
	void CollectEntities(const Target &target, std::vector<uint64_t> &out);

	/* @brief "Pavement 3: Apron", "hangar.obj", ... */
	std::string Describe(const Target &target);

	/**
	 * @brief True for what the tools can't change for good: the scenery pack's own DSF art around an airport (drawn
	 * from the pack, not from data the editor saves).
	 */
	bool IsReadOnly(const Target &target, std::string *reason = nullptr);

	/* @brief The target under the cursor (highlighted, not selected). */
	void SetHovered(const std::optional<Target> &target);
	const std::optional<Target> &GetHovered();

	/* @brief Once per frame after the panels: hands the selected / active / hovered entities to the outline pass. */
	void UpdateHighlight();
}

// -------------------------------------------------------

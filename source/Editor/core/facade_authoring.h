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
 * facade_authoring.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <SceneryEditorX/xplane/xp_facade_authoring.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorContext;
}

namespace SceneryEditorX::FacadeAuthoring
{
	/**
	 * The facade being made in the FACADE EDITOR panel: a type 1 building from a texture or a type 2 sidewalk / curb
	 * from a cross-section, saved as <project>/facades/<name>.fac (through the VFS, textures copied beside it). While
	 * "Live preview" is on, every change is saved shortly after it is made and the project's scenery redrawn, so a
	 * facade item using it (Place preview, or any drawn with the facade tool) shows the change in the viewport.
	 * Curbs keep their cross-section in <name>.fac.json beside the facade so they can be edited again.
	 */

	enum class Kind : uint8_t
	{
		None,
		Building,		// Type 1: texture panels
		Curb			// Type 2: generated from CurbFacadeSpec
	};

	Kind GetKind();

	/* @brief The facade as it will be written (a curb's is regenerated from its spec on every change). */
	XPlane::FacadeAsset &GetAsset();

	/* @brief The cross-section, textures and segments a curb is built from. */
	XPlane::CurbFacadeSpec &GetCurbSpec();

	/* @brief The file name (without .fac); setting it renames where the next save goes. */
	const std::string &GetName();
	void SetName(const std::string &name);

	/* @brief <project>/facades/<name>.fac (empty without a project). */
	std::filesystem::path GetFilePath();

	/* @brief What project items name the facade by: "facades/<name>.fac". */
	std::string GetResource();

	/* @brief A texture of the facade as a file (resolved beside the .fac; a .dds next to it wins, like X-Plane). */
	std::filesystem::path ResolveTexture(const std::string &reference);

	/**
	 * @brief Copies a texture into the facades folder (unless it is already inside the project) and returns the path
	 * the facade names it by. Empty with an error message when it can't be copied.
	 */
	std::string ImportTexture(const std::filesystem::path &file, std::string *error = nullptr);

	/* @brief Starts a type 1 building from a texture (imported into the project). */
	bool NewBuilding(const std::filesystem::path &texture, std::string *error = nullptr);

	/* @brief Starts a sidewalk / curb from a cross-section preset (textures can be set afterwards). */
	bool NewCurb(const XPlane::CurbProfile &profile, std::string *error = nullptr);

	/**
	 * @brief Edits a facade: one in the project's facades folder in place (a curb with its saved spec), any other type 1
	 * facade (the X-Plane library's, another pack's) as a copy in the facades folder with its textures copied along.
	 */
	bool Edit(const std::string &resource, std::string *error = nullptr);

	/* @brief True when Edit() can open the facade (type 1 anywhere, type 2 only curbs made here). */
	bool CanEdit(const std::string &resource);

	/* @brief Call after changing GetAsset() / GetCurbSpec(): regenerates a curb, checks it, schedules the live preview. */
	void MarkEdited();

	bool IsDirty();

	/* @brief Problems that keep the facade from working (CheckFacadeAsset), refreshed by MarkEdited(). */
	const std::vector<std::string> &GetProblems();

	/* @brief Writes the facade (and a curb's spec) to GetFilePath(), then redraws the project's scenery with it. */
	bool Save(std::string *error = nullptr);

	/* @brief Saves the facade from disk again, dropping unsaved changes. */
	bool Revert(std::string *error = nullptr);

	/* @brief Saves the facade, then drops a facade item using it where the viewport looks (one undo step); its id. */
	std::string PlacePreview(EditorContext *context);

	/* @brief The item PlacePreview() added (empty when none, or when it was removed). */
	const std::string &GetPreviewItem();

	/* @brief Saves and redraws a short while after the last change (MarkEdited) while on. */
	bool &LiveSave();

	/* @brief Once per frame: the live preview's delayed save. */
	void Tick();

	/* @brief The facades in the project's facades folder (resources, sorted). */
	std::vector<std::string> GetProjectFacades();

	/* @brief Asks the editor layer to show the FACADE EDITOR panel. */
	void RequestOpen();
	bool ConsumeOpenRequest();

	/* @brief Asks for a texture file (png / dds / jpg ...); empty when cancelled. */
	std::filesystem::path BrowseTexture(const char *title);

	/* @brief The commands (EditorCommands): facade.author.building / .curb / .edit / .open. */
	void RegisterCommands();
}

// -------------------------------------------------------

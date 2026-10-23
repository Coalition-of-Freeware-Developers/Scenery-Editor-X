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
 * facade_authoring.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "facade_authoring.h"
#include "asset_thumbnails.h"
#include "editor_commands.h"
#include "editor_project.h"
#include "editor_tools.h"
#include "facade_tool.h"
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>
#include <portable-file-dialogs.h>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui_internal.h>
#include <SceneryEditorX/xplane/xp_facade_writer.h>
#include <SceneryEditorX/xplane/xp_texture_loader.h>
#include <SceneryEditorX/xplane/xp_text.h>

// -------------------------------------------------------

namespace SceneryEditorX::FacadeAuthoring
{
	namespace fs = std::filesystem;
	using VFS::FromUtf8;
	using VFS::ToUtf8;

	namespace
	{
		constexpr const char *LOG_TAG = "Facades";
		constexpr const char *FOLDER = "facades";
		constexpr const char *SPEC_EXTENSION = ".fac.json";
		constexpr double LIVE_DELAY = 0.35;	// Seconds after the last change the live preview saves
		constexpr size_t UNDO_STEPS = 100;	// Undo steps kept per facade

		/**
		 * The facade as an undo step keeps it: a curb's spec (its asset is built from it), else the asset without its
		 * type 2 segment meshes (they aren't edited here, so a step doesn't copy them).
		 */
		struct Snapshot
		{
			XPlane::FacadeAsset asset;
			XPlane::CurbFacadeSpec curb;
		};

		struct State
		{
			Kind kind = Kind::None;
			XPlane::FacadeAsset asset;
			XPlane::CurbFacadeSpec curb;
			std::string name;
			bool dirty = false;
			bool live = true;
			double saveAt = -1.0;			// ImGui time of the live preview's next save, -1 when none is due
			std::vector<std::string> problems;
			std::string previewItem;
			bool openRequested = false;

			std::vector<Snapshot> undo;		// Oldest first
			std::vector<Snapshot> redo;		// Most recently undone last
			Snapshot committed;				// The facade as of the last edit (what the next one is undone to)
			ImGuiID held = 0;				// The widget held when the current step started (0: none); its later changes join that step
		};

		State &S()
		{
			static State state;
			return state;
		}

		fs::path FacadesFolder()
		{
			const fs::path project = EditorProjects::GetProjectFolder();
			return project.empty() ? fs::path() : project / FOLDER;
		}

		bool Fail(std::string *error, const std::string &message)
		{
			if (error)
				*error = message;
			EDITOR_WARN_TAG(LOG_TAG, "{}", message);
			return false;
		}

		bool IsInside(const fs::path &file, const fs::path &folder)
		{
			if (folder.empty())
				return false;
			const fs::path relative = file.lexically_normal().lexically_relative(folder.lexically_normal());
			return !relative.empty() && *relative.begin() != "..";
		}

		/* @brief A facade name no file in the facades folder has yet ("name", "name_2", ...). */
		std::string UniqueName(const std::string &base)
		{
			const std::string clean = XPlane::MakeFacadeFileName(base);
			const fs::path folder = FacadesFolder();
			std::string name = clean;
			for (int n = 2; !folder.empty() && fs::exists(folder / (name + ".fac")) && n < 1000; ++n)
				name = clean + "_" + std::to_string(n);
			return name;
		}

		fs::path SpecPath()
		{
			const fs::path file = GetFilePath();
			return file.empty() ? fs::path() : fs::path(file).replace_extension(SPEC_EXTENSION);
		}

		nlohmann::json SpecToJson(const XPlane::CurbFacadeSpec &spec)
		{
			nlohmann::json profile = nlohmann::json::array();
			for (const auto &[out, up] : spec.profile)
				profile.push_back({ out, up });
			return {
				{ "version", 1 }, { "kind", "curb" }, { "wallName", spec.wallName }, { "profile", profile },
				{ "wallTexture", spec.wallTexture }, { "wallTextureNormal", spec.wallTextureNormal },
				{ "textureS1", spec.textureS1 }, { "textureS2", spec.textureS2 }, { "textureT1", spec.textureT1 }, { "textureT2", spec.textureT2 },
				{ "textureLength", spec.textureLength }, { "roofTexture", spec.roofTexture }, { "roofTextureNormal", spec.roofTextureNormal },
				{ "roofScale", spec.roofScale }, { "hardRoof", spec.hardRoof }, { "segmentLengths", spec.segmentLengths },
				{ "spellingLength", spec.spellingLength }, { "curvedPiece", spec.curvedPiece }, { "lodFar", spec.lodFar }, { "ring", spec.ring } };
		}

		bool SpecFromJson(const nlohmann::json &json, XPlane::CurbFacadeSpec &spec)
		{
			if (!json.is_object() || json.value("kind", "") != "curb")
				return false;
			XPlane::CurbFacadeSpec out;
			out.wallName = json.value("wallName", out.wallName);
			if (const auto profile = json.find("profile"); profile != json.end() && profile->is_array())
			{
				for (const nlohmann::json &point : *profile)
				{
					if (point.is_array() && point.size() == 2 && point[0].is_number() && point[1].is_number())
						out.profile.emplace_back(point[0].get<float>(), point[1].get<float>());
				}
			}
			out.wallTexture = json.value("wallTexture", out.wallTexture);
			out.wallTextureNormal = json.value("wallTextureNormal", out.wallTextureNormal);
			out.textureS1 = json.value("textureS1", out.textureS1);
			out.textureS2 = json.value("textureS2", out.textureS2);
			out.textureT1 = json.value("textureT1", out.textureT1);
			out.textureT2 = json.value("textureT2", out.textureT2);
			out.textureLength = json.value("textureLength", out.textureLength);
			out.roofTexture = json.value("roofTexture", out.roofTexture);
			out.roofTextureNormal = json.value("roofTextureNormal", out.roofTextureNormal);
			out.roofScale = json.value("roofScale", out.roofScale);
			out.hardRoof = json.value("hardRoof", out.hardRoof);
			if (const auto lengths = json.find("segmentLengths"); lengths != json.end() && lengths->is_array())
			{
				out.segmentLengths.clear();
				for (const nlohmann::json &length : *lengths)
				{
					if (length.is_number())
						out.segmentLengths.push_back(length.get<float>());
				}
			}
			out.spellingLength = json.value("spellingLength", out.spellingLength);
			out.curvedPiece = json.value("curvedPiece", out.curvedPiece);
			out.lodFar = json.value("lodFar", out.lodFar);
			out.ring = json.value("ring", out.ring);
			if (out.profile.size() < 2)
				return false;
			spec = std::move(out);
			return true;
		}

		/* @brief After a change (or an undo): a curb rebuilt from its spec, the problems checked, the live preview's save scheduled. */
		void Refresh()
		{
			State &s = S();
			if (s.kind == Kind::Curb)
				s.asset = XPlane::BuildCurbFacade(s.curb);
			s.problems = s.kind == Kind::None ? std::vector<std::string>() : XPlane::CheckFacadeAsset(s.asset);
			s.dirty = true;
			// Live: saved (and redrawn) shortly after the last change, once the facade has a file
			if (s.live && s.kind != Kind::None && !GetFilePath().empty() && fs::exists(GetFilePath()))
				s.saveAt = ImGui::GetTime() + LIVE_DELAY;
		}

		Snapshot Capture()
		{
			State &s = S();
			Snapshot snapshot;
			if (s.kind == Kind::Curb)
			{
				snapshot.curb = s.curb;
				return snapshot;
			}
			// The segment meshes are moved aside while the rest is copied
			std::vector<std::pair<std::vector<XPlane::FacadeAsset::Segment>, std::vector<XPlane::FacadeAsset::Segment>>> meshes;
			for (XPlane::FacadeAsset::Floor &floor : s.asset.floors)
				meshes.emplace_back(std::move(floor.segments), std::move(floor.curvedSegments));
			snapshot.asset = s.asset;
			for (size_t i = 0; i < meshes.size(); ++i)
			{
				s.asset.floors[i].segments = std::move(meshes[i].first);
				s.asset.floors[i].curvedSegments = std::move(meshes[i].second);
			}
			return snapshot;
		}

		/* @brief What a step changes, as text (two steps are the same facade when their texts are). */
		std::string Serialize(const Snapshot &snapshot)
		{
			return S().kind == Kind::Curb ? SpecToJson(snapshot.curb).dump() : XPlane::WriteFacadeAsset(snapshot.asset);
		}

		/* @brief Goes back (or forward) to a step: the current segment meshes stay (floors aren't added or removed here). */
		void Restore(Snapshot snapshot)
		{
			State &s = S();
			if (s.kind == Kind::Curb)
				s.curb = std::move(snapshot.curb);
			else
			{
				for (size_t i = 0; i < snapshot.asset.floors.size() && i < s.asset.floors.size(); ++i)
				{
					snapshot.asset.floors[i].segments = std::move(s.asset.floors[i].segments);
					snapshot.asset.floors[i].curvedSegments = std::move(s.asset.floors[i].curvedSegments);
				}
				s.asset = std::move(snapshot.asset);
			}
			Refresh();
			s.committed = Capture();
			s.held = 0;
		}

		/* @brief Ends the step a held widget started: dropped when letting go left the facade as it was. */
		void CloseStep()
		{
			State &s = S();
			if (s.held == 0)
				return;
			if (!s.undo.empty() && Serialize(s.undo.back()) == Serialize(s.committed))
				s.undo.pop_back();
			else
				s.redo.clear();
			s.held = 0;
		}

		/* @brief Starts editing an asset: what the panel shows, unsaved until Save(), with a fresh undo history. */
		void Begin(const Kind kind, XPlane::FacadeAsset asset, const std::string &name, const bool dirty)
		{
			State &s = S();
			s.kind = kind;
			s.asset = std::move(asset);
			s.name = name;
			s.previewItem.clear();
			Refresh();
			s.saveAt = -1.0;	// Nothing changed yet
			s.dirty = dirty;
			s.undo.clear();
			s.redo.clear();
			s.held = 0;
			s.committed = Capture();
		}

		/* @brief The facade the menus mean: the selected facade item's, else the one armed in the facade tool. */
		std::string TargetResource()
		{
			const std::vector<std::string> selected = FacadeTool::GetSelectedFacades();
			ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
			if (const ProjectFile::SceneryItem *item = !selected.empty() && document ? document->FindItem(selected.front()) : nullptr)
				return item->resource;
			return FacadeTool::GetSettings().resource;
		}
	}

	// -------------------------------------------------------

	Kind GetKind()
	{
		return S().kind;
	}

	XPlane::FacadeAsset &GetAsset()
	{
		return S().asset;
	}

	XPlane::CurbFacadeSpec &GetCurbSpec()
	{
		return S().curb;
	}

	const std::string &GetName()
	{
		return S().name;
	}

	void SetName(const std::string &name)
	{
		State &s = S();
		const std::string clean = XPlane::MakeFacadeFileName(name);
		if (clean == s.name)
			return;
		s.name = clean;
		s.dirty = true;
		s.previewItem.clear();	// The preview names the old file
	}

	fs::path GetFilePath()
	{
		const fs::path folder = FacadesFolder();
		return folder.empty() || S().name.empty() ? fs::path() : folder / (S().name + ".fac");
	}

	std::string GetResource()
	{
		return S().name.empty() ? std::string() : std::string(FOLDER) + "/" + S().name + ".fac";
	}

	fs::path ResolveTexture(const std::string &reference)
	{
		const fs::path folder = FacadesFolder();
		if (reference.empty() || folder.empty())
			return {};
		return XPlane::TextureLoader::ResolveTexturePath(folder, reference);
	}

	std::string ImportTexture(const fs::path &file, std::string *error)
	{
		const fs::path folder = FacadesFolder();
		if (folder.empty())
		{
			Fail(error, "Open a project first: facades are made in its facades folder");
			return {};
		}
		if (!fs::exists(file))
		{
			Fail(error, "Texture not found: " + ToUtf8(file));
			return {};
		}
		// Inside the project already: named where it is
		if (IsInside(file, EditorProjects::GetProjectFolder()))
			return XPlane::MakeFacadeRelativePath(folder / "facade.fac", file);

		// Else copied beside the facades (an identical copy there is reused)
		fs::path target = folder / file.filename();
		std::error_code ec;
		for (int n = 2; fs::exists(target) && fs::file_size(target, ec) != fs::file_size(file, ec) && n < 1000; ++n)
			target = folder / (ToUtf8(file.stem()) + "_" + std::to_string(n) + ToUtf8(file.extension()));
		if (!fs::exists(target))
		{
			std::string copyError;
			if (!VFS::CopyRegularFile(file, target, false, &copyError))
			{
				Fail(error, "Could not copy " + ToUtf8(file.filename()) + ": " + copyError);
				return {};
			}
			EDITOR_INFO_TAG(LOG_TAG, "Copied texture {} into {}", ToUtf8(file), ToUtf8(folder));
		}
		return ToUtf8(target.filename());
	}

	bool NewBuilding(const fs::path &texture, std::string *error)
	{
		const std::string reference = ImportTexture(texture, error);
		if (reference.empty())
			return false;
		int width = 0, height = 0;
		const fs::path file = ResolveTexture(reference);
		if (!XPlane::ReadTextureSize(file, width, height))
		{
			// JPG, TGA...: the loader knows
			XPlane::TextureInfo info;
			XPlane::TextureLoader::Load(file, XPlane::TextureUsage::Albedo, &info);
			width = static_cast<int>(info.width);
			height = static_cast<int>(info.height);
		}
		if (width <= 0 || height <= 0)
			return Fail(error, "Can't read the size of " + ToUtf8(texture.filename()));

		// About 40 pixels per meter (X-Plane's own facade art), at most a 30 m building front (atlases are marked up afterwards)
		const float metersAcross = std::clamp(static_cast<float>(width) / 40.0f, 2.0f, 30.0f);
		Begin(Kind::Building, XPlane::MakeType1Facade(reference, width, height, metersAcross), UniqueName(ToUtf8(texture.stem())), true);
		EDITOR_INFO_TAG(LOG_TAG, "New building facade {} from {} ({} x {} px)", S().name, reference, width, height);
		return true;
	}

	bool NewCurb(const XPlane::CurbProfile &profile, std::string *error)
	{
		if (FacadesFolder().empty())
			return Fail(error, "Open a project first: facades are made in its facades folder");
		State &s = S();
		s.curb = XPlane::CurbFacadeSpec();
		s.curb.profile = profile.points;
		Begin(Kind::Curb, XPlane::FacadeAsset(), UniqueName(profile.name.empty() ? std::string("curb") : profile.name), true);
		EDITOR_INFO_TAG(LOG_TAG, "New sidewalk / curb facade {} ({})", s.name, profile.name);
		return true;
	}

	bool CanEdit(const std::string &resource)
	{
		return !resource.empty() && !FacadesFolder().empty() && EditorProjects::GetFacade(resource) != nullptr;
	}

	bool Edit(const std::string &resource, std::string *error)
	{
		const fs::path folder = FacadesFolder();
		if (folder.empty())
			return Fail(error, "Open a project first: facades are made in its facades folder");
		const fs::path file = EditorProjects::ResolveResourceFile(resource);
		XPlane::FacadeAsset asset;
		std::string loadError;
		if (file.empty() || !XPlane::LoadFacadeAsset(file, asset, &loadError))
			return Fail(error, file.empty() ? "Facade not found: " + resource : loadError);

		// A facade of this project's facades folder: edited in place
		if (IsInside(file, folder))
		{
			const std::string name = ToUtf8(file.stem());
			if (asset.IsType2())
			{
				// A curb made here comes back from its spec; any other type 2 facade is edited as written
				const fs::path specFile = fs::path(file).replace_extension(SPEC_EXTENSION);
				std::string text;
				XPlane::CurbFacadeSpec spec;
				nlohmann::json json;
				if (fs::exists(specFile) && XPlane::ReadTextFile(specFile, text) && (json = nlohmann::json::parse(text, nullptr, false)).is_object() &&
					SpecFromJson(json, spec))
				{
					S().curb = spec;
					Begin(Kind::Curb, std::move(asset), name, false);
				}
				else
				{
					if (fs::exists(specFile))
						EDITOR_WARN_TAG(LOG_TAG, "Can't read the curb spec {}: editing {} as written", ToUtf8(specFile), resource);
					Begin(Kind::Type2, std::move(asset), name, false);
				}
			}
			else
				Begin(Kind::Building, std::move(asset), name, false);
			EDITOR_INFO_TAG(LOG_TAG, "Editing {}", resource);
			return true;
		}

		// Any other facade: a copy, its textures (and a type 2's OBJs with their textures) copied along
		XPlane::FacadeFileCopy copied;
		std::string copyError;
		if (!XPlane::CopyFacadeFiles(asset, file, folder, &copied, &copyError))
			return Fail(error, "Could not copy " + resource + "'s files into " + ToUtf8(folder) + ": " + copyError);
		for (const std::string &kept : copied.kept)
			EDITOR_INFO_TAG(LOG_TAG, "{}: {} kept as named (a library path, or not found beside it)", resource, kept);
		const std::string base = resource.substr(resource.find_last_of('/') == std::string::npos ? 0 : resource.find_last_of('/') + 1);
		const Kind kind = asset.IsType2() ? Kind::Type2 : Kind::Building;
		Begin(kind, std::move(asset), UniqueName(base.substr(0, base.rfind('.')) + "_copy"), true);
		EDITOR_INFO_TAG(LOG_TAG, "Editing a copy of {} as {} ({} files copied into {})", resource, GetResource(), copied.files.size(), ToUtf8(folder));
		return true;
	}

	void MarkEdited()
	{
		State &s = S();
		if (s.kind != Kind::None)
		{
			// One undo step per change; while one widget stays held (a drag, typing) its changes join the step it started
			const ImGuiID held = ImGui::GetActiveID();
			Snapshot now = Capture();
			if (held == 0 || held != s.held)
			{
				CloseStep();
				if (held != 0 || Serialize(now) != Serialize(s.committed))
				{
					s.undo.push_back(std::move(s.committed));
					if (s.undo.size() > UNDO_STEPS)
						s.undo.erase(s.undo.begin());
					if (held == 0)
						s.redo.clear();	// A held step clears it when let go, unless it changed nothing
				}
				s.held = held;
			}
			s.committed = std::move(now);
		}
		Refresh();
	}

	bool CanUndo()
	{
		return S().kind != Kind::None && !S().undo.empty();
	}

	bool CanRedo()
	{
		return S().kind != Kind::None && !S().redo.empty();
	}

	bool Undo()
	{
		State &s = S();
		CloseStep();
		if (!CanUndo())
			return false;
		s.redo.push_back(std::move(s.committed));
		Snapshot previous = std::move(s.undo.back());
		s.undo.pop_back();
		Restore(std::move(previous));
		return true;
	}

	bool Redo()
	{
		State &s = S();
		CloseStep();
		if (!CanRedo())
			return false;
		s.undo.push_back(std::move(s.committed));
		Snapshot next = std::move(s.redo.back());
		s.redo.pop_back();
		Restore(std::move(next));
		return true;
	}

	bool IsDirty()
	{
		return S().dirty;
	}

	const std::vector<std::string> &GetProblems()
	{
		return S().problems;
	}

	bool Save(std::string *error)
	{
		State &s = S();
		s.saveAt = -1.0;
		if (s.kind == Kind::None)
			return Fail(error, "No facade to save");
		const fs::path path = GetFilePath();
		if (path.empty())
			return Fail(error, "Open a project first: facades are saved in its facades folder");
		std::string writeError;
		if (!XPlane::SaveFacadeAsset(path, s.asset, &writeError))
			return Fail(error, "Could not save " + ToUtf8(path) + ": " + writeError);
		if (s.kind == Kind::Curb && !VFS::WriteFileAtomic(SpecPath(), SpecToJson(s.curb).dump(1, '\t'), &writeError))
			return Fail(error, "Could not save " + ToUtf8(SpecPath()) + ": " + writeError);
		s.dirty = false;
		EDITOR_INFO_TAG(LOG_TAG, "Saved {}", ToUtf8(path));
		// Every item using it is drawn again from the new file, and so is its thumbnail
		EditorProjects::ReloadArt();
		AssetThumbnails::Invalidate(path);
		return true;
	}

	bool Revert(std::string *error)
	{
		const fs::path path = GetFilePath();
		if (path.empty() || !fs::exists(path))
			return Fail(error, "The facade hasn't been saved yet");
		const std::string preview = S().previewItem;
		const bool done = Edit(GetResource(), error);
		S().previewItem = preview;
		return done;
	}

	std::string PlacePreview(EditorContext *context)
	{
		std::string error;
		if (!EditorProjects::GetDocument() || ((S().dirty || !fs::exists(GetFilePath())) && !Save(&error)))
			return {};
		float position[3] = { 0.0f, 0.0f, 0.0f };
		const bool ground = EditorTools::GetViewCenterOnGround(position);
		const std::string id = FacadeTool::Drop(context, GetResource(), ground ? position : nullptr);
		S().previewItem = id;
		return id;
	}

	const std::string &GetPreviewItem()
	{
		State &s = S();
		ProjectFile::ProjectDocument *document = EditorProjects::GetDocument();
		if (!s.previewItem.empty() && (!document || !document->FindItem(s.previewItem)))
			s.previewItem.clear();
		return s.previewItem;
	}

	bool &LiveSave()
	{
		return S().live;
	}

	void Tick()
	{
		State &s = S();
		// A widget let go closes its undo step
		if (s.held != 0 && ImGui::GetActiveID() != s.held)
			CloseStep();
		if (s.saveAt >= 0.0 && ImGui::GetTime() >= s.saveAt)
			Save();
	}

	std::vector<std::string> GetProjectFacades()
	{
		std::vector<std::string> out;
		const fs::path folder = FacadesFolder();
		std::error_code ec;
		if (folder.empty() || !fs::is_directory(folder, ec))
			return out;
		for (const fs::directory_entry &entry : fs::directory_iterator(folder, ec))
		{
			if (entry.is_regular_file(ec) && XPlane::ToLower(ToUtf8(entry.path().extension())) == ".fac")
				out.push_back(std::string(FOLDER) + "/" + ToUtf8(entry.path().filename()));
		}
		std::ranges::sort(out);
		return out;
	}

	void RequestOpen()
	{
		S().openRequested = true;
	}

	bool ConsumeOpenRequest()
	{
		const bool requested = S().openRequested;
		S().openRequested = false;
		return requested;
	}

	fs::path BrowseTexture(const char *title)
	{
		const fs::path start = FacadesFolder().empty() ? EditorProjects::GetProjectFolder() : FacadesFolder();
		const std::vector<std::string> files = pfd::open_file(title, ToUtf8(fs::exists(start) ? start : EditorProjects::GetProjectFolder()),
			{ "Textures (*.png *.dds *.jpg *.tga *.bmp)", "*.png *.dds *.jpg *.jpeg *.tga *.bmp", "All files", "*" }).result();
		return files.empty() ? fs::path() : FromUtf8(files.front());
	}

	void RegisterCommands()
	{
		const auto hasProject = [](EditorContext *) { return EditorProjects::GetDocument() != nullptr; };
		EditorCommands::Register({ "facade.author.building", "New Building Facade...", SEDX_ICON_PICTURE_O, "",
			"A type 1 facade from a texture: mark its panels across and floors up, preview it on a footprint, save it into the project's facades folder",
			hasProject, nullptr,
			[](EditorContext *)
			{
				const fs::path texture = BrowseTexture("Choose the facade's wall texture");
				if (!texture.empty() && NewBuilding(texture))
					RequestOpen();
			} });
		EditorCommands::Register({ "facade.author.curb", "New Sidewalk / Curb Facade", SEDX_ICON_MAGIC, "",
			"A type 2 ground facade generated from a cross-section: a 3D sidewalk, curb, planter or low wall (curved variants included)",
			hasProject, nullptr,
			[](EditorContext *)
			{
				if (NewCurb(XPlane::GetCurbProfilePresets().front()))
					RequestOpen();
			} });
		EditorCommands::Register({ "facade.author.edit", "Edit Facade", SEDX_ICON_PENCIL, "",
			"Open the selected (or armed) facade in the FACADE EDITOR: this project's own in place, a library or other pack's as a copy (its textures, and a type 2's objects, copied along)",
			[](EditorContext *) { return CanEdit(TargetResource()); }, nullptr,
			[](EditorContext *)
			{
				if (Edit(TargetResource()))
					RequestOpen();
			} });
		EditorCommands::Register({ "facade.author.open", "Facade Editor", SEDX_ICON_BUILDING_O, "", "Make new facades: buildings from a texture, sidewalks and curbs from a cross-section",
			nullptr, nullptr, [](EditorContext *) { RequestOpen(); } });
	}
}

// -------------------------------------------------------

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
 * editor_project.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "editor_project.h"
#include "editor_airports.h"
#include "editor_context.h"
#include "editor_scene.h"
#include "editor_ui.h"
#include "project_dialogs.h"
#include <cmath>
#include <ctime>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <portable-file-dialogs.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/platform/platform_context.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/filesystem/file_system.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/project/project_dsf.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/xplane/xp_airport_scene.h>
#include <SceneryEditorX/xplane/xp_library_manager.h>
#include <SceneryEditorX/xplane/xp_overlay.h>
#include <SceneryEditorX/xplane/xp_overlay_scene.h>
#include <SceneryEditorX/xplane/xp_scene.h>
#include <SceneryEditorX/xplane/xp_text.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorProjects
{
	namespace fs = std::filesystem;
	using namespace ProjectFile;
	using Section = ProjectDocument::Section;

	namespace
	{
		constexpr const char *LOG_TAG = "Project";
		constexpr const char *UNSAVED_MODAL = "Unsaved project changes###ProjectUnsaved";
		constexpr const char *RECOVERY_MODAL = "Recover unsaved work###ProjectRecovery";
		constexpr double SYNC_SECONDS = 2.0;			// How often scene edits (moved / deleted / placed objects) flow into the project
		constexpr double DEFAULT_AUTOSAVE_SECONDS = 60.0;
		constexpr double DEGREES_PER_RADIAN = 57.29577951308232;
		constexpr size_t MAX_LOGGED_WARNINGS = 25;

		/* @brief What to do once the unsaved-changes question is answered. */
		enum class Pending : uint8_t
		{
			None,
			Close,
			Open,
			New,
			NewEmpty,
			ImportWed,
			Quit
		};

		struct State
		{
			std::unique_ptr<ProjectCache> cache;
			ProjectDocument document;
			Ref<UserPreferences> preferences;

			// The project in the scene
			std::unordered_map<std::string, uint64_t> placed;	// Object item id -> root entity UUID
			std::unordered_set<uint64_t> placedRoots;
			std::vector<std::string> waiting;					// Object items whose library asset needs the library index
			XPlane::GeoProjection projection;					// Frame the objects were placed in
			double datumElevation = 0.0;						// Meters, for MSL elevations
			std::string airportId;								// Project airport open in the airport editor
			std::set<std::string> warned;						// Resources already reported as missing / unsupported

			// The other placed items (polygons, lines, strings, forests, facades, autogen) as a DSF through the overlay builder
			XPlane::OverlayScene scenery;
			uint64_t sceneryRevision = ~0ull;					// Items revision it was built from
			uint64_t sceneryLibrary = 0;						// Library index generation its assets were resolved with
			uint64_t sceneryFingerprint = 0;					// GetDsfFingerprint of what it draws

			double lastSync = 0.0;
			double lastAutosave = 0.0;
			double autosaveSeconds = DEFAULT_AUTOSAVE_SECONDS;
			std::string windowTitle;
			fs::path suggestedPath;								// Where Save As starts for a project without a file (an import)

			std::vector<std::string> validation;
			uint64_t validationRevision = ~0ull;

			// Modals
			Pending pending = Pending::None;
			fs::path pendingPath;
			bool openUnsaved = false;
			bool openRecovery = false;
			std::optional<RecoveryInfo> recovery;

			fs::path startupProject;							// --project <file>
			bool startupDone = false;
		};

		State &GetState()
		{
			static State state;
			return state;
		}

		// Paths ----------------------------------------------------------

		fs::path FromUtf8(const std::string &text)
		{
			return fs::path(std::u8string(text.begin(), text.end()));
		}

		std::string ToUtf8(const fs::path &path)
		{
			const std::u8string text = path.u8string();
			return std::string(text.begin(), text.end());
		}

		fs::path ProjectFolder()
		{
			const State &state = GetState();
			if (state.document.HasFile())
				return state.document.GetPath().parent_path();
			return state.suggestedPath.empty() ? fs::path() : state.suggestedPath.parent_path();
		}

		fs::path DefaultFolder()
		{
			if (const fs::path folder = ProjectFolder(); !folder.empty())
				return folder;
			if (const char *profile = std::getenv("USERPROFILE"))
				return fs::path(profile) / "Documents";
			if (const char *home = std::getenv("HOME"))
				return fs::path(home);
			return fs::current_path();
		}

		/* @brief A path ending in the extension a format needs (the save dialog doesn't add it everywhere). */
		fs::path WithExtension(fs::path path, const char *extension)
		{
			if (GetFormatForPath(path) == ProjectFormat::Unknown)
				path += extension;
			return path;
		}

		bool LibraryReady()
		{
			return XPlane::LibraryManager::GetGeneration() > 0 || XPlane::LibraryManager::GetScanState() == XPlane::ScanState::Failed ||
				XPlane::LibraryManager::GetInstallPath().empty();
		}

		/* @brief The file behind an item's resource: an absolute path, a path in the project folder, or a library virtual path. */
		fs::path ResolveResource(const std::string &resource)
		{
			std::error_code ec;
			const fs::path direct = FromUtf8(resource);
			if (direct.is_absolute())
				return fs::exists(direct, ec) ? direct : fs::path();
			if (const fs::path folder = ProjectFolder(); !folder.empty())
			{
				const fs::path local = folder / XPlane::ToNativeRelativePath(resource);
				if (fs::exists(local, ec))
					return local;
			}
			return XPlane::LibraryManager::Resolve(resource);
		}

		/* @brief The resource to store for a file placed from disk: relative to the project folder when inside it. */
		std::string ResourceForFile(const fs::path &file)
		{
			const fs::path folder = ProjectFolder();
			if (!folder.empty())
			{
				std::error_code ec;
				const fs::path relative = fs::relative(file, folder, ec);
				const std::string text = ToUtf8(fs::path(relative.generic_u8string()));
				if (!ec && !text.empty() && text.rfind("..", 0) != 0)
					return text;
			}
			return ToUtf8(fs::path(file.generic_u8string()));
		}

		void WarnOnce(const std::string &key, const std::string &message)
		{
			State &state = GetState();
			if (state.warned.insert(key).second)
				EDITOR_WARN_TAG(LOG_TAG, "{}", message);
		}

		void LogWarnings(const std::vector<std::string> &warnings, const char *what)
		{
			for (size_t i = 0; i < warnings.size() && i < MAX_LOGGED_WARNINGS; ++i)
				EDITOR_WARN_TAG(LOG_TAG, "{}: {}", what, warnings[i]);
			if (warnings.size() > MAX_LOGGED_WARNINGS)
				EDITOR_WARN_TAG(LOG_TAG, "{}: {} more warnings", what, warnings.size() - MAX_LOGGED_WARNINGS);
		}

		void AddRecent(const fs::path &path)
		{
			State &state = GetState();
			if (!state.preferences || path.empty())
				return;
			RecentProject recent;
			recent.name = state.document.GetProject().info.name;
			recent.filePath = ToUtf8(path);
			recent.lastOpened = std::time(nullptr);
			state.preferences->AddRecentProject(recent);
			state.preferences->SavePreferences();
		}

		/* @brief WED's names for the library's numbered lines and lights ("Red Line"), so WED files using them import. */
		std::vector<std::pair<std::string, int>> GetLibraryLineNames()
		{
			std::vector<std::pair<std::string, int>> names;
			const std::shared_ptr<const XPlane::LibraryIndex> index = XPlane::LibraryManager::GetIndex();
			if (!index)
				return names;
			for (const XPlane::LibraryAsset &asset : index->GetAssets())
			{
				const std::string_view path = asset.virtualPath;
				for (const std::string_view prefix : { std::string_view("lib/airport/lines/"), std::string_view("lib/airport/lights/slow/") })
				{
					if (path.rfind(prefix, 0) != 0)
						continue;
					std::string stem(path.substr(prefix.size()));
					stem = stem.substr(0, stem.find_last_of('.'));
					int code = 0;
					std::string name = MakeWedLineName(stem, &code);
					if (code > 0)
						names.emplace_back(std::move(name), code);
				}
			}
			return names;
		}

		LoadOptions MakeLoadOptions()
		{
			LoadOptions options;
			options.wed.extraLineNames = GetLibraryLineNames();
			return options;
		}

		// Scene ----------------------------------------------------------

		float HeightOf(const SceneryItem &item)
		{
			switch (item.elevationMode)
			{
				case ElevationMode::Agl:	return static_cast<float>(item.altitude);
				case ElevationMode::Msl:	return static_cast<float>(item.altitude - GetState().datumElevation);
				case ElevationMode::Ground:	break;
			}
			return 0.0f;
		}

		/* @brief Heading (clockwise from north) of a rotation, in the convention SceneObjects::Place turns objects with. */
		float HeadingOf(const Quat &rotation)
		{
			const Vec3 north(0.0f, 0.0f, -1.0f);
			static const Vec3 east = Quat::EulerRadians(Vec3(0.0f, -static_cast<float>(90.0 / DEGREES_PER_RADIAN), 0.0f)) * north;
			const Vec3 forward = rotation * north;
			const double x = forward.x * east.x + forward.z * east.z;
			const double y = forward.x * north.x + forward.z * north.z;
			double heading = std::atan2(x, y) * DEGREES_PER_RADIAN;
			if (heading < 0.0)
				heading += 360.0;
			return static_cast<float>(heading >= 360.0 ? heading - 360.0 : heading);
		}

		bool PlaceObject(const SceneryItem &item)
		{
			State &state = GetState();
			const XPlane::AssetType type = XPlane::GetAssetType(item.resource);
			if (type == XPlane::AssetType::AutogenPoint)
				return false;	// Drawn with the rest of the project's scenery (UpdateScenery)
			if (type != XPlane::AssetType::Object)
			{
				WarnOnce("type:" + item.resource, "'" + item.resource + "' stays in the project but isn't drawn (objects place .obj and .agp files)");
				return false;
			}
			const fs::path file = ResolveResource(item.resource);
			if (file.empty())
			{
				if (!LibraryReady())
				{
					state.waiting.push_back(item.id);	// Placed once the library index is ready
					return false;
				}
				WarnOnce("missing:" + item.resource, "'" + item.resource + "' wasn't found in the project folder or any tracked library");
				return false;
			}

			const XPlane::Point2 local = state.projection.ToLocal(item.latitude, item.longitude);
			XPlane::SceneObjects::PlaceOptions options;
			options.position[0] = static_cast<float>(local.x);
			options.position[1] = HeightOf(item);
			options.position[2] = static_cast<float>(local.z);
			options.headingDegrees = item.heading;
			options.virtualPath = item.resource;
			options.name = item.name.empty() ? ToUtf8(file.filename()) : item.name;
			options.frameCamera = false;

			std::string error;
			const uint64_t root = XPlane::SceneObjects::Place(file, options, &error);
			if (root == 0)
			{
				EDITOR_ERROR_TAG(LOG_TAG, "Could not place '{}': {}", item.resource, error);
				return false;
			}
			state.placed[item.id] = root;
			state.placedRoots.insert(root);
			return true;
		}

		void PlaceItems(const std::vector<SceneryItem> &items, const bool parentHidden)
		{
			for (const SceneryItem &item : items)
			{
				const bool hidden = parentHidden || item.hidden;
				if (item.kind == ItemKind::Object && !hidden)
					PlaceObject(item);
				if (item.IsContainer())
					PlaceItems(item.children, hidden);
			}
		}

		/* @brief Takes the project's scenery out of the scene (and the selection, when it was part of it). */
		void ClearScenery(EditorContext *context)
		{
			State &state = GetState();
			if (context && context->selectedEntity != 0 && state.scenery.Contains(context->selectedEntity))
				context->selectedEntity = 0;
			state.scenery.Clear();
			state.sceneryRevision = ~0ull;
			state.sceneryFingerprint = 0;
		}

		/**
		 * @brief Draws the items that aren't objects (draped polygons and orthophotos, lines, strings, forests, facades,
		 * .agp autogen) the way the airport scene draws a pack's DSF: the visible items become an in-memory DSF tile
		 * (ProjectFile::BuildProjectDsf) that the overlay builder turns into meshes and string / facade objects, one
		 * entity per item. Rebuilt only when what the DSF draws changed (or force: a new frame, a newer library index).
		 */
		void UpdateScenery(EditorContext *context, const bool force)
		{
			State &state = GetState();
			ProjectDocument &document = state.document;
			state.sceneryRevision = document.GetItemsRevision();
			state.sceneryLibrary = XPlane::LibraryManager::GetGeneration();

			const DsfConversion conversion = BuildProjectDsf(document.GetProject());
			const uint64_t fingerprint = GetDsfFingerprint(conversion.dsf);
			if (!force && fingerprint == state.sceneryFingerprint)
				return;
			for (const std::string &warning : conversion.warnings)
				WarnOnce("dsf:" + warning, warning + " (not drawn)");

			state.sceneryFingerprint = fingerprint;

			XPlane::OverlayBuild build;
			if (!conversion.dsf.polygons.empty() || !conversion.dsf.objects.empty())
			{
				XPlane::OverlayBuilder builder([](const std::string_view path) { return ResolveResource(std::string(path)); }, state.projection);
				XPlane::OverlayBuilder::Options options;
				options.layerHeight = [](const std::string_view group, const int offset) { return XPlane::SceneAirports::GetOverlayLayerHeight(group, offset); };
				builder.Build(conversion.dsf, options, build);
				for (const std::string &warning : build.warnings)
					WarnOnce("overlay:" + warning, warning);
			}

			// One entity per item: its meshes and the objects its strings, facades and autogen place
			std::vector<XPlane::OverlayScene::Group> groups;
			std::unordered_map<std::string, size_t> groupOf;
			const auto group = [&](const std::vector<std::string> &items, const uint32_t placement) -> XPlane::OverlayScene::Group *
			{
				if (placement >= items.size())
					return nullptr;
				const auto [it, added] = groupOf.emplace(items[placement], groups.size());
				if (added)
				{
					XPlane::OverlayScene::Group &entry = groups.emplace_back();
					entry.key = items[placement];
					const SceneryItem *item = document.FindItem(entry.key);
					entry.name = !item ? entry.key : !item->name.empty() ? item->name : GetItemKindName(item->kind);
				}
				return &groups[it->second];
			};
			for (const XPlane::OverlayMesh &mesh : build.meshes)
			{
				// .agp ground tiles come from object placements, everything else from polygons
				if (XPlane::OverlayScene::Group *entry = group(mesh.kind == XPlane::OverlayKind::Autogen ? conversion.objectItems : conversion.polygonItems, mesh.placement))
					entry->meshes.push_back(&mesh);
			}
			for (const XPlane::OverlayObject &object : build.objects)
			{
				const bool fromObject = object.source == XPlane::OverlayObject::Source::Dsf || object.source == XPlane::OverlayObject::Source::Autogen;
				if (XPlane::OverlayScene::Group *entry = group(fromObject ? conversion.objectItems : conversion.polygonItems, object.placement))
					entry->objects.push_back(&object);
			}

			// The item entities are replaced (the root stays, so selecting it survives), or all of it goes when nothing is left to draw
			if (context && context->selectedEntity != 0 && state.scenery.Contains(context->selectedEntity) && (groups.empty() || context->selectedEntity != state.scenery.GetRootId()))
				context->selectedEntity = 0;
			if (groups.empty())
			{
				state.scenery.Clear();
				return;
			}
			state.scenery.Build("Project scenery", groups);

			const XPlane::OverlayScene::Stats &stats = state.scenery.GetStats();
			EDITOR_INFO_TAG(LOG_TAG, "Project scenery: {} items drawn ({} meshes, {} triangles, {} objects){}{}", stats.groups, stats.parts, stats.triangles, stats.objects,
				conversion.hidden ? ", " + std::to_string(conversion.hidden) + " hidden items left out" : std::string(),
				build.unsupported ? ", " + std::to_string(build.unsupported) + " autogen strings / blocks not drawn yet" : std::string());
		}

		void UnloadScene(EditorContext *context, const bool closeAirport)
		{
			State &state = GetState();
			ClearScenery(context);
			for (const auto &[id, root] : state.placed)
			{
				if (context && context->selectedEntity == root)
					context->selectedEntity = 0;
				EditorScene::Destroy(root);
			}
			state.placed.clear();
			state.placedRoots.clear();
			state.waiting.clear();
			if (closeAirport && EditorAirports::IsProjectAirport())
				EditorAirports::CloseAirport(context, true);
			state.airportId.clear();
		}

		void LoadIntoScene(EditorContext *context)
		{
			State &state = GetState();
			UnloadScene(context, true);
			const SceneryProject &project = state.document.GetProject();

			// The first airport goes to the airport editor; its datum becomes the scene's frame
			if (!project.airports.empty())
			{
				state.airportId = project.airports.front().airport.id;
				EditorAirports::OpenProjectAirport(project.airports.front().airport, context, true);
			}

			double latitude = 0.0, longitude = 0.0;
			if (!state.airportId.empty() && XPlane::SceneAirports::IsLoaded())
				state.projection = XPlane::SceneAirports::GetProjection();
			else if (GetProjectCenter(project, latitude, longitude))
				state.projection = XPlane::GeoProjection(latitude, longitude);
			else
				state.projection = XPlane::GeoProjection(0.0, 0.0);
			state.datumElevation = project.datum.valid ? project.datum.elevation : !project.airports.empty() ? project.airports.front().airport.elevationFt * 0.3048 : 0.0;

			PlaceItems(project.items, false);
			UpdateScenery(context, true);
			state.lastSync = ImGui::GetTime();
		}

		/**
		 * @brief Brings edits made in the scene into the project: the airport editor's changes, moved / deleted project
		 * objects, and X-Plane objects placed at the top level of the scene (library drops, imports).
		 */
		void SyncFromScene()
		{
			State &state = GetState();
			ProjectDocument &document = state.document;
			if (!document.IsOpen())
				return;

			// The project airport, edited in the airport editor
			if (EditorAirports::IsProjectAirport())
			{
				XPlane::AirportDocument *airport = EditorAirports::GetDocument();
				if (airport && airport->IsDirty())
				{
					const size_t index = document.FindAirportIndex(state.airportId);
					if (index != ProjectDocument::NO_INDEX)
					{
						const std::string newId = airport->airport.id;
						document.GetProject().airports[index].airport = airport->airport;
						document.Mark(Section::Airport, index);
						if (newId != state.airportId)
						{
							// A renamed airport id: its hierarchy item follows
							document.GetProject().ForEachItem([&state, &newId](SceneryItem &item, int)
							{
								if (item.kind == ItemKind::Airport && item.airportId == state.airportId)
									item.airportId = newId;
								return true;
							});
							document.Mark(Section::Items);
							state.airportId = newId;
						}
					}
					airport->ClearDirty();
				}
			}

			// Project objects moved or deleted in the scene
			bool moved = false;
			std::vector<std::string> deleted;
			for (auto it = state.placed.begin(); it != state.placed.end();)
			{
				const auto &[id, root] = *it;
				SceneryItem *item = document.FindItem(id);
				const entt::entity handle = EditorScene::FindHandle(root);
				if (!item)
				{
					// Removed from the project (PROJECT panel): take it out of the scene too
					EditorScene::Destroy(root);
					state.placedRoots.erase(root);
					it = state.placed.erase(it);
					continue;
				}
				if (handle == entt::null)
				{
					deleted.push_back(id);
					state.placedRoots.erase(root);
					it = state.placed.erase(it);
					continue;
				}
				if (const TransformComponent *transform = EditorScene::TryGet<TransformComponent>(handle))
				{
					double latitude = 0.0, longitude = 0.0;
					state.projection.ToGeo({ transform->translation.x, transform->translation.z }, latitude, longitude);
					const float heading = HeadingOf(transform->GetRotation());
					const double height = transform->translation.y;
					const bool positionChanged = std::abs(latitude - item->latitude) > 1e-8 || std::abs(longitude - item->longitude) > 1e-8;
					const bool headingChanged = std::abs(std::remainder(static_cast<double>(heading) - item->heading, 360.0)) > 0.01;
					const bool heightChanged = item->elevationMode != ElevationMode::Ground && std::abs(height - HeightOf(*item)) > 0.005;
					if (positionChanged || headingChanged || heightChanged)
					{
						if (positionChanged)
						{
							item->latitude = latitude;
							item->longitude = longitude;
						}
						if (headingChanged)
							item->heading = heading;
						if (heightChanged)
							item->altitude = item->elevationMode == ElevationMode::Agl ? height : height + state.datumElevation;
						moved = true;
					}
				}
				++it;
			}
			if (moved)
				document.Mark(Section::Items);
			for (const std::string &id : deleted)
			{
				EDITOR_INFO_TAG(LOG_TAG, "Removed {} from the project (deleted in the scene)", id);
				document.RemoveItem(id);
			}

			// X-Plane objects placed at the top level of the scene become project objects
			std::vector<std::pair<SceneryItem, uint64_t>> adopted;
			for (const Entity *entity : Scene::GetEntities())
			{
				if (!entity)
					continue;
				const uint64_t id = static_cast<uint64_t>(entity->GetUUID());
				if (state.placedRoots.contains(id))
					continue;
				uint64_t root = 0;
				const XPlane::XPlaneObjectComponent *component = XPlane::SceneObjects::FindObject(id, &root);
				if (!component || root != id || entity->GetParent())
					continue;	// Not an object root, or part of something else (an airport's scenery)

				const entt::entity handle = static_cast<entt::entity>(*entity);
				const TransformComponent *transform = EditorScene::TryGet<TransformComponent>(handle);
				SceneryItem item;
				item.kind = ItemKind::Object;
				item.name = EditorScene::GetName(handle);
				if (!component->virtualPath.empty())
					item.resource = component->virtualPath;
				else if (component->asset)
					item.resource = ResourceForFile(component->asset->GetPath());
				if (item.resource.empty())
					continue;
				if (transform)
				{
					state.projection.ToGeo({ transform->translation.x, transform->translation.z }, item.latitude, item.longitude);
					item.heading = HeadingOf(transform->GetRotation());
					if (std::abs(transform->translation.y) > 0.005f)
					{
						item.elevationMode = ElevationMode::Agl;
						item.altitude = transform->translation.y;
					}
				}
				if (FromUtf8(item.resource).is_absolute())
					WarnOnce("absolute:" + item.resource, "'" + item.resource + "' is outside the project folder: the project stores its absolute path (copy it into the project to keep the project portable)");
				adopted.emplace_back(std::move(item), id);
			}
			for (auto &[item, root] : adopted)
			{
				const std::string name = item.name;
				const std::string id = document.AddItem(std::move(item));
				if (id.empty())
					continue;
				state.placed[id] = root;
				state.placedRoots.insert(root);
				EDITOR_INFO_TAG(LOG_TAG, "Added {} to the project", name);
			}
		}

		void UpdateWindowTitle()
		{
			State &state = GetState();
			const std::string project = GetTitle();
			const std::string title = std::format("{} - Scenery Editor X {}", project.empty() ? "Untitled Scene" : project, SEDX_VERSION_STRING);
			if (title != state.windowTitle)
			{
				state.windowTitle = title;
				Window::SetTitle(title);
			}
		}

		// Flow -----------------------------------------------------------

		/* @brief True when the open project may be dropped; otherwise queues the action behind the unsaved-changes modal. */
		bool ConfirmDiscard(const Pending action)
		{
			State &state = GetState();
			if (!state.document.IsOpen())
				return true;
			SyncFromScene();
			if (!IsDirty())
				return true;
			state.pending = action;
			state.openUnsaved = true;
			return false;
		}

		/* @brief An airport open on its own (not the project's) with unsaved edits would be replaced by the project's. */
		bool CheckOwnAirport()
		{
			const XPlane::AirportDocument *airport = EditorAirports::GetDocument();
			if (airport && !EditorAirports::IsProjectAirport() && airport->IsDirty())
			{
				EDITOR_WARN_TAG(LOG_TAG, "Save or close the open airport ({}) first", airport->GetTitle());
				return false;
			}
			return true;
		}

		void AfterOpen(EditorContext *context, const fs::path &path)
		{
			State &state = GetState();
			ProjectDocument &document = state.document;
			LogWarnings(document.GetWarnings(), "Open");
			state.suggestedPath.clear();
			state.warned.clear();
			state.validationRevision = ~0ull;
			LoadIntoScene(context);
			AddRecent(path);

			const SceneryProject &project = document.GetProject();
			EDITOR_INFO_TAG(LOG_TAG, "Opened {} ({}{}): {} airports, {} items", project.info.name, GetProjectFormatName(document.GetFormat()),
				document.WasLoadedFromSnapshot() ? ", from the cache" : "", project.airports.size(), project.CountItems());

			// Work an earlier session didn't save
			state.recovery = document.GetRecovery();
			if (state.recovery)
				state.openRecovery = true;
		}

		bool OpenNow(const fs::path &path, EditorContext *context)
		{
			State &state = GetState();
			if (!CheckOwnAirport())
				return false;
			ProjectDocument::OpenOptions options;
			options.load = MakeLoadOptions();
			std::string error;
			UnloadScene(context, true);
			if (!state.document.Open(path, &error, options))
			{
				EDITOR_ERROR_TAG(LOG_TAG, "Could not open {}: {}", ToUtf8(path), error);
				if (state.document.IsOpen())
					LoadIntoScene(context);		// The previous project stays open
				return false;
			}
			AfterOpen(context, path);
			return true;
		}

		bool CloseNow(EditorContext *context)
		{
			State &state = GetState();
			if (!state.document.IsOpen())
				return true;
			const std::string name = state.document.GetProject().info.name;
			UnloadScene(context, true);
			state.document.Close();
			state.recovery.reset();
			state.suggestedPath.clear();
			state.validation.clear();
			EDITOR_INFO_TAG(LOG_TAG, "Closed {}", name);
			return true;
		}

		void NewNow(EditorContext *context)
		{
			(void)context;
			if (CheckOwnAirport())
				ProjectDialogs::OpenNewProject();
		}

		void NewEmptyNow(EditorContext *context)
		{
			State &state = GetState();
			if (!CheckOwnAirport())
				return;
			const fs::path start = DefaultFolder() / "New Scenery Project.edX";
			const std::string chosen = pfd::save_file("Create a Scenery Editor X project", ToUtf8(start),
				{ "Scenery Editor X project (*.edX)", "*.edX", "Binary project (*.sex)", "*.sex" }).result();
			if (chosen.empty())
				return;
			const fs::path path = WithExtension(FromUtf8(chosen), EDX_EXTENSION);

			CloseNow(context);
			state.document.New(ToUtf8(path.stem()));
			std::string error;
			if (!state.document.SaveAs(path, ProjectFormat::Unknown, &error))
			{
				EDITOR_ERROR_TAG(LOG_TAG, "Could not create {}: {}", ToUtf8(path), error);
				state.document.Close();
				return;
			}
			for (const fs::path &created : WriteGitFiles(path.parent_path()))
				EDITOR_INFO_TAG(LOG_TAG, "Wrote {} (keeps the project git friendly)", ToUtf8(created.filename()));
			AfterOpen(context, path);
			EDITOR_INFO_TAG(LOG_TAG, "Created {}", ToUtf8(path));
		}

		void ImportWedNow(EditorContext *context)
		{
			State &state = GetState();
			if (!CheckOwnAirport())
				return;
			const std::vector<std::string> files = pfd::open_file("Import a WorldEditor project", ToUtf8(DefaultFolder()),
				{ "WorldEditor project (earth.wed.xml)", "earth.wed.xml *.wed.xml", "All files", "*" }).result();
			if (files.empty())
				return;
			const fs::path path = FromUtf8(files.front());

			SceneryProject project;
			std::vector<std::string> warnings;
			std::string error;
			if (!LoadProjectFile(path, project, nullptr, &warnings, &error, MakeLoadOptions()))
			{
				EDITOR_ERROR_TAG(LOG_TAG, "Could not import {}: {}", ToUtf8(path), error);
				return;
			}
			LogWarnings(warnings, "WED import");

			CloseNow(context);
			state.document.Adopt(std::move(project));
			const std::string name = state.document.GetProject().info.name;
			AfterOpen(context, {});
			state.suggestedPath = path.parent_path() / (name + EDX_EXTENSION);
			EDITOR_INFO_TAG(LOG_TAG, "Imported {} from WorldEditor; save it to keep it as a project", name);
		}

		void RunPending(EditorContext *context)
		{
			State &state = GetState();
			const Pending action = state.pending;
			state.pending = Pending::None;
			switch (action)
			{
				case Pending::Close:		CloseNow(context); break;
				case Pending::Open:			OpenNow(state.pendingPath, context); break;
				case Pending::New:			NewNow(context); break;
				case Pending::NewEmpty:		NewEmptyNow(context); break;
				case Pending::ImportWed:	ImportWedNow(context); break;
				case Pending::Quit:			CloseNow(context); Application::Get().Stop(); break;
				case Pending::None:			break;
			}
		}

		bool SaveTo(const fs::path &path, const ProjectFormat format)
		{
			State &state = GetState();
			SyncFromScene();
			std::string error;
			if (!state.document.SaveAs(path, format, &error))
			{
				EDITOR_ERROR_TAG(LOG_TAG, "Could not save {}: {}", ToUtf8(path), error);
				return false;
			}
			LogWarnings(state.document.GetWarnings(), "Save");
			state.suggestedPath.clear();
			state.lastAutosave = ImGui::GetTime();
			AddRecent(path);
			EDITOR_INFO_TAG(LOG_TAG, "Saved {} ({})", ToUtf8(path), GetProjectFormatName(state.document.GetFormat()));
			return true;
		}
	}

	// -------------------------------------------------------

	void Init(const Ref<UserPreferences> &preferences)
	{
		State &state = GetState();
		state.preferences = preferences;
		state.cache = std::make_unique<ProjectCache>(IO::FileSystem::GetPersistentStoragePath() / "project-cache");
		state.document.SetCache(state.cache.get());
		state.document.SetEditorVersion(SEDX_VERSION_STRING);
		EditorAirports::SetProjectSaveHandler([] { return Save(); });

		if (const PlatformContext *platform = Application::Get().GetPlatformContext())
		{
			const std::vector<std::string> &args = platform->GetCommandLineArgs();
			for (size_t i = 0; i + 1 < args.size(); ++i)
			{
				if (args[i] == "--project")
					state.startupProject = FromUtf8(args[++i]);
				else if (args[i] == "--project-autosave")
					state.autosaveSeconds = std::max(1.0, std::atof(args[++i].c_str()));	// Seconds between autosaves (verification runs)
			}
		}
		ProjectDialogs::Init();
	}

	void Shutdown()
	{
		State &state = GetState();
		if (state.document.IsOpen())
		{
			SyncFromScene();
			if (IsDirty())
			{
				// Quitting with unsaved work: it's kept and offered back the next time the project opens.
				std::string error;
				if (state.document.Autosave(&error))
					EDITOR_WARN_TAG(LOG_TAG, "{} has unsaved changes; they were kept and will be offered when it's opened again", state.document.GetProject().info.name);
				else
					EDITOR_ERROR_TAG(LOG_TAG, "Unsaved changes could not be kept: {}", error);
				UnloadScene(nullptr, true);
				state.document.Close(true);
			}
			else
				CloseNow(nullptr);
		}
		EditorAirports::SetProjectSaveHandler(nullptr);
		state.document.SetCache(nullptr);
		state.cache.reset();
		state.preferences = nullptr;
	}

	void Tick(EditorContext *context)
	{
		State &state = GetState();
		const double now = ImGui::GetTime();
		ProjectDialogs::Tick(context);

		// Project asked for on the command line, once the library index can resolve its objects
		if (!state.startupDone && !state.startupProject.empty() && LibraryReady())
		{
			state.startupDone = true;
			OpenNow(state.startupProject, context);
		}

		if (state.document.IsOpen())
		{
			if (!state.waiting.empty() && LibraryReady())
			{
				const std::vector<std::string> waiting = std::move(state.waiting);
				state.waiting.clear();
				for (const std::string &id : waiting)
				{
					if (const SceneryItem *item = state.document.FindItem(id))
						PlaceObject(*item);
				}
			}

			if (now - state.lastSync >= SYNC_SECONDS)
			{
				state.lastSync = now;
				SyncFromScene();
			}

			// Item edits (PROJECT panel, scene sync) and a newer library index redraw the project's scenery
			const bool libraryChanged = state.sceneryLibrary != XPlane::LibraryManager::GetGeneration();
			if (libraryChanged || state.sceneryRevision != state.document.GetItemsRevision())
				UpdateScenery(context, libraryChanged);

			if (now - state.lastAutosave >= state.autosaveSeconds)
			{
				state.lastAutosave = now;
				std::string error;
				if (IsDirty() && !state.document.Autosave(&error))
					EDITOR_WARN_TAG(LOG_TAG, "Autosave failed: {}", error);
			}

			if (state.validationRevision != state.document.GetRevision())
			{
				state.validation = ValidateProject(state.document.GetProject());
				state.validationRevision = state.document.GetRevision();
			}
		}
		UpdateWindowTitle();
	}

	ProjectDocument *GetDocument()
	{
		State &state = GetState();
		return state.document.IsOpen() ? &state.document : nullptr;
	}

	bool IsDirty()
	{
		const State &state = GetState();
		if (!state.document.IsOpen())
			return false;
		if (state.document.IsDirty())
			return true;
		const XPlane::AirportDocument *airport = EditorAirports::GetDocument();
		return airport && EditorAirports::IsProjectAirport() && airport->IsDirty();
	}

	std::string GetTitle()
	{
		const State &state = GetState();
		if (!state.document.IsOpen())
			return {};
		const std::string name = state.document.GetProject().info.name.empty() ? std::string("Untitled Project") : state.document.GetProject().info.name;
		return IsDirty() ? name + " *" : name;
	}

	void NewProject(EditorContext *context)
	{
		if (ConfirmDiscard(Pending::New))
			NewNow(context);
	}

	void NewEmptyProject(EditorContext *context)
	{
		if (ConfirmDiscard(Pending::NewEmpty))
			NewEmptyNow(context);
	}

	void BrowseOpen(EditorContext *context)
	{
		const std::vector<std::string> files = pfd::open_file("Open a project", ToUtf8(DefaultFolder()),
			{ "Projects (*.edX *.sex earth.wed.xml)", "*.edX *.edx *.sex earth.wed.xml *.wed.xml", "Scenery Editor X (*.edX)", "*.edX *.edx",
			  "Binary project (*.sex)", "*.sex", "WorldEditor (earth.wed.xml)", "earth.wed.xml *.wed.xml", "All files", "*" }).result();
		if (!files.empty())
			Open(FromUtf8(files.front()), context);
	}

	bool Open(const fs::path &path, EditorContext *context, const bool discardUnsaved)
	{
		State &state = GetState();
		if (discardUnsaved)
			return OpenNow(path, context);
		if (!ConfirmDiscard(Pending::Open))
		{
			state.pendingPath = path;
			return false;
		}
		return OpenNow(path, context);
	}

	void OpenWhenReady(const fs::path &path)
	{
		State &state = GetState();
		state.startupProject = path;
		state.startupDone = false;
	}

	bool Save()
	{
		State &state = GetState();
		if (!state.document.IsOpen())
			return false;
		if (!state.document.HasFile())
			return SaveAs();
		return SaveTo(state.document.GetPath(), state.document.GetFormat());
	}

	bool SaveAs()
	{
		State &state = GetState();
		if (!state.document.IsOpen())
			return false;
		const fs::path start = state.document.HasFile() ? state.document.GetPath()
			: !state.suggestedPath.empty() ? state.suggestedPath
			: DefaultFolder() / (state.document.GetProject().info.name + EDX_EXTENSION);
		const std::string chosen = pfd::save_file("Save project as", ToUtf8(start),
			{ "Scenery Editor X project (*.edX)", "*.edX", "Binary project (*.sex)", "*.sex", "WorldEditor project (earth.wed.xml)", "earth.wed.xml" }).result();
		if (chosen.empty())
			return false;
		return SaveTo(WithExtension(FromUtf8(chosen), EDX_EXTENSION), ProjectFormat::Unknown);
	}

	bool Close(EditorContext *context, const bool force)
	{
		if (!force && !ConfirmDiscard(Pending::Close))
			return false;
		return CloseNow(context);
	}

	void ImportWed(EditorContext *context)
	{
		if (ConfirmDiscard(Pending::ImportWed))
			ImportWedNow(context);
	}

	void ExportWed()
	{
		State &state = GetState();
		if (!state.document.IsOpen())
			return;
		const fs::path start = DefaultFolder() / WED_FILE_NAME;
		const std::string chosen = pfd::save_file("Export a WorldEditor project", ToUtf8(start), { "WorldEditor project (earth.wed.xml)", "earth.wed.xml" }).result();
		if (chosen.empty())
			return;
		const fs::path path = FromUtf8(chosen);
		SyncFromScene();

		// WED keeps the previous file as earth.wed.bak.xml; do the same
		std::error_code ec;
		if (fs::is_regular_file(path, ec))
			fs::copy_file(path, path.parent_path() / "earth.wed.bak.xml", fs::copy_options::overwrite_existing, ec);

		std::vector<std::string> warnings;
		std::string error;
		if (!state.document.ExportTo(path, ProjectFormat::Wed, &warnings, &error))
		{
			EDITOR_ERROR_TAG(LOG_TAG, "WED export failed: {}", error);
			return;
		}
		LogWarnings(warnings, "WED export");
		EDITOR_INFO_TAG(LOG_TAG, "Exported {} for WorldEditor", ToUtf8(path));
	}

	void ImportAptDat(EditorContext *context)
	{
		State &state = GetState();
		if (!state.document.IsOpen())
			return;
		const std::vector<std::string> files = pfd::open_file("Add airports from an apt.dat", ToUtf8(DefaultFolder()), { "Airport data (apt.dat)", "apt.dat *.dat", "All files", "*" }).result();
		if (files.empty())
			return;
		XPlane::AptFile file;
		std::string error;
		if (!XPlane::LoadAptDat(FromUtf8(files.front()), file, &error))
		{
			EDITOR_ERROR_TAG(LOG_TAG, "Could not read {}: {}", files.front(), error);
			return;
		}
		SyncFromScene();
		const bool hadAirport = !state.document.GetProject().airports.empty();
		for (const XPlane::AptAirport &airport : file.airports)
		{
			ProjectAirport added;
			added.airport = airport;
			state.document.AddAirport(std::move(added));
		}
		LogWarnings(file.warnings, "apt.dat");
		EDITOR_INFO_TAG(LOG_TAG, "Added {} airports from {}", file.airports.size(), files.front());
		if (!hadAirport)
			LoadIntoScene(context);
	}

	void ExportAptDat()
	{
		State &state = GetState();
		if (!state.document.IsOpen() || state.document.GetProject().airports.empty())
		{
			EDITOR_WARN_TAG(LOG_TAG, "The project has no airports to export");
			return;
		}
		const fs::path start = DefaultFolder() / "Earth nav data" / "apt.dat";
		const std::string chosen = pfd::save_file("Export the project's airports", ToUtf8(start), { "Airport data (apt.dat)", "apt.dat" }).result();
		if (chosen.empty())
			return;
		SyncFromScene();
		std::string error;
		if (!XPlane::SaveAptDat(FromUtf8(chosen), BuildAptFile(state.document.GetProject()), &error))
		{
			EDITOR_ERROR_TAG(LOG_TAG, "apt.dat export failed: {}", error);
			return;
		}
		EDITOR_INFO_TAG(LOG_TAG, "Exported {} airports to {}", state.document.GetProject().airports.size(), chosen);
	}

	void ImportLibrary()
	{
		State &state = GetState();
		if (!state.document.IsOpen())
			return;
		const std::vector<std::string> files = pfd::open_file("Import a library.txt", ToUtf8(DefaultFolder()), { "X-Plane library (library.txt)", "library.txt", "All files", "*" }).result();
		if (files.empty())
			return;
		XPlane::LibraryFile library;
		std::string error;
		if (!XPlane::LoadLibraryFile(FromUtf8(files.front()), library, &error))
		{
			EDITOR_ERROR_TAG(LOG_TAG, "Could not read {}: {}", files.front(), error);
			return;
		}
		LogWarnings(library.warnings, "library.txt");
		library.warnings.clear();
		const size_t exports = library.exports.size();
		state.document.GetProject().library = std::move(library);
		state.document.Mark(Section::Library);
		EDITOR_INFO_TAG(LOG_TAG, "Imported {} library exports from {}", exports, files.front());
	}

	void ExportLibrary()
	{
		State &state = GetState();
		if (!state.document.IsOpen() || !state.document.GetProject().library)
		{
			EDITOR_WARN_TAG(LOG_TAG, "The project has no library to export");
			return;
		}
		const fs::path start = DefaultFolder() / "library.txt";
		const std::string chosen = pfd::save_file("Export the library", ToUtf8(start), { "X-Plane library (library.txt)", "library.txt" }).result();
		if (chosen.empty())
			return;
		std::string error;
		if (!WriteFileAtomic(FromUtf8(chosen), WriteLibraryText(*state.document.GetProject().library), &error))
		{
			EDITOR_ERROR_TAG(LOG_TAG, "library.txt export failed: {}", error);
			return;
		}
		EDITOR_INFO_TAG(LOG_TAG, "Exported {} library exports to {}", state.document.GetProject().library->exports.size(), chosen);
	}

	void ExportBinary()
	{
		State &state = GetState();
		if (!state.document.IsOpen())
			return;
		const fs::path start = DefaultFolder() / (state.document.GetProject().info.name + SEX_EXTENSION);
		const std::string chosen = pfd::save_file("Export a binary copy", ToUtf8(start), { "Binary project (*.sex)", "*.sex" }).result();
		if (chosen.empty())
			return;
		SyncFromScene();
		std::string error;
		const fs::path path = WithExtension(FromUtf8(chosen), SEX_EXTENSION);
		if (!state.document.ExportTo(path, ProjectFormat::Sex, nullptr, &error))
			EDITOR_ERROR_TAG(LOG_TAG, "Binary export failed: {}", error);
		else
			EDITOR_INFO_TAG(LOG_TAG, "Exported {}", ToUtf8(path));
	}

	void RevealFolder()
	{
		if (const fs::path folder = ProjectFolder(); !folder.empty())
			IO::FileSystem::OpenDirectoryInExplorer(folder);
	}

	fs::path GetProjectFolder()
	{
		return ProjectFolder();
	}

	void ShowAirport(const std::string &airportId, EditorContext *context)
	{
		State &state = GetState();
		if (!state.document.IsOpen() || airportId == state.airportId)
			return;
		const size_t index = state.document.FindAirportIndex(airportId);
		if (index == ProjectDocument::NO_INDEX || !CheckOwnAirport())
			return;
		SyncFromScene();	// Keeps the edits of the airport shown now
		state.airportId = airportId;
		EditorAirports::OpenProjectAirport(state.document.GetProject().airports[index].airport, context, true);
	}

	const std::string &GetShownAirport()
	{
		return GetState().airportId;
	}

	uint64_t GetPlacedEntity(const std::string &itemId)
	{
		const State &state = GetState();
		const auto it = state.placed.find(itemId);
		return it != state.placed.end() ? it->second : state.scenery.FindEntity(itemId);
	}

	std::string FindItemForEntity(const uint64_t entityId)
	{
		const State &state = GetState();
		uint64_t root = entityId;
		XPlane::SceneObjects::FindObject(entityId, &root);
		for (const auto &[id, placedRoot] : state.placed)
		{
			if (placedRoot == root)
				return id;
		}
		return state.scenery.FindKey(entityId);
	}

	bool RemoveItem(const std::string &itemId, EditorContext *context)
	{
		State &state = GetState();
		if (!state.document.IsOpen())
			return false;
		// The item's placed objects (itself or its children) leave the scene with it
		std::vector<std::string> ids;
		if (const SceneryItem *item = state.document.FindItem(itemId))
		{
			const std::function<void(const SceneryItem &)> collect = [&](const SceneryItem &entry)
			{
				ids.push_back(entry.id);
				for (const SceneryItem &child : entry.children)
					collect(child);
			};
			collect(*item);
		}
		bool airport = false;
		for (const std::string &id : ids)
		{
			const SceneryItem *entry = state.document.FindItem(id);
			airport |= entry && entry->kind == ItemKind::Airport && entry->airportId == state.airportId;
			if (const auto it = state.placed.find(id); it != state.placed.end())
			{
				if (context && context->selectedEntity == it->second)
					context->selectedEntity = 0;
				EditorScene::Destroy(it->second);
				state.placedRoots.erase(it->second);
				state.placed.erase(it);
			}
		}
		if (!state.document.RemoveItem(itemId))
			return false;
		if (airport && EditorAirports::IsProjectAirport())
		{
			EditorAirports::CloseAirport(context, true);
			state.airportId.clear();
		}
		return true;
	}

	size_t AssignLibraries()
	{
		State &state = GetState();
		const std::shared_ptr<const XPlane::LibraryIndex> index = XPlane::LibraryManager::GetIndex();
		if (!state.document.IsOpen() || !index)
			return 0;
		SceneryProject &project = state.document.GetProject();
		const fs::path xplane = XPlane::LibraryManager::GetInstallPath();

		size_t assigned = 0;
		bool librariesChanged = false;
		project.ForEachItem([&](SceneryItem &item, int)
		{
			if (!HasResource(item.kind) || item.resource.empty())
				return true;
			const XPlane::LibraryAsset *asset = index->Find(item.resource);
			const XPlane::AssetProvider *provider = asset ? asset->GetResolved() : nullptr;
			if (!provider || provider->package >= index->GetPackages().size())
				return true;
			const XPlane::LibraryPackage &package = index->GetPackages()[provider->package];
			const std::string shortId = MakeLibraryShortId(package.name);
			if (!project.FindLibrary(shortId))
			{
				LibraryReference reference;
				reference.name = package.name;
				std::error_code ec;
				const fs::path relative = xplane.empty() ? fs::path() : fs::relative(package.path / "library.txt", xplane, ec);
				reference.localPath = !ec && !relative.empty() ? "./" + ToUtf8(fs::path(relative.generic_u8string())) : ToUtf8(fs::path((package.path / "library.txt").generic_u8string()));
				reference.entryCount = package.exportCount;
				reference.uuid = MakeUuid();
				reference.shortId = shortId;
				project.libraries.push_back(std::move(reference));
				librariesChanged = true;
			}
			if (item.library != shortId)
			{
				item.library = shortId;
				++assigned;
			}
			return true;
		});
		if (librariesChanged)
			state.document.Mark(Section::Libraries);
		if (assigned > 0)
			state.document.Mark(Section::Items);
		return assigned;
	}

	void MarkEdited(const Section section)
	{
		State &state = GetState();
		if (state.document.IsOpen())
			state.document.Mark(section);
	}

	bool FindItemLocation(const std::string &itemId, std::string &parentId, size_t &index)
	{
		State &state = GetState();
		if (!state.document.IsOpen())
			return false;
		SceneryProject &project = state.document.GetProject();
		const std::vector<SceneryItem> *list = project.FindParentList(itemId, &index);
		if (!list)
			return false;
		parentId.clear();
		if (list == &project.items)
			return true;
		bool found = false;
		project.ForEachItem([&](SceneryItem &item, int)
		{
			if (&item.children == list)
			{
				parentId = item.id;
				found = true;
				return false;
			}
			return true;
		});
		return found;
	}

	bool InsertItem(SceneryItem item, const std::string &parentId, const size_t index)
	{
		State &state = GetState();
		if (!state.document.IsOpen())
			return false;
		const std::string id = state.document.AddItem(std::move(item), parentId, index);
		if (id.empty())
			return false;
		// Objects go through the object loader; everything else is drawn with the project's scenery (UpdateScenery)
		if (const SceneryItem *added = state.document.FindItem(id))
			PlaceItems({ *added }, false);
		return true;
	}

	const XPlane::GeoProjection &GetProjection()
	{
		return GetState().projection;
	}

	void NotifySceneEdited()
	{
		State &state = GetState();
		if (!state.document.IsOpen())
			return;
		SyncFromScene();
		state.lastSync = ImGui::GetTime();
	}

	const std::vector<std::string> &GetValidationProblems()
	{
		return GetState().validation;
	}

	void DrawFileMenu(EditorContext *context)
	{
		State &state = GetState();
		const bool open = state.document.IsOpen();

		if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_PLANE, "New Airport Project...").c_str(), "Ctrl+N"))
			NewProject(context);
		EditorUI::ItemTooltip("A new scenery pack for an airport: ICAO, name, country and reference point, created in Custom Scenery or a folder you choose");
		if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_FILE, "New Empty Project...").c_str()))
			NewEmptyProject(context);
		if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_FOLDER_OPEN, "Open Project...").c_str(), "Ctrl+O"))
			BrowseOpen(context);
		if (ImGui::BeginMenu("Open Recent", state.preferences && !state.preferences->GetRecentProjects().empty()))
		{
			size_t shown = 0;
			for (const auto &[time, recent] : state.preferences->GetRecentProjects())
			{
				if (shown++ >= 10)
					break;
				if (ImGui::MenuItem(recent.name.empty() ? recent.filePath.c_str() : recent.name.c_str()))
					Open(FromUtf8(recent.filePath), context);
				EditorUI::ItemTooltip(recent.filePath.c_str());
			}
			ImGui::EndMenu();
		}
		ImGui::Separator();
		if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_FLOPPY_O, "Save Project").c_str(), "Ctrl+S", false, open))
			Save();
		if (ImGui::MenuItem("Save Project As...", "Ctrl+Shift+S", false, open))
			SaveAs();
		if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_TIMES, "Close Project").c_str(), nullptr, false, open))
			Close(context);
		ImGui::Separator();
		if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_UPLOAD, "Export Scenery Pack...").c_str(), "Ctrl+E", false, open))
			ProjectDialogs::OpenExport();
		EditorUI::ItemTooltip("Write the project as an X-Plane scenery pack: Earth nav data/apt.dat, the DSF tiles and the pack's own art");
		if (ImGui::MenuItem("Export Again", "Ctrl+Shift+E", false, open))
			ProjectDialogs::QuickExport();
		ImGui::Separator();
		if (ImGui::BeginMenu("Import"))
		{
			if (ImGui::MenuItem("WorldEditor Project (earth.wed.xml)..."))
				ImportWed(context);
			if (ImGui::MenuItem("Airports (apt.dat)...", nullptr, false, open))
				ImportAptDat(context);
			if (ImGui::MenuItem("Library (library.txt)...", nullptr, false, open))
				ImportLibrary();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Export", open))
		{
			if (ImGui::MenuItem("X-Plane Scenery Pack...", "Ctrl+E"))
				ProjectDialogs::OpenExport();
			if (ImGui::MenuItem("WorldEditor Project (earth.wed.xml)..."))
				ExportWed();
			if (ImGui::MenuItem("Airports (apt.dat)...", nullptr, false, !state.document.GetProject().airports.empty()))
				ExportAptDat();
			if (ImGui::MenuItem("Library (library.txt)...", nullptr, false, state.document.GetProject().library.has_value()))
				ExportLibrary();
			if (ImGui::MenuItem("Binary Copy (.sex)..."))
				ExportBinary();
			ImGui::EndMenu();
		}
		if (ImGui::MenuItem(EditorUI::IconLabel(SEDX_ICON_FOLDER_OPEN, "Show Project Folder").c_str(), nullptr, false, !ProjectFolder().empty()))
			RevealFolder();
		ImGui::Separator();
		if (ImGui::MenuItem("Exit", "Alt+F4"))
		{
			if (ConfirmDiscard(Pending::Quit))
				Application::Get().Stop();
		}
	}

	void DrawModals(EditorContext *context)
	{
		ProjectDialogs::Draw(context);
		State &state = GetState();
		if (state.openUnsaved)
		{
			ImGui::OpenPopup(UNSAVED_MODAL);
			state.openUnsaved = false;
		}
		if (state.openRecovery)
		{
			ImGui::OpenPopup(RECOVERY_MODAL);
			state.openRecovery = false;
		}

		const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		const float width = 120.0f * ImGui::GetStyle().FontScaleDpi;
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal(UNSAVED_MODAL, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("%s has unsaved changes.", state.document.GetProject().info.name.c_str());
			ImGui::Spacing();
			if (ImGui::Button("Save", ImVec2(width, 0.0f)))
			{
				if (Save())
				{
					ImGui::CloseCurrentPopup();
					RunPending(context);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Don't save", ImVec2(width, 0.0f)))
			{
				ImGui::CloseCurrentPopup();
				CloseNow(context);
				RunPending(context);
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(width, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
			{
				ImGui::CloseCurrentPopup();
				state.pending = Pending::None;
			}
			ImGui::EndPopup();
		}

		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal(RECOVERY_MODAL, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (state.recovery)
			{
				ImGui::Text("Unsaved changes to %s from %s were found.", state.recovery->projectName.c_str(), state.recovery->written.c_str());
				if (state.recovery->sourceChangedSince)
					ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "The project file changed after they were made (another save, a git checkout...).");
				ImGui::TextDisabled("Restoring replaces what's open with them; nothing is written until you save.");
			}
			ImGui::Spacing();
			if (ImGui::Button("Restore", ImVec2(width, 0.0f)))
			{
				std::vector<std::string> warnings;
				std::string error;
				if (state.document.RestoreRecovery(&warnings, &error))
				{
					LogWarnings(warnings, "Recovery");
					LoadIntoScene(context);
					EDITOR_INFO_TAG(LOG_TAG, "Restored the unsaved changes of {}", state.document.GetProject().info.name);
				}
				else
					EDITOR_ERROR_TAG(LOG_TAG, "Could not restore: {}", error);
				state.recovery.reset();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Discard", ImVec2(width, 0.0f)))
			{
				state.document.DiscardRecovery();
				state.recovery.reset();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Decide later", ImVec2(width, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
			{
				state.recovery.reset();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}
}

// -------------------------------------------------------

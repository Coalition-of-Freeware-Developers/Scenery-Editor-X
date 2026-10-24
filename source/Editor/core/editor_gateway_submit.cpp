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
 * editor_gateway_submit.cpp
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#include "editor_gateway_submit.h"
#include "editor_airports.h"
#include "editor_gateway.h"
#include "editor_project.h"
#include "editor_theme.h"
#include "editor_ui.h"
#include <algorithm>
#include <cstdlib>
#include <future>
#include <optional>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/platform/credential_store.h>
#include <SceneryEditorX/core/platform/platform_context.h>
#include <SceneryEditorX/filesystem/file_system.h>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/project/project_gateway.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/xplane/xp_assets.h>
#include <SceneryEditorX/xplane/xp_gateway.h>
#include <SceneryEditorX/xplane/xp_gateway_package.h>
#include <SceneryEditorX/xplane/xp_gateway_validation.h>
#include <SceneryEditorX/xplane/xp_library_manager.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorGatewaySubmit
{
	namespace fs = std::filesystem;
	using namespace XPlane;

	namespace
	{
		constexpr const char *LOG_TAG = "Gateway";
		constexpr const char *WINDOW = "Submit to Scenery Gateway###GatewaySubmit";
		constexpr const char *CONFIRM_POPUP = "Upload to the Gateway?###GatewaySubmitConfirm";
		constexpr const char *PASSWORD_ENV = "SEDX_GATEWAY_PASSWORD";
		constexpr size_t MAX_SHOWN_PROBLEMS = 200;

		enum class Phase : uint8_t
		{
			Review,			// Showing what would go up, the validation and the comments
			Packaging,		// Building the zips on a worker
			Confirm,		// Packed: the confirmation names the server and the size
			Uploading,		// The request is with the Gateway client
			Done			// The server answered (or the upload wasn't sent)
		};

		/* @brief What the dialog shows: the airport as it would be uploaded, checked. */
		struct Prepared
		{
			bool valid = false;
			std::string error;						// Why there's nothing to submit
			bool fromProject = false;
			std::string airportId;
			std::string source;						// "Project 'X'" / "Scenery pack 'Y'"
			AptAirport airport;						// gui_label set to match the content
			DsfFile overlay;
			GatewayContentSummary summary;
			GatewayValidation validation;
			int parentId = 0;
			std::string validatedAgainst;			// X-Plane's build, "" when Log.txt couldn't be read
			GatewayServer server;					// Prepared for this server (the parent depends on it)
		};

		struct PackResult
		{
			bool ok = false;
			GatewayPackage package;
			std::string error;
		};

		struct State
		{
			bool initialized = false;
			bool open = false;
			bool focus = false;
			std::string projectAirport;				// Opened for this project airport; empty: what the airport editor shows
			Prepared prepared;
			char comments[4096] = {};
			bool warningsAccepted = false;

			Phase phase = Phase::Review;
			bool openConfirm = false;
			std::future<PackResult> packaging;
			std::shared_ptr<const GatewaySubmission> submission;	// Packed, waiting for the confirmation
			size_t packageBytes = 0;
			std::vector<std::string> packageWarnings;
			std::shared_ptr<GatewayRequest> upload;
			std::string uploadUser;
			std::optional<GatewayUploadResult> result;
			std::string resultNote;
			std::string failure;					// Packaging failed or the upload wasn't sent

			std::shared_ptr<GatewayRequest> import;
			int importId = 0;
			EditorContext *importContext = nullptr;

			// Command line
			int startupImport = 0;
			fs::path startupImportSave;
			std::string startupSubmit;
			std::string autoUser;
			std::string autoPassword;
			std::string autoComments;
			bool waitForProject = false;			// --project: import into it once it's open
			bool startupImportStarted = false;
			bool startupSubmitDone = false;
			bool autoMode = false;
		};

		State &GetState()
		{
			static State state;
			return state;
		}

		ImVec4 Color(const ImVec4 &palette)
		{
			return EditorTheme::ToStyle(palette);
		}

		bool LibraryReady()
		{
			return LibraryManager::GetGeneration() > 0 || LibraryManager::GetScanState() == ScanState::Failed || LibraryManager::GetInstallPath().empty();
		}

		std::string FormatBytes(const size_t bytes)
		{
			char text[32];
			if (bytes >= 1024 * 1024)
				snprintf(text, sizeof(text), "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
			else
				snprintf(text, sizeof(text), "%.1f KB", static_cast<double>(bytes) / 1024.0);
			return text;
		}

		std::string ServerName(const GatewayServer &server)
		{
			return server.official ? std::string("the public X-Plane Scenery Gateway") : "the TEST SERVER " + server.apiUrl;
		}

		/* @brief "1 runway, 3 objects, ..." for the counts that aren't zero. */
		std::string DescribeContent(const GatewayContentSummary &summary)
		{
			std::string out;
			const auto add = [&out](const size_t count, const char *one, const char *many)
			{
				if (count == 0)
					return;
				out += (out.empty() ? "" : ", ") + std::to_string(count) + " " + (count == 1 ? one : many);
			};
			add(summary.runways, "runway", "runways");
			add(summary.waterRunways, "water runway", "water runways");
			add(summary.helipads, "helipad", "helipads");
			add(summary.pavements, "pavement", "pavements");
			add(summary.lines, "painted line", "painted lines");
			add(summary.startups, "ramp start", "ramp starts");
			add(summary.jetways, "jetway", "jetways");
			add(summary.objects, "object", "objects");
			add(summary.autogenPoints, "autogen point", "autogen points");
			add(summary.facades, "facade", "facades");
			add(summary.forests, "forest", "forests");
			add(summary.strings, "object string", "object strings");
			add(summary.linePlacements, "line", "lines");
			add(summary.polygons, "draped polygon", "draped polygons");
			add(summary.orthophotos, "orthophoto", "orthophotos");
			add(summary.autogen, "autogen block", "autogen blocks");
			add(summary.roadSegments, "road", "roads");
			add(summary.exclusions, "exclusion", "exclusions");
			return out.empty() ? std::string("nothing") : out;
		}

		/* @brief Several messages as one warning: "N <what>: a, b, c and 4 more". */
		void AddListWarning(GatewayValidation &validation, const std::string &code, const std::string &what, const std::vector<std::string> &items)
		{
			if (items.empty())
				return;
			std::string text = std::to_string(items.size()) + " " + what + ": ";
			for (size_t i = 0; i < items.size() && i < 3; ++i)
				text += (i ? "; " : "") + items[i];
			if (items.size() > 3)
				text += " and " + std::to_string(items.size() - 3) + " more";
			validation.Add(GatewaySeverity::Warning, code, text);
		}

		/* @brief Looks at the airport the dialog is for: content, parent, validation. */
		void Prepare(State &state)
		{
			Prepared prepared;
			prepared.server = EditorGateway::GetServer();
			std::vector<std::string> collectWarnings, outside, packWarnings;
			uint32_t hidden = 0;
			fs::path packFolder;

			ProjectFile::ProjectDocument *project = EditorProjects::GetDocument();
			const bool projectOpen = project && project->IsOpen();
			std::string airportId = state.projectAirport;
			if (airportId.empty() && EditorAirports::IsProjectAirport())
				airportId = EditorProjects::GetShownAirport();

			if (projectOpen && !airportId.empty())
			{
				EditorProjects::NotifySceneEdited();	// The airport editor's and the scene's edits into the project
				ProjectFile::GatewayContent content;
				std::string error;
				if (!ProjectFile::CollectGatewayContent(project->GetProject(), airportId, content, &error))
				{
					prepared.error = error;
					state.prepared = std::move(prepared);
					return;
				}
				prepared.fromProject = true;
				prepared.source = "Project '" + project->GetProject().info.name + "'";
				prepared.airport = std::move(content.airport);
				prepared.overlay = std::move(content.overlay);
				collectWarnings = std::move(content.warnings);
				outside = std::move(content.outside);
				hidden = content.hidden;
				if (const ProjectFile::ProjectAirport *airport = project->GetProject().FindAirport(airportId))
					prepared.parentId = ProjectFile::GetGatewaySceneryId(*airport, prepared.server.official, prepared.server.GetKey());
				packFolder = EditorProjects::GetProjectFolder();
			}
			else if (const AirportDocument *document = EditorAirports::GetDocument())
			{
				prepared.source = "Scenery pack '" + document->GetPackageName() + "'";
				prepared.airport = document->airport;
				if (document->HasFile())
				{
					packFolder = document->GetPackageFolder();
					prepared.overlay = LoadPackOverlay(packFolder, document->airport, &packWarnings);
				}
				// A pack airport has no project to remember its scenery id: use an open project's airport with the same ID
				if (projectOpen)
				{
					if (const ProjectFile::ProjectAirport *airport = project->GetProject().FindAirport(document->airport.id))
						prepared.parentId = ProjectFile::GetGatewaySceneryId(*airport, prepared.server.official, prepared.server.GetKey());
				}
			}
			else
			{
				prepared.error = "Open an airport (from a project or a scenery pack) to submit it.";
				state.prepared = std::move(prepared);
				return;
			}
			prepared.airportId = prepared.airport.id;

			prepared.summary = SummarizeGatewayContent(prepared.airport, prepared.overlay);
			ApplyGatewayGuiLabel(prepared.airport, prepared.summary.is3D);

			GatewayValidationOptions options;
			if (LibraryManager::GetGeneration() > 0)
			{
				const std::shared_ptr<const LibraryIndex> index = LibraryManager::GetIndex();
				options.describeResource = [index](const std::string_view path) { return DescribeGatewayResource(*index, path); };
			}
			options.countSubtextures = [packFolder](const std::string_view path) -> int
			{
				fs::path file = LibraryManager::Resolve(path);
				if (file.empty() && !packFolder.empty())
					file = VFS::FindCaseInsensitive(packFolder / VFS::FromUtf8(std::string(path))).value_or(fs::path());
				PolygonAsset polygon;
				if (file.empty() || !LoadPolygonAsset(file, polygon))
					return -1;
				return static_cast<int>(polygon.subtextures.size());
			};
			prepared.validation = ValidateGatewayAirport(prepared.airport, prepared.overlay, options);

			AddListWarning(prepared.validation, "left-out", "items can't be converted for the DSF and are left out", collectWarnings);
			AddListWarning(prepared.validation, "outside-hierarchy", "visible items are outside the airport's hierarchy in the project and won't be uploaded", outside);
			AddListWarning(prepared.validation, "pack", "notes about the pack's DSF tiles", packWarnings);
			if (hidden > 0)
				prepared.validation.Add(GatewaySeverity::Warning, "hidden", std::to_string(hidden) + " hidden items in the airport's hierarchy won't be uploaded");
			if (const AirportDocument *document = EditorAirports::GetDocument(); !prepared.fromProject && document && document->IsDirty())
				prepared.validation.Add(GatewaySeverity::Warning, "unsaved", "The airport has unsaved edits: they're uploaded as the editor shows them");

			prepared.validatedAgainst = LoadXPlaneBuild(LibraryManager::GetInstallPath());
			if (prepared.validatedAgainst.empty())
				prepared.validation.Add(GatewaySeverity::Warning, "xplane-build", "X-Plane's build couldn't be read from its Log.txt (run X-Plane once): "
					"the upload says 'Unknown', which the Gateway may refuse");

			prepared.valid = true;
			state.prepared = std::move(prepared);
			state.warningsAccepted = false;
		}

		void ResetUpload(State &state)
		{
			state.phase = Phase::Review;
			state.openConfirm = false;
			state.submission.reset();
			state.packageBytes = 0;
			state.packageWarnings.clear();
			if (state.upload && !state.upload->IsFinished())
				state.upload->Cancel();
			state.upload.reset();
			state.result.reset();
			state.resultNote.clear();
			state.failure.clear();
		}

		void StartPackaging(State &state)
		{
			const Prepared &prepared = state.prepared;
			GatewayPackageInput input;
			input.airport = prepared.airport;
			input.overlay = prepared.overlay;
			input.uploader = state.autoMode ? state.autoUser : EditorGateway::GetUserName();
			input.comments = state.comments;
			input.parentId = prepared.parentId;
			input.validatedAgainst = prepared.validatedAgainst;
			input.creationAgent = std::string("Scenery Editor X ") + SEDX_VERSION_STRING;

			ResetUpload(state);
			state.phase = Phase::Packaging;
			state.packaging = std::async(std::launch::async, [input = std::move(input)]
			{
				PackResult result;
				result.ok = BuildGatewayPackage(input, result.package, &result.error);
				return result;
			});
		}

		/* @brief Sends the packed submission with the account's credentials (the password is wiped once handed over). */
		void StartUpload(State &state)
		{
			std::string userName, password;
			if (state.autoMode)
			{
				userName = state.autoUser;
				password = std::move(state.autoPassword);
				Credentials::SecureClear(state.autoPassword);
			}
			else if (!EditorGateway::GetCredentials(userName, password))
			{
				state.failure = "Set your Gateway user name and password first (Edit > Preferences > Scenery Gateway).";
				state.phase = Phase::Done;
				return;
			}
			const GatewayServer server = EditorGateway::GetServer();
			std::string reason;
			if (!CanUploadTo(server, GatewayClient::AreLiveUploadsAllowed(), &reason))
			{
				Credentials::SecureClear(password);
				state.failure = reason;
				state.phase = Phase::Done;
				return;
			}
			if (!(server == state.prepared.server))
			{
				Credentials::SecureClear(password);
				state.failure = "The Gateway server changed since the airport was checked: check it again.";
				state.phase = Phase::Done;
				return;
			}
			state.upload = GatewayClient::UploadScenery(state.submission, userName, password);
			Credentials::SecureClear(password);
			state.uploadUser = userName;
			state.phase = Phase::Uploading;
			EDITOR_INFO_TAG(LOG_TAG, "Uploading {} ({}, {}) to {} as {}{}", state.submission->icao, state.submission->is3D ? "3D" : "2D", FormatBytes(state.packageBytes),
				server.GetKey(), userName, state.submission->parentId > 0 ? ", parent " + std::to_string(state.submission->parentId) : std::string(", no parent"));
		}

		/* @brief The answer: the scenery id becomes the project airport's parent for the next upload. */
		void FinishUpload(State &state)
		{
			const std::shared_ptr<GatewayRequest> request = std::move(state.upload);
			state.upload.reset();
			state.phase = Phase::Done;
			if (!request->upload)
			{
				state.failure = request->GetState() == GatewayRequestState::Cancelled ? std::string("The upload was cancelled.") : "The upload wasn't sent: " + request->GetError();
				EDITOR_WARN_TAG(LOG_TAG, "{}", state.failure);
				return;
			}

			state.result = *request->upload;
			const GatewayUploadResult &result = *state.result;
			if (!state.autoMode)
				EditorGateway::ReportUploadResult(result);
			if (!result.Succeeded())
			{
				EDITOR_WARN_TAG(LOG_TAG, "Upload of {} to {}: {} ({})", state.prepared.airportId, state.prepared.server.GetKey(), GetGatewayUploadOutcomeName(result.outcome), result.message);
				return;
			}
			EDITOR_INFO_TAG(LOG_TAG, "Upload of {} to {}: scenery {} ({})", state.prepared.airportId, state.prepared.server.GetKey(), result.sceneryId, result.message);

			ProjectFile::ProjectDocument *project = EditorProjects::GetDocument();
			const size_t index = project && project->IsOpen() && state.prepared.fromProject ? project->FindAirportIndex(state.prepared.airportId) : ProjectFile::ProjectDocument::NO_INDEX;
			if (index == ProjectFile::ProjectDocument::NO_INDEX)
			{
				state.resultNote = "This airport isn't in a project, so nothing remembers the new scenery id: the next upload of it starts without a parent.";
				return;
			}
			ProjectFile::SetGatewaySceneryId(project->GetProject().airports[index], state.prepared.server.official, state.prepared.server.GetKey(), result.sceneryId);
			project->Mark(ProjectFile::ProjectDocument::Section::Airport, index);
			state.prepared.parentId = result.sceneryId;
			state.resultNote = "Scenery id " + std::to_string(result.sceneryId) + " is recorded in the project for " +
				(state.prepared.server.official ? std::string("the public Gateway") : state.prepared.server.GetKey()) + ": save the project to keep it for the next upload.";
			if (state.autoMode && project->HasFile() && EditorProjects::Save())
				state.resultNote = "Scenery id " + std::to_string(result.sceneryId) + " is recorded in the project, which was saved.";
		}

		void ReadCommandLine(State &state)
		{
			const PlatformContext *platform = Application::Get().GetPlatformContext();
			if (!platform)
				return;
			const std::vector<std::string> &args = platform->GetCommandLineArgs();
			for (size_t i = 0; i < args.size(); ++i)
			{
				const bool hasValue = i + 1 < args.size();
				if (args[i] == "--gateway-import" && hasValue)
				{
					state.startupImport = std::atoi(args[++i].c_str());
					if (state.startupImport <= 0)
						EDITOR_ERROR_TAG(LOG_TAG, "--gateway-import {}: expected a scenery id", args[i]);
				}
				else if (args[i] == "--gateway-import-save" && hasValue)
					state.startupImportSave = VFS::FromUtf8(args[++i]);
				else if (args[i] == "--gateway-submit" && hasValue)
					state.startupSubmit = args[++i];
				else if (args[i] == "--gateway-submit-auto" && hasValue)
					state.autoUser = args[++i];
				else if (args[i] == "--gateway-comments" && hasValue)
					state.autoComments = args[++i];
				else if (args[i] == "--project")
					state.waitForProject = true;
			}
			state.startupSubmitDone = state.startupSubmit.empty();

			// Unattended uploads: test servers only, the password from the environment
			if (!state.autoUser.empty())
			{
				const char *password = std::getenv(PASSWORD_ENV);
				if (state.startupSubmit.empty())
					EDITOR_ERROR_TAG(LOG_TAG, "--gateway-submit-auto needs --gateway-submit <airport id>");
				else if (EditorGateway::GetServer().official)
					EDITOR_ERROR_TAG(LOG_TAG, "--gateway-submit-auto only works with a test server (--gateway-url): it reads the password from {}", PASSWORD_ENV);
				else if (!password || !*password)
					EDITOR_ERROR_TAG(LOG_TAG, "--gateway-submit-auto {}: set {} to the test account's password", state.autoUser, PASSWORD_ENV);
				else
				{
					state.autoMode = true;
					state.autoPassword = password;
				}
			}
		}

		/* @brief The startup import / submission, once the library index (and a --project) are ready. */
		void TickStartup(State &state, EditorContext *context)
		{
			if (!LibraryReady())
				return;
			ProjectFile::ProjectDocument *project = EditorProjects::GetDocument();
			const bool projectReady = !state.waitForProject || (project && project->IsOpen());
			if (!projectReady)
				return;

			if (state.startupImport > 0 && !state.startupImportStarted)
			{
				state.startupImportStarted = true;
				ImportSubmission(state.startupImport, context);
				return;
			}
			if (state.import)
				return;		// Still downloading

			if (!state.startupSubmitDone)
			{
				state.startupSubmitDone = true;
				snprintf(state.comments, sizeof(state.comments), "%s", state.autoComments.c_str());
				OpenForProjectAirport(state.startupSubmit, context);
				if (state.autoMode)
				{
					if (!state.prepared.valid)
						EDITOR_ERROR_TAG(LOG_TAG, "--gateway-submit {}: {}", state.startupSubmit, state.prepared.error);
					else
					{
						for (const GatewayProblem &problem : state.prepared.validation.problems)
						{
							if (problem.severity == GatewaySeverity::Error)
								EDITOR_ERROR_TAG(LOG_TAG, "Gateway validation: {}", problem.message);
							else
								EDITOR_WARN_TAG(LOG_TAG, "Gateway validation (warning): {}", problem.message);
						}
						if (state.prepared.validation.HasErrors())
							EDITOR_ERROR_TAG(LOG_TAG, "--gateway-submit {}: {} validation errors block the upload", state.startupSubmit, state.prepared.validation.CountErrors());
						else
						{
							EDITOR_INFO_TAG(LOG_TAG, "--gateway-submit {}: validation passed ({} warnings accepted for the test run)", state.startupSubmit, state.prepared.validation.CountWarnings());
							state.warningsAccepted = true;
							StartPackaging(state);
						}
					}
				}
			}
		}

		// -------------------------------------------------------

		void DrawProblems(const GatewayValidation &validation)
		{
			const size_t errors = validation.CountErrors();
			const size_t warnings = validation.CountWarnings();
			if (errors == 0 && warnings == 0)
			{
				ImGui::TextColored(Color(EditorTheme::Palette::Accent), "%s  No problems found", SEDX_ICON_CHECK);
				return;
			}
			if (errors > 0)
				ImGui::TextColored(Color(EditorTheme::Palette::Error), "%s  %zu %s block the upload", SEDX_ICON_TIMES_CIRCLE, errors, errors == 1 ? "error" : "errors");
			if (warnings > 0)
			{
				if (errors > 0)
					ImGui::SameLine();
				ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%s  %zu %s to confirm", SEDX_ICON_EXCLAMATION_TRIANGLE, warnings, warnings == 1 ? "warning" : "warnings");
			}

			const float height = std::min(ImGui::GetTextLineHeightWithSpacing() * 8.0f, ImGui::GetTextLineHeightWithSpacing() * (static_cast<float>(validation.problems.size()) + 1.0f));
			if (ImGui::BeginChild("##GatewayProblems", ImVec2(0.0f, height), ImGuiChildFlags_Borders))
			{
				size_t shown = 0;
				for (const GatewaySeverity severity : { GatewaySeverity::Error, GatewaySeverity::Warning })
				{
					for (const GatewayProblem &problem : validation.problems)
					{
						if (problem.severity != severity || ++shown > MAX_SHOWN_PROBLEMS)
							continue;
						const bool error = severity == GatewaySeverity::Error;
						ImGui::PushStyleColor(ImGuiCol_Text, Color(error ? EditorTheme::Palette::Error : EditorTheme::Palette::Warning));
						ImGui::TextUnformatted(error ? SEDX_ICON_TIMES_CIRCLE : SEDX_ICON_EXCLAMATION_TRIANGLE);
						ImGui::PopStyleColor();
						ImGui::SameLine();
						ImGui::TextWrapped("%s", problem.message.c_str());
					}
				}
			}
			ImGui::EndChild();
		}

		void DrawConfirmPopup(State &state)
		{
			if (state.openConfirm)
			{
				ImGui::OpenPopup(CONFIRM_POPUP);
				state.openConfirm = false;
			}
			const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			if (!ImGui::BeginPopupModal(CONFIRM_POPUP, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				return;

			const Prepared &prepared = state.prepared;
			const GatewayServer server = EditorGateway::GetServer();
			ImGui::Text("Upload %s %s to", prepared.airportId.c_str(), prepared.airport.name.c_str());
			ImGui::TextColored(Color(server.official ? EditorTheme::Palette::Error : EditorTheme::Palette::Warning), "%s  %s", SEDX_ICON_CLOUD_UPLOAD,
				ServerName(server).c_str());
			ImGui::Text("as %s?", EditorGateway::GetUserName().c_str());
			ImGui::Spacing();
			ImGui::TextDisabled("%s, %s package; %s", prepared.summary.is3D ? "3D" : "2D", FormatBytes(state.packageBytes).c_str(),
				prepared.parentId > 0 ? ("updates submission " + std::to_string(prepared.parentId)).c_str() : "a first upload (no parent)");
			for (const std::string &warning : state.packageWarnings)
				ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%s  %s", SEDX_ICON_EXCLAMATION_TRIANGLE, warning.c_str());
			if (server.official)
				ImGui::TextWrapped("A moderator reviews every submission; the Gateway lists it once it's accepted.");
			ImGui::Spacing();

			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_CLOUD_UPLOAD, "Upload").c_str(), ImVec2(120.0f * ImGui::GetStyle().FontScaleDpi, 0.0f)))
			{
				StartUpload(state);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120.0f * ImGui::GetStyle().FontScaleDpi, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
			{
				ResetUpload(state);
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		void DrawProgress(State &state)
		{
			switch (state.phase)
			{
				case Phase::Review:
				case Phase::Confirm:
					break;
				case Phase::Packaging:
					ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%s  Packing the airport...", SEDX_ICON_COG);
					break;
				case Phase::Uploading:
				{
					const GatewayRequest *request = state.upload.get();
					ImGui::TextColored(Color(EditorTheme::Palette::Warning), "%s  Uploading %s to %s... %.0f s", SEDX_ICON_CLOUD_UPLOAD, FormatBytes(state.packageBytes).c_str(),
						state.prepared.server.GetKey().c_str(), request ? request->GetSeconds() : 0.0);
					const float progress = request ? request->GetProgress() : -1.0f;
					ImGui::ProgressBar(progress >= 0.0f ? progress : -1.0f * static_cast<float>(ImGui::GetTime()), ImVec2(-FLT_MIN, 0.0f), progress >= 0.0f ? nullptr : "");
					if (ImGui::SmallButton("Cancel upload") && state.upload)
						state.upload->Cancel();
					break;
				}
				case Phase::Done:
				{
					if (!state.failure.empty())
					{
						ImGui::TextColored(Color(EditorTheme::Palette::Error), "%s", SEDX_ICON_TIMES_CIRCLE);
						ImGui::SameLine();
						ImGui::TextWrapped("%s", state.failure.c_str());
					}
					else if (state.result)
					{
						const GatewayUploadResult &result = *state.result;
						ImGui::TextColored(Color(result.Succeeded() ? EditorTheme::Palette::Accent : EditorTheme::Palette::Error), "%s  %s",
							result.Succeeded() ? SEDX_ICON_CHECK : SEDX_ICON_TIMES_CIRCLE, GetGatewayUploadOutcomeName(result.outcome));
						ImGui::TextWrapped("%s", result.message.c_str());
						if (!state.resultNote.empty())
							ImGui::TextWrapped("%s", state.resultNote.c_str());
						if (result.Succeeded() && state.prepared.server.official &&
							ImGui::SmallButton(EditorUI::IconLabel(SEDX_ICON_EXTERNAL_LINK, "Open the submission's page").c_str()))
							IO::FileSystem::OpenUrl(GetGatewaySceneryPageUrl(result.sceneryId));
					}
					break;
				}
			}
		}
	}

	// -------------------------------------------------------

	void Init()
	{
		State &state = GetState();
		if (state.initialized)
			return;
		state.initialized = true;
		ReadCommandLine(state);
	}

	void Tick(EditorContext *context)
	{
		State &state = GetState();
		if (!state.initialized)
			return;
		TickStartup(state, context);

		if (state.import && state.import->IsFinished())
		{
			const std::shared_ptr<GatewayRequest> request = std::move(state.import);
			state.import.reset();
			const int id = state.importId;
			state.importId = 0;
			if (!request->Succeeded() || !request->scenery)
				EDITOR_ERROR_TAG(LOG_TAG, "Could not download Gateway submission {}: {}", id, request->GetError());
			else if (EditorProjects::ImportGatewaySubmission(*request->scenery, request->m_Server, state.importContext) && !state.startupImportSave.empty())
			{
				const fs::path path = std::move(state.startupImportSave);
				state.startupImportSave.clear();
				EditorProjects::SaveAs(path);
			}
		}

		if (state.phase == Phase::Packaging && state.packaging.valid() &&
			state.packaging.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			PackResult packed = state.packaging.get();
			if (!packed.ok)
			{
				state.failure = "The airport couldn't be packed: " + packed.error;
				state.phase = Phase::Done;
				EDITOR_ERROR_TAG(LOG_TAG, "{}", state.failure);
			}
			else
			{
				state.packageBytes = packed.package.submission.masterZip.size();
				state.packageWarnings = std::move(packed.package.warnings);
				state.submission = std::make_shared<const GatewaySubmission>(std::move(packed.package.submission));
				EDITOR_INFO_TAG(LOG_TAG, "Packed {}: {} ({} files in the pack, DSF text {})", state.submission->icao, FormatBytes(state.packageBytes),
					packed.package.packFiles.size(), packed.package.dsfText.empty() ? "none" : FormatBytes(packed.package.dsfText.size()));
				if (state.autoMode)
					StartUpload(state);
				else
				{
					state.phase = Phase::Confirm;
					state.openConfirm = true;
				}
			}
		}

		if (state.phase == Phase::Uploading && state.upload && state.upload->IsFinished())
			FinishUpload(state);
	}

	void Draw(EditorContext *context)
	{
		State &state = GetState();
		if (!state.open)
			return;

		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(660.0f * dpi, 680.0f * dpi), ImGuiCond_FirstUseEver);
		if (state.focus)
		{
			ImGui::SetNextWindowFocus();
			state.focus = false;
		}
		if (!ImGui::Begin(WINDOW, &state.open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse))
		{
			ImGui::End();
			return;
		}

		EditorGateway::DrawServerBanner();
		const Prepared &prepared = state.prepared;
		const bool busy = state.phase == Phase::Packaging || state.phase == Phase::Uploading || state.phase == Phase::Confirm;
		if (!prepared.valid)
		{
			EditorUI::EmptyState(prepared.error.c_str());
			if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_REFRESH, "Check again").c_str()))
				Prepare(state);
			ImGui::End();
			return;
		}

		const GatewayServer server = EditorGateway::GetServer();
		if (!(server == prepared.server) && !busy)
			Prepare(state);

		ImGui::Text("%s  %s", prepared.airportId.c_str(), prepared.airport.name.c_str());
		ImGui::SameLine();
		ImGui::TextDisabled("(%s)", prepared.source.c_str());
		ImGui::Spacing();

		// The details scroll; the confirmation tick, the buttons and the progress stay at the bottom
		const float footer = ImGui::GetFrameHeightWithSpacing() * 3.0f + ImGui::GetTextLineHeightWithSpacing() * 3.0f;
		ImGui::BeginChild("##GatewaySubmitBody", ImVec2(0.0f, -footer));
		if (EditorUI::BeginPropertyGrid("##GatewaySubmission", 130.0f))
		{
			EditorUI::PropertyLabel("Server");
			ImGui::TextUnformatted(server.official ? "Public X-Plane Scenery Gateway" : server.apiUrl.c_str());

			EditorUI::PropertyLabel("Account");
			if (EditorGateway::HasCredentials())
				ImGui::TextUnformatted(EditorGateway::GetUserName().c_str());
			else
				ImGui::TextColored(Color(EditorTheme::Palette::Warning), "Not signed in");
			ImGui::SameLine();
			if (ImGui::SmallButton(EditorUI::IconLabel(SEDX_ICON_COG, "Account...").c_str()))
				EditorPreferencesWindow::Open("Scenery Gateway");

			EditorUI::PropertyLabel("Type");
			ImGui::Text("%s  (gui_label %s)", prepared.summary.is3D ? "3D" : "2D", prepared.summary.is3D ? "3D" : "2D");
			EditorUI::ItemTooltip("3D: the airport places objects, facades, forests, object strings or jetways. The upload's gui_label is set to match.");

			EditorUI::PropertyLabel("Features");
			const std::vector<std::string> features = prepared.summary.GetFeatureNames();
			std::string featureText;
			for (const std::string &feature : features)
				featureText += (featureText.empty() ? "" : ", ") + feature;
			ImGui::TextUnformatted(featureText.empty() ? "None" : featureText.c_str());

			EditorUI::PropertyLabel("Content");
			ImGui::TextWrapped("%s", DescribeContent(prepared.summary).c_str());

			EditorUI::PropertyLabel("Parent");
			if (prepared.parentId > 0)
				ImGui::Text("Updates submission %d", prepared.parentId);
			else
				ImGui::TextUnformatted("None: a first upload of this airport from here");
			EditorUI::ItemTooltip(prepared.fromProject ? "The project remembers the submission each airport came from (or was last uploaded as) on each server."
				: "Airports opened from a scenery pack have no project to remember their scenery id.");

			EditorUI::PropertyLabel("Checked against");
			ImGui::TextUnformatted(prepared.validatedAgainst.empty() ? "Unknown X-Plane build" : ("X-Plane " + prepared.validatedAgainst).c_str());
			EditorUI::EndPropertyGrid();
		}

		ImGui::SeparatorText("Validation");
		DrawProblems(prepared.validation);
		ImGui::BeginDisabled(busy);
		if (ImGui::SmallButton(EditorUI::IconLabel(SEDX_ICON_REFRESH, "Check again").c_str()))
		{
			Prepare(state);
			ResetUpload(state);
		}
		ImGui::EndDisabled();

		ImGui::SeparatorText("Comments");
		ImGui::BeginDisabled(busy);
		ImGui::InputTextMultiline("##GatewayComments", state.comments, sizeof(state.comments), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 5.0f));
		ImGui::EndDisabled();
		EditorUI::ItemTooltip("What changed in this submission: the moderators and everyone who downloads it read this (it goes in the pack's README too).");
		ImGui::EndChild();

		const size_t errors = prepared.validation.CountErrors();
		const size_t warnings = prepared.validation.CountWarnings();
		if (warnings > 0)
		{
			ImGui::BeginDisabled(busy || errors > 0);
			ImGui::Checkbox("I've read the warnings above and want to upload anyway", &state.warningsAccepted);
			ImGui::EndDisabled();
		}

		// What stops the upload, in the order the artist can fix it
		std::string blocker;
		if (errors > 0)
			blocker = "Fix the validation errors first.";
		else if (warnings > 0 && !state.warningsAccepted)
			blocker = "Confirm the warnings first.";
		else if (!EditorGateway::HasCredentials())
			blocker = "Set your Gateway user name and password first (Account...).";
		else if (std::string reason; !CanUploadTo(server, GatewayClient::AreLiveUploadsAllowed(), &reason))
			blocker = reason;

		ImGui::Spacing();
		ImGui::BeginDisabled(busy || !blocker.empty());
		if (ImGui::Button(EditorUI::IconLabel(SEDX_ICON_CLOUD_UPLOAD, "Upload...").c_str(), ImVec2(140.0f * dpi, 0.0f)))
			StartPackaging(state);
		ImGui::EndDisabled();
		if (!blocker.empty())
		{
			ImGui::SameLine();
			ImGui::TextColored(Color(EditorTheme::Palette::TextLabel), "%s", blocker.c_str());
		}
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, ImGui::GetContentRegionAvail().x - 90.0f * dpi));
		if (ImGui::Button("Close", ImVec2(90.0f * dpi, 0.0f)))
			state.open = false;

		ImGui::Spacing();
		DrawProgress(state);
		DrawConfirmPopup(state);
		ImGui::End();

		if (!state.open && state.upload && !state.upload->IsFinished())
			state.open = true;	// Keep the result visible: an upload in flight can't be abandoned silently
		(void)context;
	}

	void Open(EditorContext *context)
	{
		State &state = GetState();
		state.projectAirport.clear();
		state.open = true;
		state.focus = true;
		if (state.phase != Phase::Packaging && state.phase != Phase::Uploading)
		{
			ResetUpload(state);
			Prepare(state);
		}
		(void)context;
	}

	void OpenForProjectAirport(const std::string &airportId, EditorContext *context)
	{
		State &state = GetState();
		if (state.phase == Phase::Packaging || state.phase == Phase::Uploading)
		{
			state.open = true;
			state.focus = true;
			return;
		}
		if (ProjectFile::ProjectDocument *project = EditorProjects::GetDocument(); project && project->IsOpen() && project->FindAirportIndex(airportId) != ProjectFile::ProjectDocument::NO_INDEX)
			EditorProjects::ShowAirport(airportId, context);
		state.projectAirport = airportId;
		state.open = true;
		state.focus = true;
		ResetUpload(state);
		Prepare(state);
	}

	bool CanOpen()
	{
		return EditorAirports::GetDocument() != nullptr;
	}

	void ImportSubmission(const int sceneryId, EditorContext *context)
	{
		State &state = GetState();
		if (state.import && !state.import->IsFinished())
		{
			EDITOR_WARN_TAG(LOG_TAG, "A Gateway submission is already being imported ({})", state.importId);
			return;
		}
		state.import = GatewayClient::FetchScenery(sceneryId);
		state.importId = sceneryId;
		state.importContext = context;
		EDITOR_INFO_TAG(LOG_TAG, "Downloading Gateway submission {} from {} to import it into the project", sceneryId, EditorGateway::GetServer().GetKey());
	}

	int GetImportInProgress()
	{
		const State &state = GetState();
		return state.import && !state.import->IsFinished() ? state.importId : 0;
	}
}

// -------------------------------------------------------

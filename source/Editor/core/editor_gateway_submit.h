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
 * editor_gateway_submit.h
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct EditorContext;
}

namespace SceneryEditorX::EditorGatewaySubmit
{
	/**
	 * Submitting an airport to the X-Plane Scenery Gateway, and bringing Gateway submissions into the project. Main
	 * thread only.
	 *
	 * The Submit dialog works on the airport the airport editor shows: a project airport (its rows and the items in its
	 * hierarchy; the project keeps the scenery id each upload descends from) or an airport opened from a scenery pack (its
	 * rows and the pack's DSF tiles). It shows the 2D / 3D type and features, the parent submission, WorldEditor-level
	 * validation (errors block the upload, warnings need a tick), the artist's comments, a confirmation naming the
	 * server, the upload's progress and the answer. Uploads go through EditorGateway's account and
	 * GatewayClient::UploadScenery, so the public Gateway refuses them until live uploads are switched on.
	 *
	 * Command line, for testing against a Gateway test server (scripts/gateway_mock):
	 *   --gateway-import <scenery id>      Download a submission and add it to the project (a new one when none is open)
	 *   --gateway-import-save <file.edX>   Save the project there after the import
	 *   --gateway-submit <airport id>      Open the Submit dialog for a project airport (after any import)
	 *   --gateway-submit-auto <user name>  With --gateway-submit: validate, package and upload without the dialog, as this
	 *                                      account with the password in SEDX_GATEWAY_PASSWORD; refused for the public
	 *                                      Gateway. The project is saved afterwards when it has a file.
	 *   --gateway-comments <text>          The artist's comments for --gateway-submit
	 */

	/* @brief Reads the command line. Call after EditorGateway::Init() and EditorProjects::Init(). */
	void Init();

	/* @brief Once per frame: startup imports / submissions, downloads, packaging and uploads in flight. */
	void Tick(EditorContext *context);

	/* @brief The Submit dialog. Call every frame from the layer. */
	void Draw(EditorContext *context);

	/* @brief Opens the Submit dialog for the airport the airport editor shows. */
	void Open(EditorContext *context);

	/* @brief Opens the Submit dialog for an airport of the open project. */
	void OpenForProjectAirport(const std::string &airportId, EditorContext *context);

	/* @brief True when the airport editor shows an airport that can be submitted. */
	bool CanOpen();

	/* @brief Downloads a submission and adds it to the open project (EditorProjects::ImportGatewaySubmission). */
	void ImportSubmission(int sceneryId, EditorContext *context);

	/* @brief The submission being downloaded for an import, 0 when none. */
	int GetImportInProgress();
}

// -------------------------------------------------------

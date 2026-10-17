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
 * editor_camera.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "editor_camera.h"
#include <cmath>
#include <SceneryEditorX/scene/scene.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorCamera
{
	namespace
	{
		struct HomeView
		{
			bool captured = false;
			CameraMode mode = ORBIT;
			Vec3 eye;
			Vec3 center;
			Vec3 rotation;
			float orbitDistance = 10.0f;
		};

		HomeView s_Home;

		/* Unit view direction for the camera's pitch (x) / yaw (y) angles, as used by the orbit math in Camera::Tick. */
		Vec3 Direction(const Vec3 &rotation)
		{
			const float cosPitch = std::cos(rotation.x);
			return { cosPitch * std::sin(rotation.y), std::sin(rotation.x), cosPitch * std::cos(rotation.y) };
		}
	}

	Camera *Get()
	{
		return Scene::GetCamera();
	}

	void CaptureHome()
	{
		const Camera *camera = Get();
		if (!camera)
			return;

		s_Home.captured			= true;
		s_Home.mode				= camera->mode;
		s_Home.eye				= camera->eye;
		s_Home.center			= camera->center;
		s_Home.rotation			= camera->rotation;
		s_Home.orbitDistance	= camera->GetOrbitDistance();
	}

	void Reset()
	{
		Camera *camera = Get();
		if (!camera || !s_Home.captured)
			return;

		camera->mode		= s_Home.mode;
		camera->eye			= s_Home.eye;
		camera->center		= s_Home.center;
		camera->rotation	= s_Home.rotation;
		camera->SetOrbitDistance(s_Home.orbitDistance);
	}

	void SetMode(const CameraMode mode)
	{
		Camera *camera = Get();
		if (!camera || camera->mode == mode)
			return;

		// Orbit places the eye at center + distance * direction(rotation); put the center where that
		// keeps the current eye so switching modes never makes the view jump.
		if (mode == ORBIT)
			camera->center = camera->eye - Direction(camera->rotation) * camera->GetOrbitDistance();

		camera->mode = mode;
	}

	bool IsMode(const CameraMode mode)
	{
		const Camera *camera = Get();
		return camera && camera->mode == mode;
	}

	void SetProjection(const CameraType projection)
	{
		Camera *camera = Get();
		if (!camera)
			return;

		camera->cameraType = projection;
		camera->SetProjection(projection);
	}

	bool IsProjection(const CameraType projection)
	{
		const Camera *camera = Get();
		return camera && camera->GetProjectionType() == projection;
	}

	const char *GetModeName()
	{
		const Camera *camera = Get();
		if (!camera)
			return "None";

		switch (camera->mode)
		{
			case ORBIT:	return "Orbit";
			case FLY:	return "Fly";
			case FREE:	return "Free";
			case FIXED:	return "Fixed";
			default:	return "None";
		}
	}

}

// -------------------------------------------------------

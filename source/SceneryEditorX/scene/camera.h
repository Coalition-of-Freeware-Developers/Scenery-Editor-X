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
 * camera.h
 * -------------------------------------------------------
 * Created: 16/4/2025
 * -------------------------------------------------------
 */
#pragma once
#include "node.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

    ///struct Serializer;

    enum CameraMode : uint8_t
	{
        NONE,
	    ORBIT,
	    FLY
	};

    // -------------------------------------------------------
	
	enum class CameraType : uint8_t
	{
	    Perspective,
	    Orthographic
	};

    // -------------------------------------------------------

    class CameraNode /*: public Node*/
    {
	public:
        CameraNode();
        //virtual void Serialize(Serializer &ser) override;

	    inline static const char* modeNames[] = { "ORBIT", "FLY" };
	    inline static const char* typeNames[] = { "Perspective", "Orthographic" };

		// -------------------------------------------------------

	    CameraType cameraType = CameraType::Perspective;
	    CameraMode mode = ORBIT;
	
	    Vec3 eye = Vec3(0);
	    Vec3 center = Vec3(0);
        Vec2 extent = Vec2(1.0f);
	    Vec3 rotation = Vec3(0);

		// -------------------------------------------------------

	    bool useJitter = true;
	    float zoom = 10.0f;
	
	    float farDistance = 1000.0f;
	    float nearDistance = 0.01f;
	    float horizontalFov = 60.0f;

	    float orthoFarDistance = 10.0f;
	    float orthoNearDistance = -100.0f;

	    Mat4 GetView();
	    Mat4 GetProj();
	    Mat4 GetProjJittered();
	    Mat4 GetProj(float zNear, float zFar);
	    Vec2 GetJitter();
	    void NextJitter();

	private:
	    Vec2 jitter = Vec2(0);
	    uint32_t jitterIndex = 0;

		friend class AssetManager;
	};

}

// -------------------------------------------------------

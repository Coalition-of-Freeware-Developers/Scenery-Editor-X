/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* camera.h
* -------------------------------------------------------
* Created: 16/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/scene/node.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    ///struct Serializer;

    enum CameraMode : uint8_t
	{
	    Orbit,
	    Fly
	};

    /// -------------------------------------------------------
	
	enum class CameraType : uint8_t
	{
	    Perspective,
	    Orthographic
	};

    /// -------------------------------------------------------

    class CameraNode : public Node
    {
	public:
        CameraNode();
        //virtual void Serialize(Serializer &ser) override;

	    inline static const char* modeNames[] = { "Orbit", "Fly" };
	    inline static const char* typeNames[] = { "Perspective", "Orthographic" };

		/// -------------------------------------------------------

	    CameraType cameraType = CameraType::Perspective;
	    CameraMode mode = Orbit;
	
	    Vec3 eye = Vec3(0);
	    Vec3 center = Vec3(0);
        Vec2 extent = Vec2(1.0f);
	    Vec3 rotation = Vec3(0);

		/// -------------------------------------------------------

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

/// -------------------------------------------------------

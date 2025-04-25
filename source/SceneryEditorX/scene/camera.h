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
#include <SceneryEditorX/scene/asset.h>
#include <SceneryEditorX/scene/node.h>

// -----------------------------------------

namespace SceneryEditorX
{
	enum CameraMode : uint8_t
	{
	    Orbit,
	    Fly
	};
	
	enum class CameraType : uint8_t
	{
	    Perspective,
	    Orthographic
	};

	class CameraNode : Node
    {
	public:
        CameraNode();
        virtual void Serialize(Serializer &ser) override;

	    inline static const char* modeNames[] = { "Orbit", "Fly" };
	    inline static const char* typeNames[] = { "Perspective", "Orthographic" };

		// -------------------------------------------------------

	    CameraType cameraType = CameraType::Perspective;
	    CameraMode mode = Orbit;
	
	    glm::vec3 eye = glm::vec3(0);
	    glm::vec3 center = glm::vec3(0);
	    glm::vec3 rotation = glm::vec3(0);

		// -------------------------------------------------------

	    bool useJitter = true;
	    float zoom = 10.0f;
	
	    float farDistance = 1000.0f;
	    float nearDistance = 0.01f;
	    float horizontalFov = 60.0f;
	
	    float orthoFarDistance = 10.0f;
	    float orthoNearDistance = -100.0f;
	
	    glm::vec2 extent = glm::vec2(1.0f);

	    glm::mat4 GetView();
	    glm::mat4 GetProj();
	    glm::mat4 GetProjJittered();
	    glm::mat4 GetProj(float zNear, float zFar);
	
	    glm::vec2 GetJitter();
	    void NextJitter();
	private:
	    glm::vec2 jitter = glm::vec2(0);
	    uint32_t jitterIndex = 0;

		friend class AssetManager;
	};

} // namespace SceneryEditorX

// -------------------------------------------------------

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
 * gizmos.h
 * -------------------------------------------------------
 * Created: 15/03/2026
 * -------------------------------------------------------
 */
#include "gizmos.h"
#include <imgui.h>
#include <vector>
#include <SceneryEditorX/core/input/input.h>
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/scene.h>
#include <xMath/includes/vector.h>

// -------------------------------------------------------

const xMath::Vec3 SNAP = xMath::Vec3(0.1f, 0.1f, 0.1f);

static bool s_FirstUse = true;
static std::vector<SceneryEditorX::Entity *> s_EntitiesBeingTransformed;
static std::vector<xMath::Vec3> s_PositionsPrevious;
static std::vector<xMath::Quat> s_RotationsPrevious;
static std::vector<xMath::Vec3> s_ScalesPrevious;

/**
 * @brief 
 * @param position  
 * @param rotation 
 * @param scale 
 * @return 
 */
static xMath::Matrix CreateRowMajorMatrix(const xMath::Vec3& position, const xMath::Quat& rotation, const xMath::Vec3& scale)
{
	const xMath::Matrix rotationMatrix = xMath::Matrix::CreateRotation(rotation).Transposed();

	return {
	    scale.x * rotationMatrix.m00, scale.y * rotationMatrix.m01, scale.z * rotationMatrix.m02, position.x,
		scale.x * rotationMatrix.m10, scale.y * rotationMatrix.m11, scale.z * rotationMatrix.m12, position.y,
		scale.x * rotationMatrix.m20, scale.y * rotationMatrix.m21, scale.z * rotationMatrix.m22, position.z,
		0.0f,                    0.0f                   , 0.0f,                    1.0f
	};
}

Gizmo::Gizmo(ManipulatorType type)
{
}

void Gizmo::Tick()
{
	SceneryEditorX::Camera * camera = SceneryEditorX::Scene::GetCamera();
	if (!camera)
	{
	    return;
	}

	// get selected entities
	const std::vector<SceneryEditorX::Entity *> & selectedEntities = camera->GetSelectedEntities();
	if (selectedEntities.empty())
	{
	    return;
	}
		
	// use the first entity as the primary for rotation/scale reference
	SceneryEditorX::Entity *primaryEntity = selectedEntities[0];
	if (!primaryEntity)
	{
		return;
	}
	
	// switch between position, rotation and scale operations, with W, E and R respectively
	static ImGuizmo::OPERATION transformOperation = ImGuizmo::TRANSLATE;
	if (!camera->GetFlag(SceneryEditorX::CameraFlags::IsControlled))
	{
		if (SceneryEditorX::Input::IsKeyDown(SceneryEditorX::KeyCode::W))
		{
			transformOperation = ImGuizmo::TRANSLATE;
		}
		else if (SceneryEditorX::Input::IsKeyDown(SceneryEditorX::KeyCode::E))
		{
			transformOperation = ImGuizmo::ROTATE;
		}
		else if (SceneryEditorX::Input::IsKeyDown(SceneryEditorX::KeyCode::R))
		{
			transformOperation = ImGuizmo::SCALE;
		}
	}
	
	// get matrices
	const xMath::Mat4& matrixView       = camera->GetViewMatrix().GetTranspose();
	const xMath::Mat4& matrixProjection = camera->GetProjectionMatrix().GetTranspose();
	
	// begin
	const bool isOrthographic = false;
	ImGuizmo::SetOrthographic(isOrthographic);
	ImGuizmo::BeginFrame();
	
	// calculate center position of all selected entities for gizmo placement
	static bool useWorldSpace = true;
	xMath::Vec3 centerPosition = xMath::Vec3::Zero;
	uint32_t validEntityCount = 0;
	for (SceneryEditorX::Entity* entity : selectedEntities)
	{
	    if (entity)
		{
			centerPosition += useWorldSpace ? entity->GetPosition() : entity->GetPositionLocal();
			validEntityCount++;
		}
	}
	if (validEntityCount > 0)
	{
		centerPosition /= static_cast<float>(validEntityCount);
	}
	
	// use center position for gizmo, but primary entity's rotation/scale for orientation
	xMath::Vec3 position        = centerPosition;
	xMath::Quat rotation		= useWorldSpace ? primaryEntity->GetRotation() : primaryEntity->GetRotationLocal();
	xMath::Vec3 scale           = useWorldSpace ? primaryEntity->GetScale() : primaryEntity->GetScaleLocal();
	xMath::Matrix transformMatrix = CreateRowMajorMatrix(position, rotation, scale);
	
	// save the initial position for delta calculation
	xMath::Vec3 initialPosition = position;
	xMath::Quat initialRotation = rotation;
	xMath::Vec3 initialScale = scale;
	
	// set viewport rectangle
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
	ImGuizmo::Manipulate(
		&matrixView.m00,
		&matrixProjection.m00,
		transformOperation,
		useWorldSpace ? ImGuizmo::WORLD : ImGuizmo::LOCAL,
		&transformMatrix.m00,
		nullptr,
		&SNAP.x
	);
	
	// map imguizmo to transform
	if (ImGuizmo::IsUsing())
	{
		// start of handling - save the initial transforms for all entities
		if (s_FirstUse)
		{
			s_EntitiesBeingTransformed.clear();
			s_PositionsPrevious.clear();
			s_RotationsPrevious.clear();
			s_ScalesPrevious.clear();
			
			for (SceneryEditorX::Entity* entity : selectedEntities)
			{
				if (entity)
				{
					s_EntitiesBeingTransformed.push_back(entity);
					s_PositionsPrevious.push_back(useWorldSpace ? entity->GetPosition() : entity->GetPositionLocal());
					s_RotationsPrevious.push_back(useWorldSpace ? entity->GetRotation() : entity->GetRotationLocal());
					s_ScalesPrevious.push_back(useWorldSpace ? entity->GetScale() : entity->GetScaleLocal());
				}
			}
			s_FirstUse = false;
		}
	
		transformMatrix.Transposed().Decompose(scale, rotation, position);
		
		// calculate deltas from primary entity
		xMath::Vec3 positionDelta = position - initialPosition;
		xMath::Quat rotationDelta = rotation * initialRotation.Inverse();
		xMath::Vec3 scaleRatio = xMath::Vec3(
			initialScale.x != 0.0f ? scale.x / initialScale.x : 1.0f,
			initialScale.y != 0.0f ? scale.y / initialScale.y : 1.0f,
			initialScale.z != 0.0f ? scale.z / initialScale.z : 1.0f
		);
		
		// apply transforms to all selected entities
		for (SceneryEditorX::Entity* entity : selectedEntities)
		{
			if (!entity)
				continue;
				
			if (useWorldSpace)
			{
				// for translation, apply the delta
				entity->SetPosition(entity->GetPosition() + positionDelta);

				// for rotation, apply the rotation delta
				if (transformOperation == ImGuizmo::ROTATE)
				{
					entity->SetRotation(rotationDelta * entity->GetRotation());
				}

				// for scale, apply the ratio
				if (transformOperation == ImGuizmo::SCALE)
				{
					xMath::Vec3 current_scale = entity->GetScale();
					entity->SetScale(xMath::Vec3(
						current_scale.x * scaleRatio.x,
						current_scale.y * scaleRatio.y,
						current_scale.z * scaleRatio.z
					));
				}
			}
			else
			{
				entity->SetPositionLocal(entity->GetPositionLocal() + positionDelta);

				if (transformOperation == ImGuizmo::ROTATE)
				{
					entity->SetRotationLocal(rotationDelta * entity->GetRotationLocal());
				}

				if (transformOperation == ImGuizmo::SCALE)
				{
					xMath::Vec3 current_scale = entity->GetScaleLocal();
					entity->SetScaleLocal(xMath::Vec3(
						current_scale.x * scaleRatio.x,
						current_scale.y * scaleRatio.y,
						current_scale.z * scaleRatio.z
					));
				}
			}
		}
	
		// end of handling - add transforms to the command stack for all entities as a single undo operation
		if (SceneryEditorX::Input::IsKeyReleased(SceneryEditorX::KeyCode::Click_Left))
		{
			if (!s_EntitiesBeingTransformed.empty())
			{
				SceneryEditorX::CommandStack::Add<SceneryEditorX::CommandTransformMulti>(s_EntitiesBeingTransformed, s_PositionsPrevious, s_RotationsPrevious, s_ScalesPrevious);
			}
			s_FirstUse = true;
		}
	}
}

void Gizmo::DrawGizmo(const xMath::Vec3 &position, const xMath::Vec3 &rotation, const xMath::Vec3 &scale)
{
}

bool Gizmo::AllowObjectSelection()
{
	return !ImGuizmo::IsOver() && !ImGuizmo::IsUsing();
}

bool Gizmo::EnableSnapping()
{
	return false;
}

// -------------------------------------------------------

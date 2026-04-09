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
#include <vector>
#include <SceneryEditorX/core/input/input.h>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui.h>
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/scene.h>
#include <xMath/includes/mat4.h>
#include <xMath/includes/math_utils.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	const Vec3 SNAP = xMath::Vec3(0.1f, 0.1f, 0.1f);
	ImGuizmo::MODE s_TransformMode = ImGuizmo::WORLD;
	GizmoPivotMode s_PivotMode = GizmoPivotMode::Center;
	bool s_EnableSnapping = false;

	Vec3 GetEntityPosition(Entity* entity)
	{
		if (!entity)
		{
			return xMath::Vec3(0.0f, 0.0f, 0.0f);
		}

		return s_TransformMode == ImGuizmo::WORLD ? entity->GetPosition() : entity->GetPositionLocal();
	}

	Vec3 GetEntityRotation(Entity* entity)
	{
		if (!entity)
		{
			return xMath::Vec3(0.0f, 0.0f, 0.0f);
		}

		return s_TransformMode == ImGuizmo::WORLD ? entity->GetRotation() : entity->GetRotationLocal();
	}

	Vec3 GetEntityScale(Entity* entity)
	{
		if (!entity)
		{
			return xMath::Vec3(1.0f, 1.0f, 1.0f);
		}

		return s_TransformMode == ImGuizmo::WORLD ? entity->GetScale() : entity->GetScaleLocal();
	}

	Vec3 ComputePivotPosition(const std::vector<Entity*>& selectedEntities, Entity* primaryEntity)
	{
		if (!primaryEntity)
		{
			return xMath::Vec3(0.0f, 0.0f, 0.0f);
		}

		if (s_PivotMode == GizmoPivotMode::World)
		{
			return GetEntityPosition(primaryEntity);
		}

		if (s_PivotMode == GizmoPivotMode::Local)
		{
			return GetEntityPosition(primaryEntity) + primaryEntity->GetPivotPoint();
		}

		Vec3 centerPosition = xMath::Vec3(0.0f, 0.0f, 0.0f);
		uint32_t validEntityCount = 0;
		for (Entity* entity : selectedEntities)
		{
			if (!entity)
			{
				continue;
			}

			centerPosition += GetEntityPosition(entity);
			validEntityCount++;
		}

		if (validEntityCount > 0)
		{
			centerPosition /= static_cast<float>(validEntityCount);
		}

		return centerPosition;
	}
	
	
	Gizmo::Gizmo(ManipulatorType type)
	{
	   m_Type = type;
	}
	
	void Gizmo::Tick()
	{
		Camera * camera = Scene::GetCamera();
		if (!camera)
		{
			return;
		}
	
		const std::vector<Entity*>& selectedEntities = camera->GetSelectedEntities();
		if (selectedEntities.empty())
		{
			return;
		}
			
		// use the first entity as the primary for rotation/scale reference
		Entity *primaryEntity = selectedEntities[0];
		if (!primaryEntity)
		{
			return;
		}
		
		// switch between position, rotation and scale operations, with W, E and R respectively
		static ImGuizmo::OPERATION transformOperation = ImGuizmo::TRANSLATE;
		if (!camera->IsControlled())
		{
			if (Input::IsKeyPressed(KeyCode::Q))
			{
				s_TransformMode = s_TransformMode == ImGuizmo::WORLD ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
			}
	
			if (Input::IsKeyPressed(KeyCode::P))
			{
				s_PivotMode = s_PivotMode == GizmoPivotMode::Center ? GizmoPivotMode::Local : static_cast<GizmoPivotMode>(static_cast<uint8_t>(s_PivotMode) + 1);
			}
	
			if (Input::IsKeyDown(KeyCode::W))
			{
				transformOperation = ImGuizmo::TRANSLATE;
			}
			else if (Input::IsKeyDown(KeyCode::E))
			{
				transformOperation = ImGuizmo::ROTATE;
			}
			else if (Input::IsKeyDown(KeyCode::R))
			{
				transformOperation = ImGuizmo::SCALE;
			}
		}
		
		const xMath::Mat4 matrixView = camera->GetViewMatrix().GetTranspose();
		const xMath::Mat4 matrixProjection = camera->GetProjectionMatrix().GetTranspose();
		
		// begin
		const bool isOrthographic = false;
		ImGuizmo::SetOrthographic(isOrthographic);
		ImGuizmo::BeginFrame();
		
		Vec3 position = ComputePivotPosition(selectedEntities, primaryEntity);
		Vec3 rotation = GetEntityRotation(primaryEntity);
		Vec3 scale = GetEntityScale(primaryEntity);
		xMath::Mat4 transformMatrix =
			xMath::Mat4::Translate(position)
			* xMath::Mat4::RotationRadians(rotation)
			* xMath::Mat4::Scale(scale);
		
		// save the initial position for delta calculation
		Vec3 initialPosition = position;
		Vec3 initialRotation = rotation;
		Vec3 initialScale = scale;
		
		// set viewport rectangle
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
		ImGuizmo::Manipulate(
			matrixView.Data(),
			matrixProjection.Data(),
			transformOperation,
		  s_TransformMode,
			transformMatrix.Data(),
			nullptr,
		 s_EnableSnapping ? &SNAP.x : nullptr
		);
		
		// map imguizmo to transform
		if (ImGuizmo::IsUsing())
		{
			float matrixTranslation[3] = {0.0f, 0.0f, 0.0f};
			float matrixRotationDegrees[3] = {0.0f, 0.0f, 0.0f};
			float matrixScale[3] = {1.0f, 1.0f, 1.0f};
			ImGuizmo::DecomposeMatrixToComponents(transformMatrix.Data(), matrixTranslation, matrixRotationDegrees, matrixScale);
	
			position = xMath::Vec3(matrixTranslation[0], matrixTranslation[1], matrixTranslation[2]);
			rotation = ToRadians(xMath::Vec3(matrixRotationDegrees[0], matrixRotationDegrees[1], matrixRotationDegrees[2]));
			scale = xMath::Vec3(matrixScale[0], matrixScale[1], matrixScale[2]);
			
			Vec3 positionDelta = position - initialPosition;
			Vec3 rotationDelta = rotation - initialRotation;
			Vec3 scaleRatio = xMath::Vec3(
				initialScale.x != 0.0f ? scale.x / initialScale.x : 1.0f,
				initialScale.y != 0.0f ? scale.y / initialScale.y : 1.0f,
				initialScale.z != 0.0f ? scale.z / initialScale.z : 1.0f
			);
			
			for (Entity* entity : selectedEntities)
			{
				if (!entity)
					continue;

				if (s_TransformMode == ImGuizmo::WORLD)
				{
				    entity->SetPosition(entity->GetPosition() + positionDelta);
	
					if (transformOperation == ImGuizmo::ROTATE)
					{
					 entity->SetRotation(entity->GetRotation() + rotationDelta);
					}
	
					if (transformOperation == ImGuizmo::SCALE)
					{
					    Vec3 currentScale = entity->GetScale();
						entity->SetScale(xMath::Vec3(
						 currentScale.x * scaleRatio.x,
							currentScale.y * scaleRatio.y,
							currentScale.z * scaleRatio.z
						));
					}
				}
				else
				{
					entity->SetPositionLocal(entity->GetPositionLocal() + positionDelta);
	
					if (transformOperation == ImGuizmo::ROTATE)
					{
					   entity->SetRotationLocal(entity->GetRotationLocal() + rotationDelta);
					}
	
					if (transformOperation == ImGuizmo::SCALE)
					{
						Vec3 currentScale = entity->GetScaleLocal();
						entity->SetScaleLocal(xMath::Vec3(
						 currentScale.x * scaleRatio.x,
							currentScale.y * scaleRatio.y,
							currentScale.z * scaleRatio.z
						));
					}
				}
			}
		}
	}
	
	void Gizmo::DrawGizmo(const Vec3 &position, const Vec3 &rotation, const Vec3 &scale)
	{
	}
	
	bool Gizmo::AllowObjectSelection()
	{
		return !ImGuizmo::IsOver() && !ImGuizmo::IsUsing();
	}
	
	bool Gizmo::EnableSnapping()
	{
	   return s_EnableSnapping;
	}
	
	void Gizmo::SetPivotMode(const GizmoPivotMode mode)
	{
		s_PivotMode = mode;
	}
	
	GizmoPivotMode Gizmo::GetPivotMode()
	{
		return s_PivotMode;
	}
	
	void Gizmo::SetTransformMode(const ImGuizmo::MODE mode)
	{
		s_TransformMode = mode;
	}
	
	ImGuizmo::MODE Gizmo::GetTransformMode()
	{
		return s_TransformMode;
	}

}

// -------------------------------------------------------

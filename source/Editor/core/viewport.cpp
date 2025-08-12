/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* viewport.cpp
* -------------------------------------------------------
* Created: 1/8/2025
* -------------------------------------------------------
*/
#include <Editor/core/editor.h>
#include <Editor/core/selection_manager.h>
#include <Editor/core/viewport.h>
#include <Editor/ui/resources/editor_resources.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>
#include <SceneryEditorX/asset/asset_types.h>
#include <SceneryEditorX/asset/managers/asset_manager.h>
#include <SceneryEditorX/core/input/input.h>
#include <SceneryEditorX/renderer/2d_renderer.h>
#include <SceneryEditorX/renderer/scene/scene_renderer.h>
#include <SceneryEditorX/scene/components.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/utils/math/aabb.h>
#include <SceneryEditorX/utils/math/math_utils.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	Viewport::Viewport(std::string viewportName, Editor *editor) :
        m_Editor(editor), m_ViewportName(std::move(viewportName)), m_ViewportCamera(45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f)
    {
    }

    const std::string & Viewport::GetName() const
	{
        return m_ViewportName;
    }

	bool Viewport::IsViewportVisible() const
	{
        return m_IsVisible;
	}

	bool Viewport::IsMainViewport() const
	{
        return m_IsMainViewport;
	}

	Ref<SceneRenderer> Viewport::GetRenderer() const
	{
        return m_ViewportRenderer;
	}

	Ref<Renderer2D> Viewport::GetRenderer2D() const
	{
        return m_ViewportRenderer2D;
	}

    EditorCamera & Viewport::GetViewportCamera()
    {
        return m_ViewportCamera;
    }

    std::array<Vec2, 2> Viewport::GetViewportBounds() const
	{
        return m_ViewportBounds;
	}

	void Viewport::Init(const Ref<Scene> &scene)
	{
        m_ViewportRenderer = CreateRef<SceneRenderer>(scene);
        m_ViewportRenderer2D = Ref<Renderer2D>();

        /// Set line width for both renderers
        m_ViewportRenderer2D->SetLineWidth(m_LineWidth);
        /// SceneRenderer needs to have a SetLineWidth method implemented
        // m_ViewportRenderer->SetLineWidth(m_LineWidth);
	}

	void Viewport::SetIsMainViewport(bool isMain)
	{
        m_IsMainViewport = isMain;
	}

	void Viewport::SetIsVisible(bool visible)
	{
        m_IsVisible = visible;
	}

	void Viewport::OnUpdate(DeltaTime dt)
	{
	    if (m_IsVisible)
		{
			m_ViewportCamera.SetActive(m_IsFocused);
			m_ViewportCamera.OnUpdate(dt);

			if (m_Editor->m_CurrentScene != m_Editor->m_EditorScene)
			{
				if (!m_IsMainViewport)
                    m_Editor->m_CurrentScene->OnRenderEditor(m_ViewportRenderer, m_ViewportCamera);
            }
			else
			{
				m_Editor->m_EditorScene->OnRenderEditor(m_ViewportRenderer, m_ViewportCamera);
			}

			OnRender2D();
		}
	}

	void Viewport::OnRender2D()
	{
        if (!m_ViewportRenderer || !m_ViewportRenderer->GetFinalPassImage())
            return;

        m_ViewportRenderer2D->BeginScene(m_ViewportCamera.GetViewProjection(), m_ViewportCamera.GetViewMatrix());
        m_ViewportRenderer2D->SetTargetFramebuffer(m_ViewportRenderer->GetExternalCompositeFramebuffer());

		/*
		if (m_DrawOnTopBoundingBoxes && m_ShowBoundingBoxes)
		{
			if (m_ShowBoundingBoxSelectedMeshOnly)
			{
                for (const auto& selectedEntities = SelectionManager::GetSelections(m_Editor->m_CurrentScene->GetUUID()); const auto& entityID : selectedEntities)
				{
					Entity entity = m_Editor->m_CurrentScene->GetEntityWithUUID(entityID);

					if (const auto mc = entity.TryGetComponent<SubmeshComponent>(); mc)
					{
						if(auto mesh = AssetManager::GetAsset<Mesh>(mc->Mesh); mesh)
						{
							if (auto meshSource = AssetManager::GetAsset<MeshSource>(mesh->GetMeshSource()); meshSource)
							{
								if (m_ShowBoundingBoxSubmeshes)
								{
									const auto& submeshIndices = mesh->GetSubmeshes();
									const auto& submeshes = meshSource->GetSubmeshes();

									for (uint32_t submeshIndex : submeshIndices)
									{
										Mat4 transform = m_Editor->m_CurrentScene->GetWorldSpaceTransformMatrix(entity);
										const Utils::AABB& aabb = submeshes[submeshIndex].BoundingBox;
										m_ViewportRenderer2D->DrawAABB(aabb, transform * submeshes[submeshIndex].Transform, Vec4{ 1.0f, 1.0f, 1.0f, 1.0f });
									}
								}
								else
								{
									Mat4 transform = m_Editor->m_CurrentScene->GetWorldSpaceTransformMatrix(entity);
									const Utils::AABB& aabb = meshSource->GetBoundingBox();
									m_ViewportRenderer2D->DrawAABB(aabb, transform, Vec4{ 1.0f, 1.0f, 1.0f, 1.0f });
								}
							}
						}
					}
					else if (entity.HasComponent<StaticMeshComponent>())
					{
						if (auto mesh = AssetManager::GetAsset<ObjectType::StaticMesh>(entity.GetComponent<StaticMeshComponent>().StaticMesh); mesh)
						{
							if (auto meshSource = AssetManager::GetAsset<AssetType::MeshSource>(mesh->GetMeshSource()); meshSource)
							{
								if (m_ShowBoundingBoxSubmeshes)
								{
									const auto& submeshIndices = mesh->GetSubmeshes();
									const auto& submeshes = meshSource->GetSubmeshes();
									for (uint32_t submeshIndex : submeshIndices)
									{
										Mat4 transform = m_Editor->m_CurrentScene->GetWorldSpaceTransformMatrix(entity);
										const Utils::AABB& aabb = submeshes[submeshIndex].BoundingBox;
										m_ViewportRenderer2D->DrawAABB(aabb, transform * submeshes[submeshIndex].Transform, Vec4{ 1.0f, 1.0f, 1.0f, 1.0f });
									}
								}
								else
								{
									Mat4 transform = m_Editor->m_CurrentScene->GetWorldSpaceTransformMatrix(entity);
									const Utils::AABB& aabb = meshSource->GetBoundingBox();
									m_ViewportRenderer2D->DrawAABB(aabb, transform, Vec4{ 1.0f, 1.0f, 1.0f, 1.0f });
								}
							}
						}
					}
				}
			}
			else
			{
                for (auto dynamicMeshEntities = m_Editor-> m_CurrentScene->GetAllEntitiesWith<SubmeshComponent>(); auto e : dynamicMeshEntities)
				{
					Entity entity = { e, m_Editor->m_CurrentScene.Raw() };
					Mat4 transform = m_Editor->m_CurrentScene->GetWorldSpaceTransformMatrix(entity);
					if(auto mesh = AssetManager::GetAsset<Mesh>(entity.GetComponent<SubmeshComponent>().Mesh); mesh)
					{
						if (auto meshSource = AssetManager::GetAsset<AssetType::MeshSource>(mesh->GetMeshSource()); meshSource)
						{
							const Utils::AABB& aabb = meshSource->GetBoundingBox();
							m_ViewportRenderer2D->DrawAABB(aabb, transform, Vec4{ 1.0f, 1.0f, 1.0f, 1.0f });
						}
					}
				}

                for (auto staticMeshEntities = m_Editor->m_CurrentScene->GetAllEntitiesWith<StaticMeshComponent>(); auto e : staticMeshEntities)
				{
					Entity entity = { e, m_Editor->m_CurrentScene.Raw() };
					Mat4 transform = m_Editor->m_CurrentScene->GetWorldSpaceTransformMatrix(entity);
					if(auto mesh = AssetManager::GetAsset<ObjectType::StaticMesh>(entity.GetComponent<StaticMeshComponent>().StaticMesh); mesh)
					{
						if (auto meshSource = AssetManager::GetAsset<AssetType::MeshSource>(mesh->GetMeshSource()); meshSource)
						{
							const Utils::AABB& aabb = meshSource->GetBoundingBox();
							m_ViewportRenderer2D->DrawAABB(aabb, transform, glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f });
						}
					}
				}
			}
		}
		*/

		/*
		if (m_ShowIcons)
		{
			{
                for (auto entities = m_Editor->m_CurrentScene->GetAllEntitiesWith<PointLightComponent>(); auto e : entities)
				{
					Entity entity = { e, m_Editor->m_CurrentScene.Raw() };
					m_ViewportRenderer2D->DrawQuadBillboard(m_Editor->m_CurrentScene->GetWorldSpaceTransform(entity).Translation, { 1.0f, 1.0f }, EditorResources::PointLightIcon);
				}
			}

			{
                for (auto entities = m_Editor->m_CurrentScene->GetAllEntitiesWith<SpotLightComponent>(); auto e : entities)
				{
					Entity entity = { e, m_Editor->m_CurrentScene.Raw() };
					m_ViewportRenderer2D->DrawQuadBillboard(m_Editor->m_CurrentScene->GetWorldSpaceTransform(entity).Translation, { 1.0f, 1.0f }, EditorResources::SpotLightIcon);
				}
			}

			{
                for (auto entities = m_Editor->m_CurrentScene->GetAllEntitiesWith<CameraComponent>(); auto e : entities)
				{
					Entity entity = { e, m_Editor->m_CurrentScene.Raw() };
					m_ViewportRenderer2D->DrawQuadBillboard(m_Editor->m_CurrentScene->GetWorldSpaceTransform(entity).Translation, { 1.0f, 1.0f }, EditorResources::CameraIcon);
				}
			}

			{
                for (auto entities = m_Editor->m_CurrentScene->GetAllEntitiesWith<AudioComponent>(); auto e : entities)
				{
					Entity entity = { e, m_Editor->m_CurrentScene.Raw() };
					m_ViewportRenderer2D->DrawQuadBillboard(m_Editor->m_CurrentScene->GetWorldSpaceTransform(entity).Translation, { 1.0f, 1.0f }, EditorResources::AudioIcon);
				}
			}
		}

	    for (const auto& selectedEntities = SelectionManager::GetSelections(m_Editor->m_CurrentScene->GetUUID()); const auto& entityID : selectedEntities)
		{
			Entity entity = m_Editor->m_CurrentScene->GetEntityWithUUID(entityID);

			if (!entity.HasComponent<PointLightComponent>())

			ImVec2 minBound = ImGui::GetWindowPos();
			ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.y };

			auto viewportOffset = ImGui::GetCursorPos(); /// includes tab bar
			minBound.x += viewportOffset.x;
			minBound.y += viewportOffset.y;

            if (ImVec2 viewportSize = { maxBound.x - minBound.x, maxBound.y - minBound.y }; viewportSize.x > 1 && viewportSize.y > 1)
			{
				m_ViewportRenderer->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
				m_ViewportCamera.SetViewportBounds((uint32_t)minBound.x, (uint32_t)minBound.y, (uint32_t)maxBound.x, (uint32_t)maxBound.y);

				m_Editor->m_EditorScene->SetViewportBounds((uint32_t)minBound.x, (uint32_t)minBound.y, (uint32_t)maxBound.x, (uint32_t)maxBound.y);
				if (m_Editor->m_RuntimeScene && m_IsMainViewport)
				{
					m_Editor->m_RuntimeScene->SetViewportBounds((uint32_t)minBound.x, (uint32_t)minBound.y, (uint32_t)maxBound.x, (uint32_t)maxBound.y);
				}

				/// Render viewport image
				Ref<Image2D> finalImage = m_ViewportRenderer->GetFinalPassImage();
				if (finalImage != nullptr)
				{
					UI::Image(m_ViewportRenderer->GetFinalPassImage(), viewportSize, { 0, 1 }, { 1, 0 });
					UI_HandleAssetDrop();
				}

				m_ViewportBounds[0] = { minBound.x, minBound.y };
				m_ViewportBounds[1] = { maxBound.x, maxBound.y };

				UI_GizmosToolbar();
				UI_CentralToolbar();
				UI_ViewportSettings();

				if (m_IsFocused && m_ShowGizmos && (m_ShowGizmosInPlayMode || m_Editor->m_CurrentScene != m_Editor->m_RuntimeScene || !m_IsMainViewport))
					UI_DrawGizmos();
			}
		}
		*/

	    ImGui::End();
		ImGui::PopStyleVar(2);
        /// Draw debug visuals for entities with mesh components
        //RenderEntityDebugVisuals();

        m_ViewportRenderer2D->EndScene();
	}

	void Viewport::OnEvent(Event &e)
	{
        if (m_IsMouseOver)
            m_ViewportCamera.OnEvent(e);

        if (m_IsFocused)
        {
            EventDispatcher dispatcher(e);
            dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent &event) { return OnKeyPressedEvent(event); });
            dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent &event) { return OnMouseButtonPressed(event); });
        }
	}

	void Viewport::ResetCamera()
	{
        m_ViewportCamera = EditorCamera(45.0f, 1280.0f, 720.0f, 0.1f, 1000.0f);
	}

	bool * Viewport::GetIsVisibleMemory()
	{
        return &m_IsVisible;
	}

	void Viewport::UI_DrawGizmos()
	{
	    SEDX_PROFILE_FUNC();

		if (m_SelectionMode != SelectionMode::Entity || m_Editor->m_GizmoType == -1)
			return;

		const auto& selections = SelectionManager::GetSelections(m_Editor->m_CurrentScene->GetUUID());

		if (selections.empty())
			return;

		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

		bool snap = Input::IsKeyDown(SEDX_KEY_LEFT_CONTROL);

		float snapValue = m_Editor->GetSnapValue();
		float snapValues[3] = { snapValue, snapValue, snapValue };

		Mat4 projectionMatrix, viewMatrix;
		if (m_IsMainViewport && m_Editor->m_SceneState == Editor::SceneState::Play && !m_Editor->m_EditorCameraInRuntime)
		{
			Entity cameraEntity = m_Editor->m_CurrentScene->GetMainCameraEntity();
			SceneCamera& camera = cameraEntity.GetComponent<CameraComponent>();
			projectionMatrix = camera.GetProjectionMatrix();
			viewMatrix = glm::inverse(m_Editor->m_CurrentScene->GetWorldSpaceTransformMatrix(cameraEntity));
		}
		else
		{
			projectionMatrix = m_ViewportCamera.GetProjectionMatrix();
			viewMatrix = m_ViewportCamera.GetViewMatrix();
		}

		if (selections.size() == 1)
		{
			Entity entity = m_Editor->m_CurrentScene->GetEntityWithUUID(selections[0]);
			TransformComponent& entityTransform = entity.Transform();

			Mat4 transform = m_Editor->m_CurrentScene->GetWorldSpaceTransformMatrix(entity);

			SEDX_EDITOR_OP_DECLARE;
			if (ImGuizmo::Manipulate(
				glm::value_ptr(viewMatrix),
				glm::value_ptr(projectionMatrix),
				static_cast<ImGuizmo::OPERATION>(m_Editor->m_GizmoType),
				m_Editor->m_GizmoWorldOrientation ? ImGuizmo::WORLD_SPACE : ImGuizmo::LOCAL_SPACE,
				glm::value_ptr(transform),
				nullptr,
				snap ? snapValues : nullptr)
			)
			{
				auto hold = entityTransform;

                if (Entity parent = m_Editor->m_CurrentScene->TryGetEntityWithUUID(entity.GetParentUUID()))
				{
					glm::mat4 parentTransform = m_Editor->m_CurrentScene->GetWorldSpaceTransformMatrix(parent);
					transform = glm::inverse(parentTransform) * transform;
				}

				/// Manipulated transform is now in local space of parent (= world space if no parent)
				/// We can decompose into translation, rotation, and scale and compare with original
				/// to figure out how to best update entity transform
				///
				/// Why do we do this instead of just setting the entire entity transform?
				/// Because it's more robust to set only those components of transform
				/// that we are meant to be changing (dictated by m_GizmoType).  That way we avoid
				/// small drift (particularly in rotation and scale) due numerical precision issues
				/// from all those matrix operations.
				Vec3 translation;
				glm::quat rotation;
				Vec3 scale;
                Utils::Math::DecomposeTransform(transform, translation, rotation, scale);

				switch (m_Editor->m_GizmoType)
				{
					case ImGuizmo::TRANSLATE:
					{
						entityTransform.Translation = translation;
						break;
					}
					case ImGuizmo::ROTATE:
					{
						/// Do this in Euler in an attempt to preserve any full revolutions (> 360)
						Vec3 originalRotationEuler = entityTransform.GetRotationEuler();

						/// Map original rotation to range [-180, 180] which is what ImGuizmo gives us
						originalRotationEuler.x = fmodf(originalRotationEuler.x + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();
						originalRotationEuler.y = fmodf(originalRotationEuler.y + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();
						originalRotationEuler.z = fmodf(originalRotationEuler.z + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();

						Vec3 deltaRotationEuler = glm::eulerAngles(rotation) - originalRotationEuler;

						/// Try to avoid drift due numeric precision
						if (fabs(deltaRotationEuler.x) < 0.001) deltaRotationEuler.x = 0.0f;
						if (fabs(deltaRotationEuler.y) < 0.001) deltaRotationEuler.y = 0.0f;
						if (fabs(deltaRotationEuler.z) < 0.001) deltaRotationEuler.z = 0.0f;

						entityTransform.SetRotationEuler(entityTransform.GetRotationEuler() += deltaRotationEuler);
						break;
					}
					case ImGuizmo::SCALE:
					{
						entityTransform.Scale = scale;
						break;
					}
				}

				SEDX_EDITOR_AGGLOMERATE_OP(PushComponent<decltype(hold)>, entity, hold);
			}
			else SEDX_EDITOR_COMPLETE_OP;
		}
		else
		{
			if (m_MultiTransformTarget == TransformationTarget::MedianPoint && m_Editor->m_GizmoType == ImGuizmo::SCALE)
			{
				/// TODO: Disabling multi-entity scaling for median point mode for now since it causes strange scaling behavior
				return;
			}

			Vec3 medianLocation = Vec3(0.0f);
			Vec3 medianScale = Vec3(1.0f);
			Vec3 medianRotation = Vec3(0.0f);
			for (auto entityID : selections)
			{
				Entity entity = m_Editor->m_CurrentScene->GetEntityWithUUID(entityID);
				medianLocation += entity.Transform().Translation;
				medianScale += entity.Transform().Scale;
				medianRotation += entity.Transform().GetRotationEuler();
			}
			medianLocation /= selections.size();
			medianScale /= selections.size();
			medianRotation /= selections.size();

			Mat4 medianPointMatrix = glm::translate(Mat4(1.0f), medianLocation) * glm::toMat4(glm::quat(medianRotation)) * glm::scale(Mat4(1.0f), medianScale);
			Mat4 deltaMatrix = Mat4(1.0f);

			/// TODO: Undo stack multi-transform.
			if (ImGuizmo::Manipulate(
				glm::value_ptr(viewMatrix),
				glm::value_ptr(projectionMatrix),
				static_cast<ImGuizmo::OPERATION>(m_Editor->m_GizmoType),
				m_Editor->m_GizmoWorldOrientation ? ImGuizmo::WORLD_SPACE : ImGuizmo::LOCAL_SPACE,
				glm::value_ptr(medianPointMatrix),
				glm::value_ptr(deltaMatrix),
				snap ? snapValues : nullptr)
			)
			{
				switch (m_MultiTransformTarget)
				{
					case TransformationTarget::MedianPoint:
					{
						for (auto entityID : selections)
						{
							Entity entity = m_Editor->m_CurrentScene->GetEntityWithUUID(entityID);
							TransformComponent& transform = entity.Transform();
							transform.SetTransform(deltaMatrix * transform.GetTransform());
						}

						break;
					}
					case TransformationTarget::IndividualOrigins:
					{
						Vec3 deltaTranslation, deltaScale;
						glm::quat deltaRotation;
                        Utils::Math::DecomposeTransform(deltaMatrix, deltaTranslation, deltaRotation, deltaScale);

						for (auto entityID : selections)
						{
							Entity entity = m_Editor->m_CurrentScene->GetEntityWithUUID(entityID);
							TransformComponent& transform = entity.Transform();

							switch (m_Editor->m_GizmoType)
							{
								case ImGuizmo::TRANSLATE:
								{
									transform.Translation += deltaTranslation;
									break;
								}
								case ImGuizmo::ROTATE:
								{
									transform.SetRotationEuler(transform.GetRotationEuler() + glm::eulerAngles(deltaRotation));
									break;
								}
								case ImGuizmo::SCALE:
								{
									if (deltaScale != glm::vec3(1.0f, 1.0f, 1.0f))
										transform.Scale *= deltaScale;
									break;
								}
							}
						}
						break;
					}
				}
			}
		}
	}

	void Viewport::UI_GizmosToolbar()
	{
	    if ((!m_ShowGizmosInPlayMode && m_IsMainViewport) && m_Editor->m_CurrentScene == m_Editor->m_RuntimeScene)
			return;

		UI::ScopedStyle disableSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		UI::ScopedStyle disableWindowBorder(ImGuiStyleVar_WindowBorderSize, 0.0f);
		UI::ScopedStyle windowRounding(ImGuiStyleVar_WindowRounding, 4.0f);
		UI::ScopedStyle disablePadding(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		const float cursorYOffset = ImGui::GetCursorStartPos().y + 10.f;
		constexpr float desiredHeight = 26.0f;
		constexpr float buttonSize = 18.0f;
		constexpr float edgeOffset = 4.0f;
		constexpr float numberOfButtons = 4.0f;
		constexpr float gizmoPanelBackgroundWidth = edgeOffset * 6.0f + buttonSize * numberOfButtons + edgeOffset * (numberOfButtons - 1.0f) * 2.0f;

		ImGui::SetCursorPos(ImVec2(15.f, cursorYOffset));
		{
			ImVec2 p_min = ImGui::GetCursorScreenPos();
			ImVec2 size  = ImVec2(gizmoPanelBackgroundWidth, desiredHeight);
			ImVec2 p_max = ImVec2(p_min.x + size.x, p_min.y + size.y);

			ImGui::GetWindowDrawList()->AddRectFilled(p_min,p_max, IM_COL32(15, 15, 15, 127), 4.0f);

			char verticalName[256];
			snprintf(verticalName, sizeof(verticalName), "%s_%s", "##gizmosV", m_ViewportName.c_str());

			ImGui::BeginVertical(verticalName, { gizmoPanelBackgroundWidth, desiredHeight });
			ImGui::Spring();

			char horizontalName[256];
			snprintf(horizontalName, sizeof(horizontalName), "%s_%s", "##gizmosH", m_ViewportName.c_str());

			ImGui::BeginHorizontal(horizontalName, { gizmoPanelBackgroundWidth, desiredHeight });
			ImGui::Spring();
			{
				UI::ScopedStyle enableSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(edgeOffset * 2.0f, 0));

                constexpr ImColor c_SelectedGizmoButtonColor = Colors::Theme::accent;
                constexpr ImColor c_UnselectedGizmoButtonColor = Colors::Theme::textBrighter;

				auto gizmoButton = [&c_SelectedGizmoButtonColor, buttonSize](const Ref<Texture2D>& icon, const ImColor& tint, float paddingY = 0.0f)
				{
					const float height = std::min((float)icon->GetHeight(), buttonSize) - paddingY * 2.0f;
					const float width = (float)icon->GetWidth() / (float)icon->GetHeight() * height;
					const bool clicked = ImGui::InvisibleButton(UI::GenerateID(), ImVec2(width, height));
					UI::DrawButtonImage(icon, tint, tint, tint, UI::RectOffset(UI::GetItemRect(), 0.0f, paddingY));

					return clicked;
				};

				ImColor buttonTint = m_Editor->m_GizmoType == -1 ? c_SelectedGizmoButtonColor : c_UnselectedGizmoButtonColor;
				if (gizmoButton(EditorResources::PointerIcon, buttonTint))
					m_Editor->m_GizmoType = -1;

				UI::SetTooltip("Select");

				buttonTint = m_Editor->m_GizmoType == ImGuizmo::OPERATION::TRANSLATE ? c_SelectedGizmoButtonColor : c_UnselectedGizmoButtonColor;
				if (gizmoButton(EditorResources::MoveIcon, buttonTint))
					m_Editor->m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;

				UI::SetTooltip("Translate");

				buttonTint = m_Editor->m_GizmoType == ImGuizmo::OPERATION::ROTATE ? c_SelectedGizmoButtonColor : c_UnselectedGizmoButtonColor;
				if (gizmoButton(EditorResources::RotateIcon, buttonTint))
					m_Editor->m_GizmoType = ImGuizmo::OPERATION::ROTATE;

				UI::SetTooltip("Rotate");

				buttonTint = m_Editor->m_GizmoType == ImGuizmo::OPERATION::SCALE ? c_SelectedGizmoButtonColor : c_UnselectedGizmoButtonColor;
				if (gizmoButton(EditorResources::ScaleIcon, buttonTint))
					m_Editor->m_GizmoType = ImGuizmo::OPERATION::SCALE;

				UI::SetTooltip("Scale");

			}
			ImGui::Spring();
			ImGui::EndHorizontal();
			ImGui::Spring();
			ImGui::EndVertical();
		}

		/// Gizmo orientation window
		constexpr float offsetFromLeft = 10.f;
		constexpr float worldLocalBackgroundWidth = edgeOffset * 6.0f + buttonSize + edgeOffset * 2.0f;

		ImGui::SetCursorPos(ImVec2(15.f + gizmoPanelBackgroundWidth + offsetFromLeft, cursorYOffset));

		char worldLocalName[256];
		snprintf(worldLocalName, sizeof(worldLocalName), "%s_%s", "##world_local", m_ViewportName.c_str());
		{

			ImVec2 p_min = ImGui::GetCursorScreenPos();
			ImVec2 size  = ImVec2(worldLocalBackgroundWidth, desiredHeight);
			ImVec2 p_max = ImVec2(p_min.x + size.x, p_min.y + size.y);

			ImGui::GetWindowDrawList()->AddRectFilled(p_min,p_max, IM_COL32(15, 15, 15, 127), 4.0f);

			/// TODO: maybe reuse one buffer??
			char worldLocalHorizontalName[256];
			snprintf(worldLocalHorizontalName, sizeof(worldLocalHorizontalName), "%s_%s", "##world_localH", m_ViewportName.c_str());

			ImGui::BeginHorizontal(worldLocalHorizontalName, { worldLocalBackgroundWidth, desiredHeight });
			ImGui::Spring();
			{
				auto worldLocalButton = [buttonSize](const Ref<Texture2D>& worldIcon, const Ref<Texture2D>& localIcon, bool isWorld, const ImColor& tint, float paddingY = 0.0f)
				{
					const float iconHeight = isWorld ? static_cast<float>(worldIcon->GetHeight()) : static_cast<float>(localIcon->GetHeight());
					const float iconWidth = isWorld ? static_cast<float>(worldIcon->GetWidth()) : static_cast<float>(localIcon->GetWidth());

					const float height = std::min(iconHeight, buttonSize) - paddingY * 2.0f;
					const float width = iconWidth / iconHeight * height;
					const bool clicked = ImGui::InvisibleButton(UI::GenerateID(), ImVec2(width, height));
					UI::DrawButtonImage(isWorld ? worldIcon : localIcon, tint, tint, tint, UI::RectOffset(UI::GetItemRect(), 0.0f, paddingY));

					return clicked;
				};

				if (worldLocalButton(EditorResources::GizmoWorldOrientationIcon, EditorResources::GizmoLocalOrientationIcon, m_Editor->m_GizmoWorldOrientation, Colors::Theme::textBrighter))
					m_Editor->m_GizmoWorldOrientation = !m_Editor->m_GizmoWorldOrientation;

				UI::SetTooltip("Toggles the transform gizmo coordinate systems between world and local (object) space");
			}
			ImGui::Spring();
			ImGui::EndHorizontal();
		}
	}

	void Viewport::UI_CentralToolbar()
	{
	    if (!m_IsMainViewport)
			return;

		UI::ScopedStyle disableSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		UI::ScopedStyle disableWindowBorder(ImGuiStyleVar_WindowBorderSize, 0.0f);
		UI::ScopedStyle windowRounding(ImGuiStyleVar_WindowRounding, 4.0f);
		UI::ScopedStyle disablePadding(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		const float cursorYOffset = ImGui::GetCursorStartPos().y + 10.f;
		constexpr float buttonSize = 18.0f + 5.0f;
		constexpr float edgeOffset = 4.0f;
		constexpr float numberOfButtons = 3.0f;
		constexpr float desiredHeight = 26.0f + 5.0f;
		constexpr float backgroundWidth = edgeOffset * 6.0f + buttonSize * numberOfButtons + edgeOffset * (numberOfButtons - 1.0f) * 2.0f;

		ImGui::SetCursorPos(ImVec2(ImGui::GetWindowContentRegionWidth() / 2.f - backgroundWidth / 2.f, cursorYOffset));

		ImVec2 p_min = ImGui::GetCursorScreenPos();
		ImVec2 size  = ImVec2(backgroundWidth, desiredHeight);
		ImVec2 p_max = ImVec2(p_min.x + size.x, p_min.y + size.y);

		ImGui::GetWindowDrawList()->AddRectFilled(p_min,p_max, IM_COL32(15, 15, 15, 127), 4.0f);

		char verticalName[256];
		snprintf(verticalName, sizeof(verticalName), "%s_%s", "##viewport_central_toolbarV", m_ViewportName.c_str());

		ImGui::BeginVertical(verticalName, { backgroundWidth, desiredHeight });
		ImGui::Spring();

		char horizontalName[256];
		snprintf(horizontalName, sizeof(horizontalName), "%s_%s", "##viewport_central_toolbarH", m_ViewportName.c_str());

		ImGui::BeginHorizontal(horizontalName, { backgroundWidth, desiredHeight});
		ImGui::Spring();
		{
			UI::ScopedStyle enableSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(edgeOffset * 2.0f, 0));

			const ImColor c_ButtonTint = Colors::Theme::text;
			const ImColor c_SimulateButtonTint = m_Editor->m_SceneState == Editor::SceneState::Simulate ? ImColor(1.0f, 0.75f, 0.75f, 1.0f) : c_ButtonTint;

			auto toolbarButton = [buttonSize](const Ref<Texture2D>& icon, const ImColor& tint, float paddingY = 0.0f)
			{
				const float height = std::min((float)icon->GetHeight(), buttonSize) - paddingY * 2.0f;
				const float width = (float)icon->GetWidth() / (float)icon->GetHeight() * height;
				const bool clicked = ImGui::InvisibleButton(UI::GenerateID(), ImVec2(width, height));
				UI::DrawButtonImage(icon, tint, tint, tint, UI::RectOffset(UI::GetItemRect(), 0.0f, paddingY));

				return clicked;
			};

			Ref<Texture2D> buttonTex = m_Editor->m_SceneState == Editor::SceneState::Play ? EditorResources::StopIcon : EditorResources::PlayIcon;
			if (toolbarButton(buttonTex, c_ButtonTint))
			{
				m_Editor->m_TitleBarPreviousColor = m_Editor->m_TitleBarActiveColor;
				if (m_Editor->m_SceneState == Editor::SceneState::Edit)
				{
					m_Editor->m_TitleBarTargetColor = Colors::Theme::titlebarOrange;
					m_Editor->OnScenePlay();
				}
				else if (m_Editor->m_SceneState != Editor::SceneState::Simulate)
				{
					m_Editor->m_TitleBarTargetColor = Colors::Theme::titlebarGreen;
					m_Editor->OnSceneStop();
				}

				m_Editor->m_AnimateTitleBarColor = true;
			}
			UI::SetTooltip(m_Editor->m_SceneState == Editor::SceneState::Edit ? "Play" : "Stop");

			if (toolbarButton(EditorResources::SimulateIcon, c_SimulateButtonTint))
			{
				if (m_Editor->m_SceneState == Editor::SceneState::Edit)
					m_Editor->OnSceneStartSimulation();
				else if (m_Editor->m_SceneState == Editor::SceneState::Simulate)
					m_Editor->OnSceneStopSimulation();
			}
			UI::SetTooltip(m_Editor->m_SceneState == Editor::SceneState::Simulate ? "Stop" : "Simulate Physics");

			if (toolbarButton(EditorResources::PauseIcon, c_ButtonTint))
			{
				if (m_Editor->m_SceneState == Editor::SceneState::Play)
					m_Editor->m_SceneState = Editor::SceneState::Pause;
				else if (m_Editor->m_SceneState == Editor::SceneState::Pause)
					m_Editor->m_SceneState = Editor::SceneState::Play;
			}
			UI::SetTooltip(m_Editor->m_SceneState == Editor::SceneState::Pause ? "Resume" : "Pause");
		}
		ImGui::Spring();
		ImGui::EndHorizontal();
		ImGui::Spring();
		ImGui::EndVertical();
	}

	void Viewport::UI_ViewportSettings()
	{
	    UI::ScopedStyle disableSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		UI::ScopedStyle disableWindowBorder(ImGuiStyleVar_WindowBorderSize, 0.0f);
		UI::ScopedStyle windowRounding(ImGuiStyleVar_WindowRounding, 4.0f);
		UI::ScopedStyle disablePadding(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		const float cursorYOffset = ImGui::GetCursorStartPos().y + 10.f;
		constexpr float buttonSize = 18.0f;
		constexpr float edgeOffset = 2.0f;
		constexpr float windowHeight = 32.0f;
		constexpr float numberOfButtons = 1.0f;
		constexpr float backgroundWidth = edgeOffset * 6.0f + buttonSize * numberOfButtons + edgeOffset * (numberOfButtons - 1.0f) * 2.0f;
		constexpr float desiredHeight = 26.0f;

		ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionAvail().x - 15.f - backgroundWidth, cursorYOffset));

		ImVec2 p_min = ImGui::GetCursorScreenPos();
		ImVec2 size  = ImVec2(backgroundWidth, desiredHeight);
		ImVec2 p_max = ImVec2(p_min.x + size.x, p_min.y + size.y);

		ImGui::GetWindowDrawList()->AddRectFilled(p_min,p_max, IM_COL32(15, 15, 15, 127), 4.0f);

		bool openSettingsPopup = false;

		char verticalName[256];
		snprintf(verticalName, sizeof(verticalName), "%s_%s", "##viewportSettingsV", m_ViewportName.c_str());

		char horizontalName[256];
		snprintf(horizontalName, sizeof(horizontalName), "%s_%s", "##viewportSettingsH", m_ViewportName.c_str());

		ImGui::BeginVertical(verticalName, { backgroundWidth, desiredHeight });
		ImGui::Spring();
		ImGui::BeginHorizontal(horizontalName, { backgroundWidth, desiredHeight });
		ImGui::Spring();
		{
			UI::ScopedStyle enableSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(edgeOffset * 2.0f, 0));
			const ImColor c_UnselectedGizmoButtonColor = Colors::Theme::textBrighter;

			auto imageButton = [buttonSize](const Ref<Texture2D>& icon, const ImColor& tint, float paddingY = 0.0f)
			{
				const float height = std::min((float)icon->GetHeight(), buttonSize) - paddingY * 2.0f;
				const float width = (float)icon->GetWidth() / (float)icon->GetHeight() * height;
				const bool clicked = ImGui::InvisibleButton(UI::GenerateID(), ImVec2(width, height));
				UI::DrawButtonImage(icon, tint, tint, tint, UI::RectOffset(UI::GetItemRect(), 0.0f, paddingY));

				return clicked;
			};

			if (imageButton(EditorResources::GearIcon, c_UnselectedGizmoButtonColor))
				openSettingsPopup = true;
			UI::SetTooltip("Viewport Settings");
		}
		ImGui::Spring();
		ImGui::EndHorizontal();
		ImGui::Spring();
		ImGui::EndVertical();

		/// Draw the settings popup
		{
			int32_t sectionIdx = 0;

			static float popupWidth = 310.0f;

			auto beginSection = [&sectionIdx](const char* name)
			{
				if (sectionIdx > 0)
					UI::ShiftCursorY(5.5f);

				UI::Fonts::PushFont("Bold");
				ImGui::TextUnformatted(name);
				UI::Fonts::PopFont();
				UI::Draw::Underline(Colors::Theme::backgroundDark);
				UI::ShiftCursorY(3.5f);

				bool result = ImGui::BeginTable("##section_table", 2, ImGuiTableFlags_SizingStretchSame);
				if (result)
				{
					ImGui::TableSetupColumn("Labels", ImGuiTableColumnFlags_WidthFixed, popupWidth * 0.5f);
					ImGui::TableSetupColumn("Widgets", ImGuiTableColumnFlags_WidthFixed, popupWidth * 0.5f);
				}

				sectionIdx++;
				return result;
			};

			auto endSection = []()
			{
				ImGui::EndTable();
			};

			auto slider = [](const char* label, float& value, float min = 0.0f, float max = 0.0f)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(label);
				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-1);
				UI::ShiftCursor(GImGui->Style.FramePadding.x, -GImGui->Style.FramePadding.y);
				return UI::SliderFloat(UI::GenerateID(), &value, min, max);
			};

			auto drag = [](const char* label, float& value, float delta = 1.0f, float min = 0.0f, float max = 0.0f)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(label);
				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-1);
				UI::ShiftCursor(GImGui->Style.FramePadding.x, -GImGui->Style.FramePadding.y);
				return UI::DragFloat(UI::GenerateID(), &value, delta, min, max);
			};

			auto checkbox = [](const char* label, bool& value)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(label);
				ImGui::TableSetColumnIndex(1);
				auto table = ImGui::GetCurrentTable();
				float columnWidth = ImGui::TableGetMaxColumnWidth(table, 1);
				UI::ShiftCursor(columnWidth - ImGui::GetFrameHeight() - ImGui::GetStyle().ItemInnerSpacing.x, -GImGui->Style.FramePadding.y);
				return UI::Checkbox(UI::GenerateID(), &value);
			};

			auto dropdown = [](const char* label, const char** options, int32_t optionCount, int32_t* selected)
			{
				const char* current = options[*selected];
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(label);
				ImGui::TableSetColumnIndex(1);
				ImGui::PushItemWidth(-1);

				bool result = false;
				UI::ShiftCursor(GImGui->Style.FramePadding.x, -GImGui->Style.FramePadding.y);
				if (UI::BeginCombo(UI::GenerateID(), current))
				{
					for (int i = 0; i < optionCount; i++)
					{
						const bool is_selected = (current == options[i]);
						if (ImGui::Selectable(options[i], is_selected))
						{
							current = options[i];
							*selected = i;
							result = true;
						}

						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					UI::EndCombo();
				}
				ImGui::PopItemWidth();

				return result;
			};

			UI::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(0, 5.5f));
			UI::ScopedStyle windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
			UI::ScopedStyle windowRounding(ImGuiStyleVar_PopupRounding, 4.0f);
			UI::ScopedStyle cellPadding(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 5.5f));

			if (openSettingsPopup)
				ImGui::OpenPopup("ViewportSettingsPanel");

			ImGui::SetNextWindowPos({ (m_ViewportBounds[1].x - popupWidth) - 34, m_ViewportBounds[0].y + edgeOffset + windowHeight });
			if (ImGui::BeginPopup("ViewportSettingsPanel", ImGuiWindowFlags_NoMove))
			{
				auto& viewportRenderOptions = m_ViewportRenderer->GetOptions();

				if (beginSection("General"))
				{
					static const char* selectionModes[] = { "Entity", "Submesh" };
					dropdown("Selection Mode", selectionModes, 2, (int32_t*)&m_SelectionMode);

					static const char* s_TransformTargetNames[] = { "Median Point", "Individual Origins" };
					dropdown("Multi-Transform Target", s_TransformTargetNames, 2, (int32_t*)&m_MultiTransformTarget);

					endSection();
				}

				if (beginSection("Display"))
				{
					checkbox("Show Icons", m_ShowIcons);
					checkbox("Show Gizmos", m_ShowGizmos);
					checkbox("Show Gizmos In Play Mode", m_ShowGizmosInPlayMode);
					checkbox("Show Bounding Boxes", m_ShowBoundingBoxes);
					if (m_ShowBoundingBoxes)
					{
						checkbox("Selected Entity", m_ShowBoundingBoxSelectedMeshOnly);

						if (m_ShowBoundingBoxSelectedMeshOnly)
							checkbox("Submeshes", m_ShowBoundingBoxSubmeshes);
					}

					checkbox("Show Grid", viewportRenderOptions.ShowGrid);
					checkbox("Show Selected Wireframe", viewportRenderOptions.ShowSelectedInWireframe);

					checkbox("Show Animation Debug", viewportRenderOptions.ShowAnimationDebug);

					static const char* physicsColliderViewOptions[] = { "Selected Entity", "All" };
					checkbox("Show Physics Colliders", viewportRenderOptions.ShowPhysicsColliders);
					dropdown("Physics Collider Mode", physicsColliderViewOptions, 2, (int32_t*)&viewportRenderOptions.PhysicsColliderMode);
					checkbox("Show Colliders On Top", viewportRenderOptions.ShowPhysicsCollidersOnTop);

					if (drag("Line Width", m_LineWidth, 0.5f, 1.0f, 10.0f))
					{
						m_ViewportRenderer2D->SetLineWidth(m_LineWidth);
						m_ViewportRenderer->SetLineWidth(m_LineWidth);
					}

					endSection();
				}

				if (beginSection("Scene Camera"))
				{
					slider("Exposure", m_ViewportCamera.GetExposure(), 0.0f, 5.0f);
					drag("Speed", m_ViewportCamera.m_NormalSpeed, 0.001f, 0.0002f, 0.5f);

					endSection();
				}

				ImGui::EndPopup();
			}
		}
	}

	void Viewport::UI_HandleAssetDrop()
	{
	    if (!ImGui::BeginDragDropTarget() || m_Editor->m_SceneState != Editor::SceneState::Edit)
			return;

	    if (auto data = ImGui::AcceptDragDropPayload("asset_payload"))
		{
			uint64_t count = data->DataSize / sizeof(AssetHandle);

			for (uint64_t i = 0; i < count; i++)
			{
				AssetHandle assetHandle = *(((AssetHandle*)data->Data) + i);
				const AssetMetadata& assetData = Project::GetEditorAssetManager()->GetMetadata(assetHandle);

				/// We can't really support dragging and dropping scenes when we're dropping multiple assets
				if (count == 1 && assetData.Type == AssetType::Scene)
				{
					m_Editor->OpenScene(Project::GetEditorAssetManager()->GetFileSystemPath(assetData));
					break;
				}

                if (Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle))
				{
					if (asset->GetAssetType() == AssetType::MeshSource)
                        m_Editor->OnCreateMeshFromMeshSource({}, asset.As<MeshSource>());

                    else if (asset->GetAssetType() == AssetType::Mesh)
					{
						auto mesh = asset.As<Mesh>();
						auto rootEntity = m_Editor->m_EditorScene->InstantiateMesh(mesh);
						SelectionManager::DeselectAll();
						SelectionManager::Select(m_Editor->m_EditorScene->GetUUID(), rootEntity.GetUUID());
					}
					else if (asset->GetAssetType() == AssetType::StaticMesh)
					{
						auto staticMesh = asset.As<StaticMesh>();
						auto rootEntity = m_Editor->m_EditorScene->InstantiateStaticMesh(staticMesh);
						SelectionManager::DeselectAll();
						SelectionManager::Select(m_Editor->m_EditorScene->GetUUID(), rootEntity.GetUUID());
					}
					else if (asset->GetAssetType() == AssetType::Prefab)
					{
						Ref<Prefab> prefab = asset.As<Prefab>();
						auto rootEntity = m_Editor->m_EditorScene->Instantiate(prefab);
						SelectionManager::DeselectAll();
						SelectionManager::Select(m_Editor->m_EditorScene->GetUUID(), rootEntity.GetUUID());
					}
				}
				else
				{
					m_Editor->m_InvalidAssetMetadataPopupData.Metadata = assetData;
					m_Editor->UI_ShowInvalidAssetMetadataPopup();
				}
			}
		}

		ImGui::EndDragDropTarget();
	}

	bool Viewport::OnKeyPressedEvent(KeyPressedEvent &e)
	{
		if (m_Editor->m_CurrentScene == m_Editor->m_RuntimeScene && IsMainViewport())
			return false;

		if ((m_IsMouseOver) && !Input::IsMouseButtonDown(MouseButton::Right))
		{
			switch (e.GetKeyCode())
			{
			case KeyCode::Q:
				m_Editor->m_GizmoType = -1;
				break;
			case KeyCode::W:
				m_Editor->m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
				break;
			case KeyCode::E:
				m_Editor->m_GizmoType = ImGuizmo::OPERATION::ROTATE;
				break;
			case KeyCode::R:
				m_Editor->m_GizmoType = ImGuizmo::OPERATION::SCALE;
				break;
			case KeyCode::F:
				{
					if (SelectionManager::GetSelectionCount(m_Editor->m_CurrentScene->GetUUID()) == 0)
						break;

					// TODO: Maybe compute average location to focus on? Or maybe cycle through all the selected entities?
					UUID selectedEntityID = SelectionManager::GetSelections(m_Editor->m_CurrentScene->GetUUID()).front();
					Entity selectedEntity = m_Editor->m_CurrentScene->GetEntityWithUUID(selectedEntityID);
					m_ViewportCamera.Focus(m_Editor->m_CurrentScene->GetWorldSpaceTransform(selectedEntity).Translation);
					break;
				}
			}
		}

		if (Input::IsKeyDown(SEDX_KEY_LEFT_CONTROL) && !Input::IsMouseButtonDown(MouseButton::Right))
		{
			switch (e.GetKeyCode())
			{
			case KeyCode::B:
				/// Toggle bounding boxes
				m_ShowBoundingBoxes = !m_ShowBoundingBoxes;
				break;
			case KeyCode::G:
				/// Toggle grid
				m_ViewportRenderer->GetOptions().ShowGrid = !m_ViewportRenderer->GetOptions().ShowGrid;
				break;
			case KeyCode::P:
				{
					auto& viewportRendererOptions = m_ViewportRenderer->GetOptions();
					viewportRendererOptions.ShowPhysicsColliders = !viewportRendererOptions.ShowPhysicsColliders;
					break;
				}
			}
		}

		return false;
	}

	bool Viewport::OnMouseButtonPressed(MouseButtonPressedEvent &e)
	{
		if (!m_IsMouseOver)
			return false;

		if (e.GetMouseButton() != MouseButton::Left)
			return false;

		if (Input::IsKeyDown(KeyCode::LeftAlt) || Input::IsMouseButtonDown(MouseButton::Right))
			return false;

		if (ImGuizmo::IsOver())
			return false;

		if (ImGui::IsAnyItemHovered())
			return false;

		ImGui::ClearActiveID();

        if (auto [mouseX, mouseY] = GetMouseViewportSpace(m_IsMouseOver); mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
		{
            std::vector<Editor::SelectionData> selectionData;
            auto [origin, direction] = CastRay(mouseX, mouseY);

            for (auto meshEntities = m_Editor->m_CurrentScene->GetAllEntitiesWith<SubmeshComponent>(); auto e : meshEntities)
			{
				Entity entity = { e, m_Editor->m_CurrentScene.Raw() };
				auto& mc = entity.GetComponent<SubmeshComponent>();
				if (auto mesh = AssetManager::GetAsset<Mesh>(mc.Mesh); mesh)
				{
					if (auto meshSource = AssetManager::GetAsset<MeshSource>(mesh->GetMeshSource()); meshSource)
					{
						auto& submeshes = meshSource->GetSubmeshes();
						auto& submesh = submeshes[mc.SubmeshIndex];
						Mat4 transform = m_Editor->m_CurrentScene->GetWorldSpaceTransformMatrix(entity);
						Ray ray =
						{
							glm::inverse(transform) * Vec4(origin, 1.0f),
							glm::inverse(Mat3(transform)) * direction
						};

						float t;
                        if (bool intersects = ray.IntersectsAABB(submesh.BoundingBox, t))
						{
                            for (const auto& triangleCache = meshSource->GetTriangleCache(mc.SubmeshIndex); const auto& triangle : triangleCache)
							{
								if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t))
								{
									selectionData.push_back({ entity, &submesh, t });
									break;
								}
							}
						}
					}
				}
			}

			auto staticMeshEntities = m_Editor->m_CurrentScene->GetAllEntitiesWith<StaticMeshComponent>();
			for (auto e : staticMeshEntities)
			{
				Entity entity = { e, m_Editor->m_CurrentScene.Raw() };
				auto& smc = entity.GetComponent<StaticMeshComponent>();
				if (auto staticMesh = AssetManager::GetAsset<StaticMesh>(smc.StaticMesh); staticMesh)
				{
					if (auto meshSource = AssetManager::GetAsset<MeshSource>(staticMesh->GetMeshSource()); meshSource)
					{
						auto& submeshes = meshSource->GetSubmeshes();
						for (uint32_t i = 0; i < submeshes.size(); i++)
						{
							auto& submesh = submeshes[i];
							Mat4 transform =m_Editor-> m_CurrentScene->GetWorldSpaceTransformMatrix(entity);
							Ray ray = {
								glm::inverse(transform * submesh.Transform) * Vec4(origin, 1.0f),
								glm::inverse(Mat3(transform * submesh.Transform)) * direction
							};

							float t;
                            if (bool intersects = ray.IntersectsAABB(submesh.BoundingBox, t))
							{
                                for (const auto& triangleCache = meshSource->GetTriangleCache(i); const auto& triangle : triangleCache)
								{
									if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t))
									{
										selectionData.push_back({ entity, &submesh, t });
										break;
									}
								}
							}
						}
					}
				}
			}

			std::sort(selectionData.begin(), selectionData.end(), [](auto& a, auto& b) { return a.Distance < b.Distance; });

			bool ctrlDown = Input::IsKeyDown(KeyCode::LeftControl) || Input::IsKeyDown(KeyCode::RightControl);
			bool shiftDown = Input::IsKeyDown(KeyCode::LeftShift) || Input::IsKeyDown(KeyCode::RightShift);
			if (!ctrlDown)
				SelectionManager::DeselectAll();

			if (!selectionData.empty())
			{
				Entity entity = selectionData.front().Entity;
				if (shiftDown)
				{
					while (entity.GetParent())
					{
						entity = entity.GetParent();
					}
				}
				if (SelectionManager::IsSelected(m_Editor->m_CurrentScene->GetUUID(), entity.GetUUID()) && ctrlDown)
					SelectionManager::Deselect(m_Editor->m_CurrentScene->GetUUID(), entity.GetUUID());
				else
					SelectionManager::Select(m_Editor->m_CurrentScene->GetUUID(), entity.GetUUID());
			}
		}

		return false;
	}

	std::pair<float, float> Viewport::GetMouseViewportSpace(bool primaryViewport) const
	{
		auto [mx, my] = ImGui::GetMousePos();
		const auto& viewportBounds = m_ViewportBounds;
		mx -= viewportBounds[0].x;
		my -= viewportBounds[0].y;
		auto viewportWidth = viewportBounds[1].x - viewportBounds[0].x;
		auto viewportHeight = viewportBounds[1].y - viewportBounds[0].y;

		return { (mx / viewportWidth) * 2.0f - 1.0f, ((my / viewportHeight) * 2.0f - 1.0f) * -1.0f };
	}

	std::pair<Vec3, Vec3> Viewport::CastRay(float mx, float my) const
	{
        Vec4 mouseClipPos = {mx, my, -1.0f, 1.0f};

        auto inverseProj = glm::inverse(m_ViewportCamera.GetProjectionMatrix());
        auto inverseView = glm::inverse(Mat3(m_ViewportCamera.GetViewMatrix()));

        Vec4 ray = inverseProj * mouseClipPos;
        Vec3 rayPos = m_ViewportCamera.GetPosition();
        Vec3 rayDir = inverseView * Vec3(ray);

        return {rayPos, rayDir};
	}

}

/// -------------------------------------------------------

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
 * renderer_primitives.cpp
 * -------------------------------------------------------
 * Created: 02/03/2026
 * -------------------------------------------------------
 */
#include "renderer.h"
#include <colors.h>
#include <Editor/modules/scene_render.h>
#include <SceneryEditorX/core/time/fps_timer.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/scene/components/lights.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	void Renderer::DrawLine(const Vec3& from, const Vec3& to, const Color& color_from, const Color& color_to, float duration_sec /*= 0.0f*/)
	{
		if (duration_sec <= 0.0f)
		{
			// single frame line - add directly to render list
			m_Lines_Vertices.emplace_back(from, color_from);
			m_Lines_Vertices.emplace_back(to, color_to);
		}
		else
		{
			// persistent line - add to persistent list with expiration time
			PersistentLine line;
			line.from        = from;
			line.to          = to;
			line.color_from  = color_from;
			line.color_to    = color_to;
			line.expire_time = FPSTimer::GetTimeSec() + static_cast<double>(duration_sec);
			m_Persistent_Lines.push_back(line);
		}
	}

	// convenience overloads so callers may use simpler forms
	void Renderer::DrawLine(const Vec3& from, const Vec3& to)
	{
		DrawLine(from, to, Color(1, 1, 1, 1), Color(1, 1, 1, 1), 0.0f);
	}

	void Renderer::DrawLine(const Vec3& from, const Vec3& to, const Color& color, float duration_sec /*= 0.0f*/)
	{
		DrawLine(from, to, color, color, duration_sec);
	}

	void Renderer::DrawTriangle(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Color& color /*= DEBUG_COLOR*/, float duration_sec /*= 0.0f*/)
	{
		DrawLine(v0, v1, color, color, duration_sec);
		DrawLine(v1, v2, color, color, duration_sec);
		DrawLine(v2, v0, color, color, duration_sec);
	}

	void Renderer::DrawBox(const BoundingBox& box, const Color& color, float duration_sec /*= 0.0f*/)
	{
		const Vec3& min = box.GetMin();
		const Vec3& max = box.GetMax();
	
		DrawLine(Vec3(min.x, min.y, min.z), Vec3(max.x, min.y, min.z), color, color, duration_sec);
		DrawLine(Vec3(max.x, min.y, min.z), Vec3(max.x, max.y, min.z), color, color, duration_sec);
		DrawLine(Vec3(max.x, max.y, min.z), Vec3(min.x, max.y, min.z), color, color, duration_sec);
		DrawLine(Vec3(min.x, max.y, min.z), Vec3(min.x, min.y, min.z), color, color, duration_sec);
		DrawLine(Vec3(min.x, min.y, min.z), Vec3(min.x, min.y, max.z), color, color, duration_sec);
		DrawLine(Vec3(max.x, min.y, min.z), Vec3(max.x, min.y, max.z), color, color, duration_sec);
		DrawLine(Vec3(max.x, max.y, min.z), Vec3(max.x, max.y, max.z), color, color, duration_sec);
		DrawLine(Vec3(min.x, max.y, min.z), Vec3(min.x, max.y, max.z), color, color, duration_sec);
		DrawLine(Vec3(min.x, min.y, max.z), Vec3(max.x, min.y, max.z), color, color, duration_sec);
		DrawLine(Vec3(max.x, min.y, max.z), Vec3(max.x, max.y, max.z), color, color, duration_sec);
		DrawLine(Vec3(max.x, max.y, max.z), Vec3(min.x, max.y, max.z), color, color, duration_sec);
		DrawLine(Vec3(min.x, max.y, max.z), Vec3(min.x, min.y, max.z), color, color, duration_sec);
	}

	void Renderer::DrawCircle(const Vec3& center, const Vec3& axis, const float radius, uint32_t segment_count, const Color& color /*= DEBUG_COLOR*/, float duration_sec /*= 0.0f*/)
	{
		if (radius <= 0.0f)
			return;

		// need at least 4 segments
		segment_count = xMath::Max<uint32_t>(segment_count, static_cast<uint32_t>(4));

		std::vector<Vec3> points;
		points.reserve(segment_count + 1);
		points.resize(segment_count + 1);

		// compute points on circle
		float angleStep = xMath::TWO_PI / static_cast<float>(segment_count);
		for (uint32_t i = 0; i <= segment_count; i++)
		{
			float angle = static_cast<float>(i) * angleStep;
			if (axis.x != 0.0f)
			{
				points[i] = Vec3(center.x, cos(angle) * radius + center.y, sin(angle) * radius + center.z);
			}
			else if (axis.y != 0.0f)
			{
				points[i] = Vec3(cos(angle) * radius + center.x, center.y, sin(angle) * radius + center.z);
			}
			else
			{
				points[i] = Vec3(cos(angle) * radius + center.x, sin(angle) * radius + center.y, center.z);
			}
		}

		// draw
		for (uint32_t i = 0; i <= segment_count - 1; i++)
		{
			DrawLine(points[i], points[i + 1], color, color, duration_sec);
		}
	}

	void Renderer::DrawSphere(const Vec3& center, float radius, uint32_t segment_count, const Color& color /*= DEBUG_COLOR*/, float duration_sec /*= 0.0f*/)
	{
		// need at least 4 segments
		segment_count = xMath::Max(segment_count, static_cast<uint32_t>(4));

		Vec3 Vertex1, Vertex2, Vertex3, Vertex4;
		const float AngleInc = 2.f * xMath::PI / static_cast<float>(segment_count);
		uint32_t NumSegmentsY = segment_count;
		float Latitude = AngleInc;
		uint32_t NumSegmentsX;
		float Longitude;
		float SinY1 = 0.0f, CosY1 = 1.0f, SinY2, CosY2;
		float SinX, CosX;

		while (NumSegmentsY--)
		{
			SinY2 = sin(Latitude);
			CosY2 = cos(Latitude);

			Vertex1 = Vec3(SinY1, 0.0f, CosY1) * radius + center;
			Vertex3 = Vec3(SinY2, 0.0f, CosY2) * radius + center;
			Longitude = AngleInc;

			NumSegmentsX = segment_count;
			while (NumSegmentsX--)
			{
				SinX = sin(Longitude);
				CosX = cos(Longitude);

				Vertex2 = Vec3((CosX * SinY1), (SinX * SinY1), CosY1) * radius + center;
				Vertex4 = Vec3((CosX * SinY2), (SinX * SinY2), CosY2) * radius + center;

				DrawLine(Vertex1, Vertex2, color, color, duration_sec);
				DrawLine(Vertex1, Vertex3, color, color, duration_sec);

				Vertex1 = Vertex2;
				Vertex3 = Vertex4;
				Longitude += AngleInc;
			}
			SinY1 = SinY2;
			CosY1 = CosY2;
			Latitude += AngleInc;
		}
	}

	void Renderer::DrawDirectionalArrow(const Vec3& start, const Vec3& end, float arrow_size, const Color& color /*= DEBUG_COLOR*/, float duration_sec /*= 0.0f*/)
	{
		arrow_size = xMath::Max(0.1f, arrow_size);

		DrawLine(start, end, color, color, duration_sec);

		Vec3 Dir = (end - start);
		Dir = xMath::Normalize(Dir);
		Vec3 Up(0.0f, 0.0f, 1.0f);
		Vec3 Right = xMath::Cross(Dir, Up);
		if (xMath::Length2(Right) < 1e-6f)
		{
			// Dir is parallel to world-up; choose another up and recompute
			Up = Vec3(0.0f, 1.0f, 0.0f);
			Right = xMath::Cross(Dir, Up);
		}
		Right = xMath::Normalize(Right);
		Up = xMath::Cross(Right, Dir);

		Matrix TM;
		TM.m00 = Dir.x;   TM.m01 = Dir.y;   TM.m02 = Dir.z;
		TM.m10 = Right.x; TM.m11 = Right.y; TM.m12 = Right.z;
		TM.m20 = Up.x;    TM.m21 = Up.y;    TM.m22 = Up.z;

		// since dir is x direction, my arrow will be pointing +y, -x and -y, -x
		float arrow_sqrt = sqrt(arrow_size);
		Vec3 arrow_pos;
		DrawLine(end, end + TM * Vec3(-arrow_sqrt, arrow_sqrt, 0), color, color, duration_sec);
		DrawLine(end, end + TM * Vec3(-arrow_sqrt, -arrow_sqrt, 0), color, color, duration_sec);
	}

	void Renderer::DrawPlane(const xMath::Plane& plane, const Color& color /*= DEBUG_COLOR*/, float duration_sec /*= 0.0f*/)
	{
		// arrow indicating normal
		Vec3 plane_origin = plane.normal * plane.d;
		DrawDirectionalArrow(plane_origin, plane_origin + plane.normal * 2.0f, 0.2f, color, duration_sec);

		Vec3 U, V;
		// build an orthonormal basis for the plane normal
		U = xMath::Cross(plane.normal, Vec3(0.0f, 0.0f, 1.0f));
		if (xMath::Length2(U) < 1e-6f)
			U = Vec3(1.0f, 0.0f, 0.0f);
		U = xMath::Normalize(U);
		V = xMath::Normalize(xMath::Cross(plane.normal, U));
		static const float SCALE = 10000.0f;
		DrawLine(plane_origin - U * SCALE, plane_origin + U * SCALE, color, color, duration_sec);
		DrawLine(plane_origin - V * SCALE, plane_origin + V * SCALE, color, color, duration_sec);
	}

	void Renderer::UpdatePersistentLines()
	{
		double current_time = SceneryEditorX::FPSTimer::GetTimeSec();

		// add non-expired persistent lines to the render list and remove expired ones
		for (auto it = m_Persistent_Lines.begin(); it != m_Persistent_Lines.end();)
		{
			if (current_time < it->expire_time)
			{
				// line is still valid, add to render list
				m_Lines_Vertices.emplace_back(it->from, it->color_from);
				m_Lines_Vertices.emplace_back(it->to, it->color_to);
				++it;
			}
			else
			{
				// line has expired, remove it
				it = m_Persistent_Lines.erase(it);
			}
		}
	}

	void Renderer::AddLinesToBeRendered()
	{

		// Debug: draw scene lights (iterate all entities and draw light gizmos)
		if (Camera* camera = Scene::GetCamera())
		{
			for (Entity* entity : Scene::GetEntities())
			{
				if (!entity)
					continue;

				if (Component* comp = entity->GetComponentByType(ComponentType::Light))
				{
					Light* light = dynamic_cast<Light*>(comp);
					if (!light) continue;

					if (light->GetLightType() == LightType::Directional)
					{
						Vec3 pos = light->GetEntity()->GetPosition() - light->GetEntity()->GetForward() * FLT_MAX;
						DrawDirectionalArrow(pos, Vec3(0.0f, 0.0f, 0.0f), 2.5f, Color(1.0f,1.0f,1.0f,1.0f), 0.0f);
					}
					else if (light->GetLightType() == LightType::Point)
					{
						Vec3 center = light->GetEntity()->GetPosition();
						float radius   = light->GetRange();
						uint32_t segment_count = 64;

						DrawCircle(center, Vec3(0.0f, 1.0f, 0.0f), radius, segment_count, Color(1.0f,1.0f,1.0f,1.0f), 0.0f);
						DrawCircle(center, Vec3(1.0f, 0.0f, 0.0f), radius, segment_count, Color(1.0f,1.0f,1.0f,1.0f), 0.0f);
						DrawCircle(center, Vec3(0.0f, 0.0f, 1.0f), radius, segment_count, Color(1.0f,1.0f,1.0f,1.0f), 0.0f);
					}
					else if (light->GetLightType() == LightType::Spot)
					{
						float opposite = light->GetRange() * tan(light->GetAngle());

						Vec3 pos_end_center = light->GetEntity()->GetForward() * light->GetRange();
						Vec3 pos_end_up     = pos_end_center + light->GetEntity()->GetUp()    * opposite;
						Vec3 pos_end_right  = pos_end_center + light->GetEntity()->GetRight() * opposite;
						Vec3 pos_end_down   = pos_end_center + light->GetEntity()->GetDown()  * opposite;
						Vec3 pos_end_left   = pos_end_center + light->GetEntity()->GetLeft()  * opposite;

						Vec3 pos_start = light->GetEntity()->GetPosition();
						DrawLine(pos_start, pos_start + pos_end_center, Color(1,1,1,1), Color(1,1,1,1), 0.0f);
						DrawLine(pos_start, pos_start + pos_end_up, Color(1,1,1,1), Color(1,1,1,1), 0.0f);
						DrawLine(pos_start, pos_start + pos_end_right, Color(1,1,1,1), Color(1,1,1,1), 0.0f);
						DrawLine(pos_start, pos_start + pos_end_down, Color(1,1,1,1), Color(1,1,1,1), 0.0f);
						DrawLine(pos_start, pos_start + pos_end_left, Color(1,1,1,1), Color(1,1,1,1), 0.0f);
					}
					else if (light->GetLightType() == LightType::Area)
					{
						Vec3 center     = light->GetEntity()->GetPosition();
						Vec3 right      = light->GetEntity()->GetRight();
						Vec3 up         = light->GetEntity()->GetUp();
						Vec3 forward    = light->GetEntity()->GetForward();
						float half_width   = light->GetAreaWidth() * 0.5f;
						float half_height  = light->GetAreaHeight() * 0.5f;

						Vec3 corner_tl = center - right * half_width + up * half_height; // top-left
						Vec3 corner_tr = center + right * half_width + up * half_height; // top-right
						Vec3 corner_br = center + right * half_width - up * half_height; // bottom-right
						Vec3 corner_bl = center - right * half_width - up * half_height; // bottom-left

						DrawLine(corner_tl, corner_tr, Color(1,1,1,1), Color(1,1,1,1), 0.0f);
						DrawLine(corner_tr, corner_br, Color(1,1,1,1), Color(1,1,1,1), 0.0f);
						DrawLine(corner_br, corner_bl, Color(1,1,1,1), Color(1,1,1,1), 0.0f);
						DrawLine(corner_bl, corner_tl, Color(1,1,1,1), Color(1,1,1,1), 0.0f);

						float arrow_length = xMath::Min(half_width, half_height) * 0.5f;
						DrawDirectionalArrow(center, center + forward * arrow_length, arrow_length * 0.3f, Color(1.0f,1.0f,1.0f,1.0f), 0.0f);
					}
				}
			}
		}
		
		// Draw AABBs for all entities (editor debug visualization)
		{
			auto get_color = [](Render* renderable)
			{
				const Color color_visible  = Color(0.8f, 0.8f, 0.8f, 1.0f);
				const Color color_occluded = Color(1.0f, 0.0f, 0.0f, 1.0f);
				return renderable->IsVisible() ? color_visible : color_occluded;
			};

			for (Entity* entity : Scene::GetEntities())
			{
				if (!entity)
					continue;

				// meshes
				if (Component* comp = entity->GetComponentByType(ComponentType::Renderable))
				{
					Render* renderable = static_cast<Render*>(comp);
					if (Camera* camera = Scene::GetCamera())
					{
						const Vec3 camera_position   = camera->GetEntity() ? camera->GetEntity()->GetPosition() : camera->GetEyePosition();
						const BoundingBox& bounding_box = renderable->GetBoundingBox();
						const Vec3 closest = bounding_box.GetClosestPoint(camera_position);
						const float distance = xMath::Distance(closest, camera_position);

						if (distance > renderable->GetMaxRenderDistance())
							continue;

						DrawBox(bounding_box, get_color(renderable), 0.0f);
					}
				}

				// lights
				if (Component* comp = entity->GetComponentByType(ComponentType::Light))
				{
					Light* light = static_cast<Light*>(comp);
					DrawBox(light->GetBoundingBox(), Color::Yellow(), 0.0f);
				}
			}
		}
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

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
 * spline.cpp
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#include "spline.h"
#include <algorithm>
#include <cmath>
#include <string_view>
#include <SceneryEditorX/renderer/renderer_declarations.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/mesh.h>
#include <SceneryEditorX/scene/scene.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		constexpr const char* K_CONTROL_POINT_PREFIX = "spline_point_";
		constexpr const char* K_INSTANCE_PREFIX = "spline_instance_";

		[[nodiscard]] bool StartsWith(const std::string& value, const std::string_view prefix)
		{
			return value.starts_with(prefix.data());
		}
	}

	Spline::Spline(Entity* entity) : Component(entity)
	{
	}

	Spline::~Spline()
	{
		ClearRoadMesh();
	}

	void Spline::Tick()
	{
		if (!m_mesh_enabled)
		{
			if (HasRoadMesh())
			{
				ClearRoadMesh();
			}
			return;
		}

		const std::vector<Vec3> controlPoints = GetControlPointsLocal();
		if (controlPoints.size() < 2)
		{
			ClearRoadMesh();
			return;
		}

		const bool dirty = (m_prev_closed_loop != m_closed_loop)
			|| (m_prev_resolution != m_resolution)
			|| (m_prev_spline_type != m_spline_type)
			|| (m_prev_height != m_height)
			|| (m_prev_thickness != m_thickness)
			|| (m_prev_uv_tiling_u != m_uv_tiling_u)
			|| (m_prev_uv_tiling_v != m_uv_tiling_v)
			|| (m_prev_conform_to_terrain != m_conform_to_terrain)
			|| (m_prev_terrain_offset != m_terrain_offset)
			|| (m_prev_control_points != controlPoints)
			|| !HasRoadMesh();

		if (!dirty)
		{
			return;
		}

		GenerateRoadMesh();
		m_prev_closed_loop = m_closed_loop;
		m_prev_resolution = m_resolution;
		m_prev_spline_type = m_spline_type;
		m_prev_height = m_height;
		m_prev_thickness = m_thickness;
		m_prev_uv_tiling_u = m_uv_tiling_u;
		m_prev_uv_tiling_v = m_uv_tiling_v;
		m_prev_conform_to_terrain = m_conform_to_terrain;
		m_prev_terrain_offset = m_terrain_offset;
		m_prev_control_points = controlPoints;
	}

	Vec3 Spline::GetPoint(float t) const
	{
		return EvaluatePoint(GetControlPoints(), t);
	}

	Vec3 Spline::GetTangent(float t) const
	{
		return EvaluateTangent(GetControlPoints(), t);
	}

	float Spline::GetLength(uint32_t samples_per_span) const
	{
		const std::vector<Vec3> points = GetControlPoints();
		if (points.size() < 2 || samples_per_span == 0)
		{
			return 0.0f;
		}

		const uint32_t spanCount = m_closed_loop ? static_cast<uint32_t>(points.size()) : static_cast<uint32_t>(points.size() - 1);
		const uint32_t totalSamples = std::max(1u, spanCount * samples_per_span);

		float length = 0.0f;
		Vec3 previous = EvaluatePoint(points, 0.0f);
		for (uint32_t i = 1; i <= totalSamples; ++i)
		{
			const float t = static_cast<float>(i) / static_cast<float>(totalSamples);
			const Vec3 current = EvaluatePoint(points, t);
			length += xMath::Distance(previous, current);
			previous = current;
		}

		return length;
	}

	uint32_t Spline::GetControlPointCount() const
	{
		if (!m_EntityPtr)
			return 0;

		uint32_t count = 0;
		for (const UUID& childId : m_EntityPtr->Children())
		{
			Entity child = Scene::TryGetEntityWithUUID(childId);
			if (child && StartsWith(child.Name(), K_CONTROL_POINT_PREFIX))
			{
				++count;
			}
		}

		return count;
	}

	void Spline::AddControlPoint(const Vec3& local_position)
	{
		if (!m_EntityPtr)
			return;

		const uint32_t index = GetControlPointCount();
		Entity point = Scene::CreateEntity(std::string(K_CONTROL_POINT_PREFIX) + std::to_string(index));
		if (!point)
			return;

		point.SetParent(*m_EntityPtr);
		point.SetPositionLocal(local_position);
	}

	void Spline::RemoveLastControlPoint()
	{
		if (!m_EntityPtr)
			return;

		UUID lastControlPoint{};
		for (const UUID& childId : m_EntityPtr->Children())
		{
			Entity child = Scene::TryGetEntityWithUUID(childId);
			if (child && StartsWith(child.Name(), K_CONTROL_POINT_PREFIX))
			{
				lastControlPoint = child.GetUUID();
			}
		}

		if (lastControlPoint != UUID())
		{
			if (const Entity child = Scene::TryGetEntityWithUUID(lastControlPoint))
			{
				Scene::DestroyEntity(child);
			}
		}
	}

	void Spline::GenerateRoadMesh()
	{
		const std::vector<Vec3> splinePoints = GetControlPointsLocal();
		if (splinePoints.size() < 2)
		{
			ClearRoadMesh();
			return;
		}

		GenerateMesh(splinePoints, GetProfilePoints(), IsProfileClosed());
	}

	void Spline::ClearRoadMesh()
	{
		m_Mesh.Reset();
	}

	void Spline::SpawnInstances()
	{
		if (!m_EntityPtr)
			return;

		ClearInstances();

		const std::vector<Vec3> points = GetControlPointsLocal();
		if (points.size() < 2 || m_instance_spacing <= 0.001f)
		{
			return;
		}

		const float totalLength = GetLength();
		if (totalLength <= 0.0f)
		{
			return;
		}

		const uint32_t sampleCount = std::max(8u, m_resolution * static_cast<uint32_t>(points.size()) * 4u);
		float walkedDistance = 0.0f;
		float nextSpawnDistance = 0.0f;
		Vec3 previous = EvaluatePoint(points, 0.0f);
		uint32_t spawned = 0;

		for (uint32_t i = 1; i <= sampleCount; ++i)
		{
			const float t = static_cast<float>(i) / static_cast<float>(sampleCount);
			Vec3 current = EvaluatePoint(points, t);
			walkedDistance += xMath::Distance(previous, current);
			previous = current;

			if (walkedDistance < nextSpawnDistance)
				continue;

			Entity instance = Scene::CreateEntity(std::string(K_INSTANCE_PREFIX) + std::to_string(spawned));
			if (!instance)
				continue;

			instance.SetParent(*m_EntityPtr);
			instance.SetPositionLocal(current);

			if (m_align_instances_to_spline)
			{
				const Vec3 tangent = EvaluateTangent(points, t);
				const float yaw = std::atan2(tangent.x, tangent.z);
				instance.SetRotationLocal(Vec3(0.0f, yaw, 0.0f));
			}

			if (m_instance_random_scale_min != 1.0f || m_instance_random_scale_max != 1.0f)
			{
				const float scale = xMath::Random<float>(m_instance_random_scale_min, m_instance_random_scale_max);
				instance.SetScaleLocal(Vec3(scale, scale, scale));
			}

			nextSpawnDistance += m_instance_spacing;
			++spawned;
		}
	}

	void Spline::ClearInstances()
	{
		if (!m_EntityPtr)
			return;

		std::vector<UUID> toDestroy;
		for (const UUID& childId : m_EntityPtr->Children())
		{
			Entity child = Scene::TryGetEntityWithUUID(childId);
			if (child && StartsWith(child.Name(), K_INSTANCE_PREFIX))
			{
				toDestroy.push_back(child.GetUUID());
			}
		}

		for (const UUID& childId : toDestroy)
		{
			if (const Entity child = Scene::TryGetEntityWithUUID(childId))
			{
				Scene::DestroyEntity(child);
			}
		}
	}

	std::vector<Vec3> Spline::GetControlPoints() const
	{
		std::vector<Vec3> points;
		if (!m_EntityPtr)
			return points;

		for (const UUID& childId : m_EntityPtr->Children())
		{
			Entity child = Scene::TryGetEntityWithUUID(childId);
			if (child && StartsWith(child.Name(), K_CONTROL_POINT_PREFIX))
			{
				points.push_back(child.GetPosition());
			}
		}

		return points;
	}

	std::vector<Vec3> Spline::GetControlPointsLocal() const
	{
		std::vector<Vec3> points;
		if (!m_EntityPtr)
			return points;

		for (const UUID& childId : m_EntityPtr->Children())
		{
			Entity child = Scene::TryGetEntityWithUUID(childId);
			if (child && StartsWith(child.Name(), K_CONTROL_POINT_PREFIX))
			{
				points.push_back(child.GetPositionLocal());
			}
		}

		return points;
	}

	std::vector<Vec2> Spline::GetProfilePoints() const
	{
		return GetProfilePointsForWidth(m_thickness);
	}

	std::vector<Vec2> Spline::GetProfilePointsForWidth(float width) const
	{
		const float halfWidth = std::max(width, 0.001f) * 0.5f;
		return {
			Vec2(-halfWidth, 0.0f),
			Vec2(-halfWidth, m_height),
			Vec2(halfWidth, m_height),
			Vec2(halfWidth, 0.0f)
		};
	}

	bool Spline::IsProfileClosed() const
	{
		return false;
	}

	void Spline::GenerateMesh(const std::vector<Vec3>& spline_points, const std::vector<Vec2>& profile_points, bool /*close_profile*/)
	{
		if (spline_points.size() < 2 || profile_points.size() < 2)
		{
			m_Mesh.Reset();
			return;
		}

		m_Mesh = CreateRef<Mesh>();
	}

	Vec3 Spline::CatmullRom(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t)
	{
		const float t2 = t * t;
		const float t3 = t2 * t;
		return 0.5f * ((2.0f * p1)
			+ (-p0 + p2) * t
			+ (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
			+ (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
	}

	Vec3 Spline::CatmullRomTangent(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t)
	{
		const float t2 = t * t;
		return 0.5f * ((-p0 + p2)
			+ (4.0f * p0 - 10.0f * p1 + 8.0f * p2 - 2.0f * p3) * t
			+ (-3.0f * p0 + 9.0f * p1 - 9.0f * p2 + 3.0f * p3) * t2);
	}

	Vec3 Spline::EvaluatePoint(const std::vector<Vec3>& points, float t) const
	{
		if (points.empty())
		{
			return {0.0f, 0.0f, 0.0f};
		}
		if (points.size() == 1)
		{
			return points.front();
		}

		t = std::clamp(t, 0.0f, 1.0f);

		if (m_spline_type == SplineType::Bezier && points.size() >= 4)
		{
			const uint32_t segmentCount = std::max(1u, static_cast<uint32_t>((points.size() - 1) / 3));
			const float scaledT = t * static_cast<float>(segmentCount);
			const uint32_t segment = std::min(segmentCount - 1, static_cast<uint32_t>(scaledT));
			const float localT = scaledT - static_cast<float>(segment);

			const size_t i0 = static_cast<size_t>(segment) * 3;
			const size_t i1 = std::min(i0 + 1, points.size() - 1);
			const size_t i2 = std::min(i0 + 2, points.size() - 1);
			const size_t i3 = std::min(i0 + 3, points.size() - 1);

			const float u = 1.0f - localT;
			const float uu = u * u;
			const float uuu = uu * u;
			const float tt = localT * localT;
			const float ttt = tt * localT;

			return (uuu * points[i0])
				+ (3.0f * uu * localT * points[i1])
				+ (3.0f * u * tt * points[i2])
				+ (ttt * points[i3]);
		}

		uint32_t spanIndex = 0;
		float localT = 0.0f;
		MapToSpan(t, points, spanIndex, localT);

		const int32_t pointCount = static_cast<int32_t>(points.size());
		const int32_t i1 = static_cast<int32_t>(spanIndex);
		const int32_t i2 = m_closed_loop ? (i1 + 1) % pointCount : xMath::Min(i1 + 1, pointCount - 1);
		const int32_t i0 = m_closed_loop ? (i1 - 1 + pointCount) % pointCount : xMath::Max(i1 - 1, 0);
		const int32_t i3 = m_closed_loop ? (i2 + 1) % pointCount : xMath::Min(i2 + 1, pointCount - 1);

		return CatmullRom(points[i0], points[i1], points[i2], points[i3], localT);
	}

	Vec3 Spline::EvaluateTangent(const std::vector<Vec3>& points, float t) const
	{
		if (points.size() < 2)
		{
			return {0.0f, 0.0f, 1.0f};
		}

		t = std::clamp(t, 0.0f, 1.0f);

		if (m_spline_type == SplineType::Bezier && points.size() >= 4)
		{
			const uint32_t segmentCount = std::max(1u, static_cast<uint32_t>((points.size() - 1) / 3));
			const float scaledT = t * static_cast<float>(segmentCount);
			const uint32_t segment = std::min(segmentCount - 1, static_cast<uint32_t>(scaledT));
			const float localT = scaledT - static_cast<float>(segment);

			const size_t i0 = static_cast<size_t>(segment) * 3;
			const size_t i1 = std::min(i0 + 1, points.size() - 1);
			const size_t i2 = std::min(i0 + 2, points.size() - 1);
			const size_t i3 = std::min(i0 + 3, points.size() - 1);

			const float u = 1.0f - localT;
			Vec3 tangent =
				3.0f * u * u * (points[i1] - points[i0])
				+ 6.0f * u * localT * (points[i2] - points[i1])
				+ 3.0f * localT * localT * (points[i3] - points[i2]);

			const float length = xMath::Length(tangent);
			if (length > 0.0001f)
			{
				tangent /= length;
			}

			return tangent;
		}

		uint32_t spanIndex = 0;
		float localT = 0.0f;
		MapToSpan(t, points, spanIndex, localT);

		const int32_t pointCount = static_cast<int32_t>(points.size());
		const int32_t i1 = static_cast<int32_t>(spanIndex);
		const int32_t i2 = m_closed_loop ? (i1 + 1) % pointCount : xMath::Min(i1 + 1, pointCount - 1);
		const int32_t i0 = m_closed_loop ? (i1 - 1 + pointCount) % pointCount : xMath::Max(i1 - 1, 0);
		const int32_t i3 = m_closed_loop ? (i2 + 1) % pointCount : xMath::Min(i2 + 1, pointCount - 1);

		Vec3 tangent = CatmullRomTangent(points[i0], points[i1], points[i2], points[i3], localT);
		const float length = xMath::Length(tangent);
		if (length > 0.0001f)
		{
			tangent /= length;
		}

		return tangent;
	}

	void Spline::MapToSpan(float t, const std::vector<Vec3>& points, uint32_t& span_index, float& local_t) const
	{
		const uint32_t spanCount = m_closed_loop ? static_cast<uint32_t>(points.size()) : static_cast<uint32_t>(points.size() - 1);
		if (spanCount == 0)
		{
			span_index = 0;
			local_t = 0.0f;
			return;
		}

		t = std::clamp(t, 0.0f, 1.0f);
		const float scaled = t * static_cast<float>(spanCount);
		span_index = std::min(spanCount - 1, static_cast<uint32_t>(scaled));
		local_t = scaled - static_cast<float>(span_index);
	}
}

// -------------------------------------------------------

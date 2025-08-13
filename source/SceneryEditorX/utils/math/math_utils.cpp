/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* math_utils.cpp - Mathematical utility functions
* -------------------------------------------------------
* Created: 12/8/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/utils/math/math_utils.h>
#include <SceneryEditorX/utils/math/vector.h>
#include <SceneryEditorX/utils/math/transforms.h>
#include <SceneryEditorX/utils/math/quat.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

/// -----------------------------------------------------

namespace SceneryEditorX::Utils
{

	bool IsEqual(float a, float b, float epsilon)
	{
	    return fabsf(a - b) <= epsilon;
	}

	bool IsZero(float value, float epsilon)
	{
	    return fabsf(value) <= epsilon;
	}

	float Distance(const Vec3& a, const Vec3& b)
	{
	    const float dx = b.x - a.x;
	    const float dy = b.y - a.y;
	    const float dz = b.z - a.z;
	    return sqrtf(dx*dx + dy*dy + dz*dz);
	}

	Vec3 Normalize(const Vec3& v)
	{
	    const float len2 = v.x*v.x + v.y*v.y + v.z*v.z;
	    if (len2 <= 0.0f)
	    {
	        return Vec3(0.0f, 0.0f, 0.0f);
	    }
	    const float invLen = 1.0f / sqrtf(len2);
	    return Vec3(v.x*invLen, v.y*invLen, v.z*invLen);
	}

	float Dot(const Vec3& a, const Vec3& b)
	{
	    return a.x*b.x + a.y*b.y + a.z*b.z;
	}

	Vec3 Cross(const Vec3& a, const Vec3& b)
	{
	    return Vec3(
	        a.y * b.z - a.z * b.y,
	        a.z * b.x - a.x * b.z,
	        a.x * b.y - a.y * b.x
	    );
	}

	// ---------------------------------------------------------------------
	// Compatibility wrappers (glm::quat <-> native Quat)
	bool Math::DecomposeTransform(const Mat4 &mat, Vec3 &translation, glm::quat &rotation, Vec3 &scale)
	{
		Quat nativeQ;
		const bool ok = Utils::DecomposeTransform(mat, translation, nativeQ, scale);
		if (ok)
			rotation = glm::quat(nativeQ.w, nativeQ.x, nativeQ.y, nativeQ.z);
		return ok;
	}

	Mat4 Math::ComposeTransform(const Vec3 &translation, const glm::quat &rotation, const Vec3 &scale)
	{
		const Quat q(rotation.w, rotation.x, rotation.y, rotation.z);
		return Utils::ComposeTransform(translation, q, scale);
	}

}

/// -------------------------------------------------------

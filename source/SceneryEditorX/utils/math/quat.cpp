/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* quat.cpp
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/utils/math/math_utils.h>
#include <SceneryEditorX/utils/math/matrix.h>
#include <SceneryEditorX/utils/math/quat.h>
#include <tracy/Tracy.hpp>

/// -----------------------------------------------------

namespace SceneryEditorX
{

	Quat::Quat() : x(0), y(0), z(0), w(1) {}

	Quat::Quat(const float w, const float x, const float y, const float z) : x(x), y(y), z(z), w(w) {}

	Quat::Quat(const Vec4& vec4) : x(vec4.x), y(vec4.y), z(vec4.z), w(vec4.w) {}

	Quat::Quat(const Quat& q)
	{
		w = q.w;
		x = q.x;
		y = q.y;
		z = q.z;
	}

	Quat::~Quat() = default;

	Quat& Quat::operator=(const Quat& q)
	{
		if (this == &q)
			return *this;
		w = q.w;
		x = q.x;
		y = q.y;
		z = q.z;
		return *this;
	}

	bool Quat::operator==(const Quat& rhs) const { return w == rhs.w && x == rhs.x && y == rhs.y && z == rhs.z; }

	bool Quat::operator!=(const Quat& rhs) const { return !(*this == rhs); }

	Quat& Quat::operator*=(const Quat& rhs)
	{
		Quat q;

		q.w = w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z;
		q.x = w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y;
		q.y = w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x;
		q.z = w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w;

		*this = q;
		return *this;
	}

	Quat Quat::operator*(const Quat& rhs) const
	{
		Quat q;

		q.w = w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z;
		q.x = w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y;
		q.y = w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x;
		q.z = w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w;

		return q;
	}

	Quat Quat::operator*(const float rhs) const { return {w * rhs, x * rhs, y * rhs, z * rhs}; }
	Quat Quat::operator+(const Quat& rhs) const { return {w + rhs.w, x + rhs.x, y + rhs.y, z + rhs.z}; }
	Quat Quat::operator-(const Quat& rhs) const { return {w - rhs.w, x - rhs.x, y - rhs.y, z - rhs.z}; }
	Vec4 Quat::operator*(const Vec4& rhs) const { return ToMatrix(*this) * rhs; }
	Vec3 Quat::operator*(const Vec3& rhs) const { return ToMatrix(*this) * Vec4(rhs, 1.0f); }

	Quat Quat::GetNormalized() const
    {
		const float mag = sqrtf(w * w + x * x + y * y + z * z);
		return {w / mag, x / mag, y / mag, z / mag};
	}

	void Quat::Normalize()
	{
		const float mag = sqrtf(w * w + x * x + y * y + z * z);

		w /= mag;
		x /= mag;
		y /= mag;
		z /= mag;
	}

	Quat Quat::Conjugate() { return {w, -x, -y, -z}; }

	void Quat::SetEulerDegrees(float X, float Y, float Z)
	{
		X *= 0.0174532925f; ///< To radians!
		Y *= 0.0174532925f; ///< To radians!
		Z *= 0.0174532925f; ///< To radians!

		X *= 0.5f;
		Y *= 0.5f;
		Z *= 0.5f;

		const float sinx = sinf(X);
		const float siny = sinf(Y);
		const float sinz = sinf(Z);
		const float cosx = cosf(X);
		const float cosy = cosf(Y);
		const float cosz = cosf(Z);

		w = cosx * cosy * cosz + sinx * siny * sinz;
		this->x = sinx * cosy * cosz + cosx * siny * sinz;
		this->y = cosx * siny * cosz - sinx * cosy * sinz;
		this->z = cosx * cosy * sinz - sinx * siny * cosz;
	}

	float Quat::Dot(const Quat& b) const { return (w * b.w + x * b.x * y * b.y + z * b.z); }

	Matrix4x4 Quat::ToMatrix() const { return ToMatrix(*this); }

	Quat Quat::FromToRotation(const Vec3& from, const Vec3& to)
	{
		const Vec3 unitFrom = from.GetNormalized();
		const Vec3 unitTo = to.GetNormalized();
		const float dot = Vec3::Dot(unitFrom, unitTo);

		Vec3 cross = Vec3::Cross(unitFrom, unitTo);
		float w = sqrt((1 + dot) * 0.5);
		float k = sqrt((1 - dot) * 0.5);
		float scale = w > 0 ? 1.0 / w : 0;

		if (dot >= 1.0f)
            return {}; ///< identity

        if (dot <= -1.0f)
        {
            /**
             * If the two vectors are pointing in opposite directions then we
             * need to supply a quaternion corresponding to a rotation of
             * PI-radians about an axis orthogonal to the fromDirection.
             */
            Vec3 axis = Vec3::Cross(unitFrom, Vec3(1, 0, 0));
            if (axis.GetSqrMagnitude() < 1e-6)
            {
                /**
                 * Bad luck. The x-axis and fromDirection are linearly
                 * dependent (collinear). We'll take the axis as the vector
                 * orthogonal to both the y-axis and fromDirection instead.
                 * The y-axis and fromDirection will clearly not be linearly
                 * dependent.
                 */
                axis = Vec3::Cross(unitFrom, Vec3(0, 1, 0));
            }

            ///< Note that we need to normalize the axis as the cross product of two unit vectors is not necessarily a unit vector.
            return Quat::AngleAxis(180, axis.GetNormalized()); ///< 180 Degrees = PI radians
        }

        ///< Scalar component.
        const float s = sqrt(unitFrom.GetSqrMagnitude() * unitTo.GetSqrMagnitude()) + Vec3::Dot(unitFrom, unitTo);

        ///< Vector component.
        Vec3 v = Vec3::Cross(unitFrom, unitTo);
        v.x *= -1;
        v.y *= -1;

        ///< Return the normalized quaternion rotation.
        return Quat(Vec4(v, s)).GetNormalized();
    }

	Quat Quat::LookRotation(const Vec3& lookAt) { return FromToRotation(Vec3(0, 0, 1), lookAt); }

	///< TODO: Modify LookRotation entries after modifying the original function
	Quat Quat::LookRotation(const Vec3& lookAt, const Vec3& upDirection)
	{
		///< Calculate the unit quaternion that rotates Vector3::FORWARD to face in the specified forward direction.
		const Quat q1 = LookRotation(lookAt);

		/**
		 * We can't preserve the upwards direction if the forward and upwards
		 * vectors are linearly dependent (collinear).
		 */
		if (Vec3::Cross(lookAt, upDirection).GetSqrMagnitude() < 1e-6)
            return q1;

        ///< Determine the upwards direction obtained after applying q1.
		const Vec3 newUp = q1 * Vec3(0, 1, 0);

		/**
		 * Calculate the unit quaternion rotation that rotates the newUp
		 * direction to look in the specified upward direction.
		 */
		const Quat q2 = FromToRotation(newUp, upDirection);

		/**
		 * Return the combined rotation so that we first rotate to look in the
		 * forward direction and then rotate to align Vector3::UPWARD with the
		 * specified upward direction. There is no need to normalize the result
		 * as both q1 and q2 are unit quaternions.
		 */
		return q2 * q1;
	}

	Quat Quat::LookRotation2(const Vec3& forward, const Vec3& up)
	{
		Vec3 right = Vec3::Cross(up, forward);
		Quat result;
		result.w = sqrtf(1.0f + right.x + up.y + forward.z) * 0.5f;
		float w4_recip = 1.0f / (4.0f * result.w);
		result.x = (forward.y - up.z) * w4_recip;
		result.y = (right.z - forward.x) * w4_recip;
		result.z = (up.x - right.y) * w4_recip;
		return result;
	}

	Quat Quat::Slerp(const Quat& from, const Quat& to, const float t)
	{
		float cosTheta = Quat::Dot(from, to);
		Quat temp(to);

		if (cosTheta < 0.0f)
		{
			cosTheta *= -1.0f;
			temp = temp * -1.0f;
		}

		const float theta = acosf(cosTheta);
		const float sinTheta = 1.0f / sinf(theta);

		return sinTheta * (((Quat)(from * sinf(theta * (1.0f - t)))) + ((Quat)(temp * sinf(t * theta))));
	}

	Quat Quat::Lerp(const Quat& from, const Quat& to, const float t)
	{
		const Quat src = from * (1.0f - t);
		const Quat dst = to * t;

		Quat q = src + dst;
		return q;
	}

	float Quat::Angle(const Quat& a, const Quat& b)
	{
		const float degrees = acosf((b * a.GetInverse()).w) * 2.0f * 57.2957795f;
		if (degrees > 180.0f)
			return 360.0f - degrees;
		return degrees;
	}

	float Quat::Dot(const Quat& a, const Quat& b) { return a.Dot(b); }

	Quat Quat::AngleAxis(float angle, const Vec4& axis)
	{
		const Vec4 vn = axis.GetNormalized();

		angle *= 0.0174532925f; ///< To radians!
		angle *= 0.5f;
		const float sinAngle = sin(angle);

		return {cos(angle), vn.x * sinAngle, vn.y * sinAngle, vn.z * sinAngle};
	}

	Quat Quat::GetInverse() const
	{
		const float sqr = GetSqrMagnitude();
		const float invSqr = 1.0f / sqr;

		return {w * invSqr, -x * invSqr, -y * invSqr, -z * invSqr};
	}

	Quat Quat::EulerDegrees(float X, float Y, float Z)
	{
		X *= 0.0174532925f; ///< To radians!
		Y *= 0.0174532925f; ///< To radians!
		Z *= 0.0174532925f; ///< To radians!

		return EulerRadians(X, Y, Z);
	}

	Quat Quat::EulerRadians(float X, float Y, float Z)
	{
		///< NEW
		const float cy = cos(Z * 0.5);
		const float sy = sin(Z * 0.5);
		const float cp = cos(Y * 0.5);
		const float sp = sin(Y * 0.5);
		const float cr = cos(X * 0.5);
		const float sr = sin(X * 0.5);

		float qw = cr * cp * cy + sr * sp * sy;
		float qx = -sr * cp * cy + cr * sp * sy;
		float qy = cr * sp * cy + sr * cp * sy;
		float qz = cr * cp * sy - sr * sp * cy;

		//return Quat(qw, qx, qy, qz);

		///< OLD
		X *= 0.5f;
		Y *= 0.5f;
		Z *= 0.5f;

		float sinx = sinf(X);
		float siny = sinf(Y);
		float sinz = sinf(Z);
		float cosx = cosf(X);
		float cosy = cosf(Y);
		float cosz = cosf(Z);

		Quat q;

		q.w = cosx * cosy * cosz + sinx * siny * sinz;
		q.x = sinx * cosy * cosz + cosx * siny * sinz;
		q.y = cosx * siny * cosz - sinx * cosy * sinz;
		q.z = cosx * cosy * sinz - sinx * siny * cosz;

		return q;
	}

	Matrix4x4 Quat::ToMatrix(const Quat& q)
	{
		ZoneScoped;

		const float sqw = q.w * q.w;
		const float sqx = q.x * q.x;
		const float sqy = q.y * q.y;
		const float sqz = q.z * q.z;
		const float invs = 1.0f / (sqx + sqy + sqz + sqw);

		Matrix4x4 matrix = Matrix4x4::Identity();

		float* mat = &matrix.rows[0][0];

		mat[0] = (sqx - sqy - sqz + sqw) * invs;
		mat[5] = (-sqx + sqy - sqz + sqw) * invs;
		mat[10] = (-sqx - sqy + sqz + sqw) * invs;

		float tmp1 = q.x * q.y;
		float tmp2 = q.z * q.w;
		mat[4] = 2.0 * (tmp1 + tmp2) * invs;
		mat[1] = 2.0 * (tmp1 - tmp2) * invs;

		tmp1 = q.x * q.z;
		tmp2 = q.y * q.w;
		mat[8] = 2.0 * (tmp1 - tmp2) * invs;
		mat[2] = 2.0 * (tmp1 + tmp2) * invs;

		tmp1 = q.y * q.z;
		tmp2 = q.x * q.w;
		mat[9] = 2.0 * (tmp1 + tmp2) * invs;
		mat[6] = 2.0 * (tmp1 - tmp2) * invs;

		return matrix;
	}

	Vec3 Quat::ToEulerRadians() const
	{
		//const Quat& q = *this;

		const double sqw = w * w;
		const double sqx = x * x;
		const double sqy = y * y;
		const double sqz = z * z;
		const double unit = sqx + sqy + sqz + sqw;
		const double test = x * y + z * w;
		Vec3 euler;

		if (test > 0.499 * unit)
		{ ///< Singularity at north pole
			euler.y = 2 * atan2(x, w);
			euler.x = M_PI / 2;
			euler.z = 0;
			return euler;
		}
		if (test < -0.499 * unit)
		{ ///< Singularity at south pole
			euler.y = -2 * atan2(x, w);
			euler.x = -M_PI / 2;
			euler.z = 0;
			return euler;
		}
		euler.y = atan2(2 * y * w + 2 * x * z, sqw - sqx - sqy + sqz);	/// Yaw
		euler.x = asin(-2 * (x * z - y * w) / unit);								/// Pitch
		euler.z = atan2(2 * x * w + 2 * y * z, sqw + sqx - sqy - sqz);	/// Roll

		return euler;
	}

	/// -----------------------------------------------------

	Vec3 Quat::ToEulerDegrees() const { return ToEulerRadians() * Utils::RAD_TO_DEG; }
	Vec4 operator*(const Vec4& v, const Quat& m) { return Quat::ToMatrix(m) * v; }
	Vec3 operator*(const Vec3& v, const Quat& m) { return Quat::ToMatrix(m) * Vec4(v, 1.0f); }
	Quat operator*(const float f, const Quat& m) { return {m.w * f, m.x * f, m.y * f, m.z * f}; }

	/// -----------------------------------------------------

}

/// -----------------------------------------------------

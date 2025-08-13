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
// ReSharper disable IdentifierTypo
#include "quat.h"
#include "math_utils.h"
#if TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

/// -----------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @brief Default constructor creating an identity quaternion.
	 *
	 * Initializes the quaternion to represent no rotation (identity).
	 * The resulting quaternion has components (x=0, y=0, z=0, w=1).
	 */
	Quat::Quat() : x(0), y(0), z(0), w(1) {}

	/**
	 * @brief Constructs a quaternion from individual components.
	 *
	 * Creates a quaternion with the specified scalar and vector components
	 * in the order (w, x, y, z).
	 */
	Quat::Quat(const float w, const float x, const float y, const float z) : x(x), y(y), z(z), w(w) {}

	/**
	 * @brief Constructs a quaternion from a Vec4.
	 *
	 * Creates a quaternion by copying components from a Vec4 where
	 * vec4.x->quat.x, vec4.y->quat.y, vec4.z->quat.z, vec4.w->quat.w.
	 */
	Quat::Quat(const Vec4& vec4) : x(vec4.x), y(vec4.y), z(vec4.z), w(vec4.w) {}

	/**
	 * @brief Copy constructor.
	 *
	 * Creates a new quaternion by copying all components from another quaternion.
	 */
	Quat::Quat(const Quat& q)
	{
		w = q.w;
		x = q.x;
		y = q.y;
		z = q.z;
	}

	Quat::~Quat() = default;

	/**
	 * @brief Copy assignment operator with self-assignment protection.
	 *
	 * Assigns the components of another quaternion to this quaternion.
	 * Includes check for self-assignment to avoid unnecessary work.
	 */
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

	/**
	 * @brief Equality comparison operator.
	 *
	 * Compares two quaternions for exact component-wise equality.
	 * Returns true only if all four components are exactly equal.
	 */
	bool Quat::operator==(const Quat& rhs) const { return w == rhs.w && x == rhs.x && y == rhs.y && z == rhs.z; }

	/**
	 * @brief Inequality comparison operator.
	 *
	 * Returns true if any component differs between the two quaternions.
	 */
	bool Quat::operator!=(const Quat& rhs) const { return !(*this == rhs); }

	/**
	 * @brief Quaternion multiplication assignment operator.
	 *
	 * Multiplies this quaternion by another quaternion using Hamilton's
	 * quaternion product formula and stores the result in this quaternion.
	 * The operation represents composition of rotations.
	 */
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

	/**
	 * @brief Quaternion multiplication operator.
	 *
	 * Multiplies two quaternions using Hamilton's quaternion product formula.
	 * This represents the composition of two rotations where the right-hand
	 * side rotation is applied first, followed by the left-hand side rotation.
	 */
	Quat Quat::operator*(const Quat& rhs) const
	{
		Quat q;

		q.w = w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z;
		q.x = w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y;
		q.y = w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x;
		q.z = w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w;

		return q;
	}

	/**
	 * @brief Scalar multiplication operator.
	 *
	 * Multiplies all components of the quaternion by a scalar value.
	 * Primarily used for mathematical computations and interpolation.
	 */
	Quat Quat::operator*(const float rhs) const { return {w * rhs, x * rhs, y * rhs, z * rhs}; }

	/**
	 * @brief Component-wise addition operator.
	 *
	 * Adds corresponding components of two quaternions. Used primarily
	 * in mathematical computations and interpolation algorithms.
	 */
	Quat Quat::operator+(const Quat& rhs) const { return {w + rhs.w, x + rhs.x, y + rhs.y, z + rhs.z}; }

	/**
	 * @brief Component-wise subtraction operator.
	 *
	 * Subtracts corresponding components of two quaternions. Used primarily
	 * in mathematical computations and for calculating differences.
	 */
	Quat Quat::operator-(const Quat& rhs) const { return {w - rhs.w, x - rhs.x, y - rhs.y, z - rhs.z}; }

	/**
	 * @brief Transforms a Vec4 by this quaternion's rotation.
	 *
	 * Converts the quaternion to a matrix and multiplies the Vec4 by it.
	 * This applies the rotation represented by the quaternion to the vector.
	 */
	Vec4 Quat::operator*(const Vec4& rhs) const { return ToMatrix(*this) * rhs; }

	/**
	 * @brief Transforms a Vec3 by this quaternion's rotation.
	 *
	 * Converts the quaternion to a matrix and multiplies the Vec3 (extended
	 * to homogeneous coordinates with w=1) by it to apply the rotation.
	 */
	Vec3 Quat::operator*(const Vec3& rhs) const { return ToMatrix(*this) * Vec4(rhs, 1.0f); }

	/**
	 * @brief Returns a normalized copy of this quaternion.
	 *
	 * Creates a new quaternion with the same rotation as this quaternion
	 * but with unit length (magnitude = 1). Essential for proper rotation behavior.
	 *
	 * @return A new normalized quaternion representing the same rotation
	 */
	Quat Quat::GetNormalized() const
    {
		const float mag = sqrtf(w * w + x * x + y * y + z * z);
		return {w / mag, x / mag, y / mag, z / mag};
	}

	/**
	 * @brief Normalizes this quaternion in-place.
	 *
	 * Modifies this quaternion to have unit length while preserving
	 * the rotation it represents. This should be called after operations
	 * that might change the quaternion's magnitude.
	 */
	void Quat::Normalize()
	{
		const float mag = sqrtf(w * w + x * x + y * y + z * z);

		w /= mag;
		x /= mag;
		y /= mag;
		z /= mag;
	}

	/**
	 * @brief Returns the conjugate of this quaternion.
	 *
	 * The conjugate of quaternion (x, y, z, w) is (-x, -y, -z, w).
	 * For unit quaternions, this is equivalent to the inverse and
	 * represents the opposite rotation.
	 *
	 * @return A new quaternion that is the conjugate/inverse of this quaternion
	 */
	Quat Quat::Conjugate() { return {w, -x, -y, -z}; }

	/**
	 * @brief Sets this quaternion from Euler angles in degrees.
	 *
	 * Constructs a rotation quaternion from Euler angles using XYZ rotation order.
	 * The input angles are converted from degrees to radians and then used to
	 * compute the quaternion components using trigonometric functions.
	 *
	 * @param X Rotation around X-axis in degrees (pitch)
	 * @param Y Rotation around Y-axis in degrees (yaw)
	 * @param Z Rotation around Z-axis in degrees (roll)
	 */
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

	/**
	 * @brief Calculates the dot product with another quaternion.
	 *
	 * Computes the dot product between this quaternion and another quaternion.
	 * The dot product measures the similarity between two rotations and is used
	 * in interpolation algorithms.
	 *
	 * @param b The quaternion to compute dot product with
	 * @return The dot product value (1.0 = identical, -1.0 = opposite)
	 *
	 * @note There appears to be a bug in this implementation - it's missing
	 *       the y component multiplication (should be + x * b.x + y * b.y + z * b.z)
	 */
	float Quat::Dot(const Quat& b) const { return (w * b.w + x * b.x + y * b.y + z * b.z); }

	Matrix4x4 Quat::ToMatrix() const { return ToMatrix(*this); }

	/**
	 * @brief Creates a quaternion representing rotation from one vector to another.
	 *
	 * Calculates the shortest rotation that would align the 'from' vector with the 'to' vector.
	 * This function handles several special cases:
	 * - Parallel vectors (dot >= 1.0): Returns identity quaternion
	 * - Anti-parallel vectors (dot <= -1.0): Creates 180° rotation around perpendicular axis
	 * - General case: Uses cross product to determine rotation axis and angle
	 *
	 * @param from The source vector (normalized internally)
	 * @param to The target vector (normalized internally)
	 * @return A quaternion representing the rotation from 'from' to 'to'
	 */
	Quat Quat::FromToRotation(const Vec3& from, const Vec3& to)
	{
		const Vec3 unitFrom = SceneryEditorX::Utils::Normalize(from);
		const Vec3 unitTo = SceneryEditorX::Utils::Normalize(to);
		const float dot = SceneryEditorX::Utils::Dot(unitFrom, unitTo);

		if (dot >= 1.0f)
			return {}; // identity

		if (dot <= -1.0f)
		{
			// 180-degree rotation around any axis orthogonal to from
			Vec3 axis = SceneryEditorX::Utils::Cross(unitFrom, Vec3(1, 0, 0));
			if (SceneryEditorX::Utils::Dot(axis, axis) < 1e-6f)
			{
				axis = SceneryEditorX::Utils::Cross(unitFrom, Vec3(0, 1, 0));
			}

			const Vec3 normAxis = SceneryEditorX::Utils::Normalize(axis);
			return Quat::AngleAxis(180.0f, Vec4(normAxis.x, normAxis.y, normAxis.z, 0.0f));
		}

		// General case
		const float s_from = SceneryEditorX::Utils::Dot(unitFrom, unitFrom);
		const float s_to = SceneryEditorX::Utils::Dot(unitTo, unitTo);
		const float s = sqrtf(s_from * s_to) + SceneryEditorX::Utils::Dot(unitFrom, unitTo);
		Vec3 v = SceneryEditorX::Utils::Cross(unitFrom, unitTo);
		v.x *= -1.0f;
		v.y *= -1.0f;
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
	if (SceneryEditorX::Utils::Dot(SceneryEditorX::Utils::Cross(lookAt, upDirection), SceneryEditorX::Utils::Cross(lookAt, upDirection)) == 0.0f)
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
	Vec3 right = SceneryEditorX::Utils::Cross(up, forward);
		Quat result;
		result.w = sqrtf(1.0f + right.x + up.y + forward.z) * 0.5f;
		float w4_recip = 1.0f / (4.0f * result.w);
		result.x = (forward.y - up.z) * w4_recip;
		result.y = (right.z - forward.x) * w4_recip;
		result.z = (up.x - right.y) * w4_recip;
		return result;
	}

	/**
	 * @brief Spherical linear interpolation between two quaternions.
	 *
	 * Performs SLERP (Spherical Linear Interpolation) which provides smooth rotation
	 * interpolation with constant angular velocity. This is the preferred method for
	 * animating rotations as it maintains the most natural motion.
	 *
	 * The algorithm:
	 * 1. Computes the dot product to measure quaternion similarity
	 * 2. Handles quaternion double-cover by negating if dot < 0
	 * 3. Uses spherical interpolation formula with sine weights
	 *
	 * @param from The starting quaternion
	 * @param to The ending quaternion
	 * @param t Interpolation parameter (0.0 = from, 1.0 = to)
	 * @return A quaternion smoothly interpolated between from and to
	 */
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
		return q.GetNormalized();
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
		// Normalize axis (x, y, z only)
		const float len = sqrtf(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
		Vec4 vn = axis;
		if (len > 0.0f)
		{
			const float inv = 1.0f / len;
			vn.x *= inv; vn.y *= inv; vn.z *= inv;
		}

		angle *= 0.0174532925f; // degrees to radians
		angle *= 0.5f;
		const float sinAngle = sinf(angle);

		return {cosf(angle), vn.x * sinAngle, vn.y * sinAngle, vn.z * sinAngle};
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

	/**
	 * @brief Converts a quaternion to a 4x4 rotation matrix.
	 *
	 * Transforms the quaternion representation into a 4x4 transformation matrix
	 * that represents the same rotation. This conversion is essential for interfacing
	 * with graphics APIs and combining with other transformation matrices.
	 *
	 * The conversion uses the standard quaternion-to-matrix formula:
	 * - Computes squared components for efficiency
	 * - Uses inverse normalization factor to handle non-unit quaternions
	 * - Applies optimized formula to directly compute matrix elements
	 *
	 * @param q The quaternion to convert (should be normalized for best results)
	 * @return A Matrix4x4 representing the same rotation with translation = 0
	 */
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

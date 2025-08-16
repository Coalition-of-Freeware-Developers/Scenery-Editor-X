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
#include <SceneryEditorX/utils/math/quat.h>
#include <SceneryEditorX/utils/math/math_utils.h>
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
	 *
	 * @note - This is equivalent to a rotation of 0 degrees around any axis.
	 *
	 * @code
	 * Quat identity; // Creates identity quaternion (0, 0, 0, 1)
	 * @endcode
	 */
	Quat::Quat() : x(0), y(0), z(0), w(1) {}

	/**
	 * @brief Constructs a quaternion from individual components.
	 *
	 * Creates a quaternion with the specified scalar and vector components
	 * in the order (w, x, y, z).
	 *
	 * @param w The scalar component (cos(θ/2))
	 * @param x The x component of the vector part (axis.x * sin(θ/2))
	 * @param y The y component of the vector part (axis.y * sin(θ/2))
	 * @param z The z component of the vector part (axis.z * sin(θ/2))
	 * @return A quaternion with the specified components
	 *
	 * @note - For valid rotations, the quaternion should be normalized.
	 * @note - The quaternion represents a rotation where the vector part
	 * (x, y, z) encodes the rotation axis scaled by sin(θ/2)
	 *
	 * @code
	 * Quat q(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
	 * Quat rotation(0.707f, 0.707f, 0.0f, 0.0f); // 90° rotation around X-axis
	 * @endcode
	 */
	Quat::Quat(const float w, const float x, const float y, const float z) : x(x), y(y), z(z), w(w) {}

	/**
	 * @brief Constructs a quaternion from a Vec4.
	 *
	 * Creates a quaternion by copying components from a Vec4 where
	 * vec4.x->quat.x, vec4.y->quat.y, vec4.z->quat.z, vec4.w->quat.w.
	 *
	 * @param vec4 The Vec4 containing quaternion components (x, y, z, w)
	 * @return A quaternion with the same components as vec4
	 *
	 * @note - This constructor allows creating a quaternion directly
	 * from a Vec4, which is useful for interfacing with APIs
	 * @note - The Vec4 is expected to have the components in the order
	 * x, y, z, w
	 *
	 * @code
	 * Vec4 components(0.0f, 0.0f, 0.0f, 1.0f);
	 * Quat q(components); // Creates quaternion from Vec4
	 * @endcode
	 */
	Quat::Quat(const Vec4& vec4) : x(vec4.x), y(vec4.y), z(vec4.z), w(vec4.w) {}

    /**
     * @note Storage Layout vs Parameter Order
     * Internally the quaternion stores components as (x,y,z,w) in memory so the
     * vector part is tightly packed at the beginning for cache/SIMD friendliness
     * with other math structures (Vec3/Vec4). The public constructor however
     * takes parameters in the common (w,x,y,z) order for readability. Always use
     * the named members (x,y,z,w) when accessing components. Do not assume
     * constructor parameter ordering matches memory layout for binary dumps or
     * serialization – consult the math library documentation first.
     */

	/**
	 * @brief Copy constructor.
	 *
	 * Creates a new quaternion by copying all components from another quaternion.
	 *
	 * @param q The quaternion to copy from
	 * @return A new quaternion with the same components as q
	 *
	 * @note - This constructor allows creating a new quaternion instance
	 * with the same values as an existing quaternion.
	 *
	 * @code
	 * Quat original = Quat::EulerDegrees(45, 0, 0);
	 * Quat copy(original); // Creates a copy of the original quaternion
	 * @endcode
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
	 *
	 * @param q The quaternion to copy from
	 * @return Reference to this quaternion after assignment
	 *
	 * @note - This operator allows for direct assignment of quaternions
	 * without needing to create a new instance
	 *
	 * @code
	 * Quat q1 = Quat::EulerDegrees(45, 0, 0);
	 * Quat q2;
	 * q2 = q1; // Assigns q1 to q2, copying all components
	 * @endcode
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
	 *
	 * @param rhs The quaternion to compare against
	 * @return true if all components are equal, false otherwise
	 *
	 * @note - This performs exact floating-point comparison, which may not be
	 * suitable for quaternions that have undergone floating-point arithmetic.
	 * Consider using an epsilon-based comparison for such cases.
	 * @note - Useful for checking if two quaternions represent the same rotation.
	 *
	 * @code
	 * Quat q1 = Quat::EulerDegrees(45, 0, 0);
	 * Quat q2 = Quat::EulerDegrees(45, 0, 0);
	 * bool same = (q1 == q2); // true if exactly equal
	 * @endcode
	 */
	bool Quat::operator==(const Quat& rhs) const { return w == rhs.w && x == rhs.x && y == rhs.y && z == rhs.z; }

	/**
	 * @brief Inequality comparison operator.
	 *
	 * Compares two quaternions for inequality.
	 *
	 * @param rhs The quaternion to compare against
	 * @return true if any components differ, false if all are equal
	 *
	 * @note - This is the logical negation of the equality operator.
	 * @note - Useful for checking if two quaternions represent different rotations.
	 *
	 * @code
	 * Quat q1 = Quat::EulerDegrees(45, 0, 0);
	 * Quat q2 = Quat::EulerDegrees(90, 0, 0);
	 * bool different = (q1 != q2); // true, different rotations
	 * @endcode
	 */
	bool Quat::operator!=(const Quat& rhs) const { return !(*this == rhs); }

	/**
	 * @brief Quaternion multiplication assignment operator.
	 *
	 * Multiplies this quaternion by another quaternion using Hamilton's
	 * quaternion product formula and stores the result in this quaternion.
	 * The operation represents composition of rotations.
	 *
	 * @param rhs The quaternion to multiply by
	 * @return Reference to this quaternion after multiplication
	 *
	 * @note - Quaternion multiplication is not commutative: A *= B ≠ B *= A
	 * @note - The result represents applying the right-hand side rotation first,
	 * then the left-hand side rotation.
	 *
	 * @code
	 * Quat rotX = Quat::EulerDegrees(90, 0, 0);
	 * Quat rotY = Quat::EulerDegrees(0, 45, 0);
	 * rotX *= rotY;
	 * @endcode
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
	 *
	 * @param rhs The quaternion to multiply by
	 * @return A new quaternion representing the composed rotation
	 *
	 * @note - Quaternion multiplication is not commutative: A * B ≠ B * A
	 * @note - The resulting quaternion represents applying the right-hand side
	 * rotation first, then the left-hand side rotation.
	 *
	 * @code
	 * Quat rotX = Quat::EulerDegrees(90, 0, 0); // 90° around X-axis
	 * Quat rotY = Quat::EulerDegrees(0, 45, 0); // 45° around Y-axis
	 * Quat combined = rotX * rotY; // Apply rotY first, then rotX
	 * @endcode
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
	 *
	 * @param rhs The scalar value to multiply by
	 * @return A new quaternion with all components scaled
	 *
	 * @note - This operation does not represent a valid rotation.
	 * @note - Scaling a unit quaternion will make it non-unit,
	 * requiring normalization for proper rotation behavior.
	 *
	 * @code
	 * Quat q = Quat::EulerDegrees(45, 0, 0);
	 * Quat scaled = q * 0.5f; // Scale all components by 0.5
	 * @endcode
	 */
	Quat Quat::operator*(const float rhs) const
	{
	    return {w * rhs, x * rhs, y * rhs, z * rhs};
	}

	/**
	 * @brief Component-wise addition operator.
	 *
	 * Adds corresponding components of two quaternions. Used primarily
	 * in mathematical computations and interpolation algorithms.
	 *
	 * @param rhs The quaternion to add
	 * @return A new quaternion with component-wise sum
	 *
	 * @note - This is not the same as combining rotations.
	 * @note - Adding quaternions typically produces non-unit quaternions.
	 * @note - This operation does not represent a valid rotation.
	 *
	 * @code
	 * Quat q1(1, 0, 0, 0);
	 * Quat q2(0, 1, 0, 0);
	 * Quat sum = q1 + q2;
	 * @endcode
	 */
	Quat Quat::operator+(const Quat& rhs) const
	{
	    return {w + rhs.w, x + rhs.x, y + rhs.y, z + rhs.z};
	}

	/**
	 * @brief Component-wise subtraction operator.
	 *
	 * Subtracts corresponding components of two quaternions. Used primarily
	 * in mathematical computations and for calculating differences.
	 *
	 * @param rhs The quaternion to subtract
	 * @return A new quaternion with component-wise difference
	 *
	 * @note - This is not the same as inverse rotation composition.
	 * @note - Subtracting quaternions typically produces non-unit quaternions.
	 * @note - This operation does not represent a valid rotation.
	 *
	 * @code
	 * Quat q1(1, 1, 0, 0);
	 * Quat q2(0, 1, 0, 0);
	 * Quat diff = q1 - q2; // Result: (1, 0, 0, 0)
	 * @endcode
	 */
	Quat Quat::operator-(const Quat& rhs) const
	{
	    return {w - rhs.w, x - rhs.x, y - rhs.y, z - rhs.z};
	}

	/// Unary negation: negate all components (q and -q represent the same rotation)
	/**
	 * @brief Unary negation operator.
	 *
	 * Returns the additive inverse of this quaternion by negating all components.
	 *
	 * This operation is useful for ensuring the shortest path during interpolation
	 * (e.g., SLERP). Note that the negated quaternion represents the same rotation
	 * as the original quaternion, but with opposite sign.
	 *
	 * @return A new quaternion with all components negated
	 *
	 * @note - This is not the same as conjugation or inverse; it simply negates the components.
	 * @note - The negated quaternion is often used in interpolation to ensure the shortest path.
	 *
	 * @code
	 * Quat q(1, 2, 3, 4);
	 * Quat negated = -q; // Result: (-1, -2, -3, -4)
	 * @endcode
	 */
	Quat Quat::operator-() const { return { -w, -x, -y, -z }; }

	/**
	 * @brief Transforms a Vec4 by this quaternion's rotation.
	 *
	 * Converts the quaternion to a matrix and multiplies the Vec4 by it.
	 * This applies the rotation represented by the quaternion to the vector.
	 *
	 * @param rhs The Vec4 to transform
	 * @return A new Vec4 containing the rotated vector
	 *
	 * @note - The Vec4 is treated as a point in 3D space (w=1) for transformation.
	 * This is the most common operation for rotating 3D vectors.
	 *
	 * @code
	 * Quat rotation = Quat::EulerDegrees(90, 0, 0);
	 * Vec4 vector(1, 0, 0, 1);
	 * Vec4 rotated = rotation * vector; // Rotates the vector by 90° around X-axis
	 * @endcode
	 */
	Vec4 Quat::operator*(const Vec4& rhs) const { return ToMatrix(*this) * rhs; }

	/**
	 * @brief Transforms a Vec3 by this quaternion's rotation.
	 *
	 * Converts the quaternion to a matrix and multiplies the Vec3 (extended
	 * to homogeneous coordinates with w=1) by it to apply the rotation.
	 *
	 * @param rhs The Vec3 to transform
	 * @return A new Vec3 containing the rotated vector
	 *
	 * @note - This is the most common operation for rotating 3D vectors.
	 * The Vec3 is treated as a point in 3D space (w=1) for transformation.
	 * This allows the quaternion to be used as a rotation matrix.
	 *
	 * @code
	 * Quat rotation = Quat::EulerDegrees(0, 90, 0); // 90° around Y-axis
	 * Vec3 vector(1, 0, 0);
	 * Vec3 rotated = rotation * vector; // Rotates the vector by 90° around Y-axis
	 * @endcode
	 */
	Vec3 Quat::operator*(const Vec3& rhs) const { return ToMatrix(*this) * Vec4(rhs, 1.0f); }

	/**
	 * @brief Returns a normalized copy of this quaternion.
	 *
	 * Creates a new quaternion with the same rotation as this quaternion
	 * but with unit length (magnitude = 1). Essential for proper rotation behavior.
	 *
	 * @return A new normalized quaternion representing the same rotation
	 *
	 * @note - Normalization is important after operations that might change the quaternion's magnitude,
	 * such as multiplication or addition.
	 *
	 * @note - If the quaternion has zero magnitude, the result is undefined.
	 * This should be called after operations that might change the quaternion's magnitude.
	 *
	 * @code
	 * Quat q(2, 4, 6, 8); // Non-unit quaternion
	 * Quat normalized = q.GetNormalized(); // Unit quaternion with same rotation
	 * @endcode
	 */
    Quat Quat::GetNormalized() const
    {
        const float mag2 = w * w + x * x + y * y + z * z;
        if (mag2 <= 0.0f)
            return Quat(); // identity fallback for degenerate quaternion
        const float mag = sqrtf(mag2);
        if (mag <= 0.0f)
            return Quat();
        return {w / mag, x / mag, y / mag, z / mag};
    }

    /**
     * @brief GLM-style alias for GetNormalized().
     *
     * Returns a normalized copy of this quaternion.
     *
     * @return A new normalized quaternion representing the same rotation
     *
     * @note - This is an alias for GetNormalized() and does not modify the original quaternion.
     * @note - If the quaternion has zero magnitude, the result is undefined.
     *
     * @code
     * Quat q(2, 4, 6, 8); // Non-unit quaternion
     * Quat normalized = q.normalize(); // Unit quaternion with same rotation
     * @endcode
     */
    Quat Quat::normalize() const
	{
		return GetNormalized();
	}

	/**
	 * @brief Normalizes this quaternion in-place.
	 *
	 * Modifies this quaternion to have unit length while preserving
	 * the rotation it represents. This should be called after operations
	 * that might change the quaternion's magnitude.
	 *
	 * @note - This method modifies the quaternion in place.
	 * @note - If the quaternion has zero magnitude, the result is undefined.
	 *
	 * @code
	 * Quat q(2, 4, 6, 8); // Non-unit quaternion
	 * q.Normalize(); // q is now a unit quaternion
	 * @endcode
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
	 *
	 * @note - For unit quaternions: conjugate == inverse
	 * @note - Conjugate of conjugate equals the original quaternion
	 *
	 * @code
	 * Quat rotation = Quat::EulerDegrees(45, 0, 0);
	 * Quat opposite = rotation.Conjugate(); // -45° rotation around X-axis
	 * @endcode
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
	 *
	 * @note - The rotation order is XYZ (Tait-Bryan angles).
	 * @note - Angles are converted to radians internally.
	 * @note - Euler angles can suffer from gimbal lock at certain orientations.
	 * @note - This modifies the current quaternion to represent the specified rotation.
	 *
	 * @code
	 * Quat rotation;
	 * rotation.SetEulerDegrees(90, 45, 30);
	 * @endcode
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
	 * @note - Used internally by Slerp and angle calculation functions.
	 * @note - The result is always in the range [-1.0, 1.0].
	 * @note - Both quaternions should be normalized for accurate results.
	 *
	 * @code
	 * Quat q1 = Quat::EulerDegrees(45, 0, 0);
	 * Quat q2 = Quat::EulerDegrees(90, 0, 0);
	 * float similarity = q1.Dot(q2); // Dot product value
	 * @endcode
	 */
	float Quat::Dot(const Quat& b) const { return (w * b.w + x * b.x + y * b.y + z * b.z); }


    /**
     * @brief Converts this quaternion to a rotation matrix.
     *
     * Converts the quaternion to a 4x4 rotation matrix that can be used
     * for transforming vectors in 3D space. The resulting matrix represents
     * the same rotation as the quaternion.
     *
     * @return A Mat4 representing the same rotation as this quaternion
     *
     * @note - The quaternion should be normalized for accurate conversion.
     * @note - The resulting matrix has translation components set to zero.
     * @note - This is equivalent to calling ToMatrix() on the quaternion instance.
     *
     * @code
     * Quat rotation = Quat::EulerDegrees(45, 0, 0);
     * Mat4 rotMatrix = rotation.ToMatrix();
     * @endcode
     */
    Mat4 Quat::ToMatrix() const { return ToMatrix(*this); }

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
	 *
	 * @note - Both input vectors are normalized internally to ensure correct behavior.
	 * @note - Returns identity quaternion if vectors are already aligned.
	 * @note - Handles special cases for parallel and antiparallel vectors.
	 *
	 * @code
	 * Vec3 from(1, 0, 0);
	 * Vec3 to(0, 1, 0);
	 * Quat rotation = Quat::FromToRotation(from, to);
	 * @endcode
	 */
	Quat Quat::FromToRotation(const Vec3& from, const Vec3& to)
	{
		const Vec3 unitFrom = SceneryEditorX::Normalize(from);
		const Vec3 unitTo = SceneryEditorX::Normalize(to);
		const float dot = SceneryEditorX::Dot(unitFrom, unitTo);

		if (dot >= 1.0f)
			return {}; /// identity

		if (dot <= -1.0f)
		{
			/// 180-degree rotation around any axis orthogonal to from
			Vec3 axis = SceneryEditorX::Cross(unitFrom, Vec3(1, 0, 0));
			if (SceneryEditorX::Dot(axis, axis) < 1e-6f)
			{
				axis = SceneryEditorX::Cross(unitFrom, Vec3(0, 1, 0));
			}

			const Vec3 normAxis = SceneryEditorX::Normalize(axis);
			return Quat::AngleAxis(180.0f, Vec4(normAxis.x, normAxis.y, normAxis.z, 0.0f));
		}

		/// General case
		const float s_from = SceneryEditorX::Dot(unitFrom, unitFrom);
		const float s_to = SceneryEditorX::Dot(unitTo, unitTo);
		const float s = sqrtf(s_from * s_to) + SceneryEditorX::Dot(unitFrom, unitTo);
        Vec3 v = Cross(unitFrom, unitTo);
        // @note: Previous implementation negated x & y only (v.x*= -1, v.y*=-1) which
        // introduced an implicit handedness flip for some rotations. That was unintentional
        // and produced asymmetric results. We now keep the raw cross product.
        return Quat(Vec4(v, s)).GetNormalized();
    }

    /**
     * @brief Creates a quaternion that looks in the specified direction.
     *
     * Generates a rotation quaternion that orients an object to look in the
     * given direction. Assumes the object's forward direction is
     * initially along the positive Z-axis (0, 0, 1).
     *
     * @param lookAt The direction to look towards (will be normalized)
     * @return A quaternion representing the look rotation
     *
     * @note - This is equivalent to FromToRotation(Vec3(0,0,1), lookAt)
     * @note - The lookAt vector is normalized internally to ensure correct behavior.
     * @note - This function does not handle up direction constraints.
     *
     * @code
     * Vec3 target(1, 0, 0); // Look towards positive X
     * Quat lookRotation = Quat::LookRotation(target);
     * @endcode
     */
    Quat Quat::LookRotation(const Vec3& lookAt)
	{
	    return FromToRotation(Vec3(0, 0, 1), lookAt);
	}

	///< TODO: Modify LookRotation entries after modifying the original function

    /**
     * @brief Creates a quaternion that looks in the specified direction with an up constraint.
     *
     * Generates a rotation quaternion that orients an object to look in the
     * specified direction while attempting to keep the specified up direction
     * aligned with the object's up vector.
     *
     * This is useful for camera orientations where you want to maintain a
     * consistent up direction while looking at a target.
     *
     * @param lookAt The direction to look towards (will be normalized)
     * @param upDirection The desired up direction (will be normalized)
     * @return A quaternion representing the constrained look rotation
     *
     * @note - This function may have issues with certain edge cases
     * (e.g., collinear lookAt and upDirection vectors).
     * @note - If the lookAt and upDirection vectors are collinear, the function
     * @note - This function is deprecated and may not handle all edge cases correctly.
     *
     * @code
     * Vec3 target(1, 0, 0); // Look towards positive X
     * Vec3 up(0, 1, 0); // Keep +Y as up
     * Quat lookRotation = Quat::LookRotation(target, up);
     * @endcode
     */
    [[deprecated]] Quat Quat::LookRotation(const Vec3& lookAt, const Vec3& upDirection)
	{
		///< Calculate the unit quaternion that rotates Vector3::FORWARD to face in the specified forward direction.
		const Quat q1 = LookRotation(lookAt);

		
		/// We can't preserve the upwards direction if the forward and upwards
		/// vectors are linearly dependent (collinear).
	    if (SceneryEditorX::Dot(SceneryEditorX::Cross(lookAt, upDirection), SceneryEditorX::Cross(lookAt, upDirection)) == 0.0f)
            return q1;

        /// Determine the upwards direction obtained after applying q1.
	    const Vec3 newUp = q1 * Vec3(0, 1, 0);


		 /// Calculate the unit quaternion rotation that rotates the newUp
		 /// direction to look in the specified upward direction.
		const Quat q2 = FromToRotation(newUp, upDirection);

		/// Return the combined rotation so that we first rotate to look in the
		/// forward direction and then rotate to align Vector3::UPWARD with the
		/// specified upward direction. There is no need to normalize the result
		/// as both q1 and q2 are unit quaternions.
		return q2 * q1;
	}

    /**
     * @brief Creates a quaternion that looks in the specified direction with an up constraint (improved).
     *
     * Generates a rotation quaternion that orients an object with the specified
     * forward and up directions. This is the recommended method for creating
     * look-at rotations with proper up vector constraints.
     *
     * @param forward The forward direction vector (will be normalized)
     * @param up The up direction vector (will be normalized)
     * @return A quaternion representing the orientation
     *
     * @note - This is the improved version of LookRotation with better edge case handling
     * @note - Input vectors should not be parallel for best results
     *
     * @code
     * Vec3 forward(1, 0, 0);  // Look towards +X
     * Vec3 up(0, 1, 0);       // Keep +Y as up
     * Quat rotation = Quat::LookRotation2(forward, up);
     * @endcode
     */
    Quat Quat::LookRotation2(const Vec3& forward, const Vec3& up)
	{
	    Vec3 right = SceneryEditorX::Cross(up, forward);
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
	 *
	 * @note - SLERP provides constant angular velocity unlike linear interpolation
	 * @note - t values outside [0,1] will extrapolate beyond the endpoints
	 * @note - Automatically handles quaternion double-cover (chooses shortest path)
	 * @note - This function is suitable for animations and smooth transitions
	 *
	 * @code
	 * Quat start = Quat::EulerDegrees(0, 0, 0);
	 * Quat end = Quat::EulerDegrees(90, 0, 0);
	 * Quat middle = Quat::Slerp(start, end, 0.5f); // 45° rotation
	 * @endcode
	 */
    Quat Quat::Slerp(const Quat &from, const Quat &to, const float t)
    {
        // Robust SLERP with small-angle fallback to avoid precision loss.
        Quat a = from.GetNormalized();
        Quat b = to.GetNormalized();
        float dot = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
        if (dot < 0.0f)
        {
            dot = -dot;
            b.w = -b.w;
            b.x = -b.x;
            b.y = -b.y;
            b.z = -b.z; // shortest path
        }
        dot = std::clamp(dot, -1.0f, 1.0f);
        // Small angle -> nlerp
        if (1.0f - dot < 1e-3f)
        {
            Quat result(a.w + t * (b.w - a.w), a.x + t * (b.x - a.x), a.y + t * (b.y - a.y), a.z + t * (b.z - a.z));
            return result.GetNormalized();
        }
        float theta0 = acosf(dot);
        float theta = theta0 * t;
        float sinTheta0 = sinf(theta0);
        float sinTheta = sinf(theta);
        float s0 = cosf(theta) - dot * sinTheta / sinTheta0;
        float s1 = sinTheta / sinTheta0;
        return Quat(s0 * a.w + s1 * b.w, s0 * a.x + s1 * b.x, s0 * a.y + s1 * b.y, s0 * a.z + s1 * b.z);
    }


    /**
     * @brief Linearly interpolates between two quaternions.
     *
     * Performs linear interpolation (LERP) between two quaternions.
     * This method is less smooth than SLERP and can result in non-uniform
     * angular velocity, but is simpler and faster.
     *
     * @param from The starting quaternion
     * @param to The ending quaternion
     * @param t Interpolation parameter (0.0 = from, 1.0 = to)
     * @return A quaternion interpolated between from and to
     *
     * @note - LERP is not suitable for animations requiring constant angular velocity
     * @note - t values outside [0,1] will extrapolate beyond the endpoints
     * @note - This method does not handle quaternion double-cover, so it may not
     *        produce the shortest path between rotations
     *
     * @code
     * Quat start = Quat::EulerDegrees(0, 0, 0);
     * Quat end = Quat::EulerDegrees(90, 0, 0);
     * Quat middle = Quat::Lerp(start, end, 0.5f); // 45° rotation
     * @endcode
     */
    Quat Quat::Lerp(const Quat& from, const Quat& to, const float t)
	{
		const Quat src = from * (1.0f - t);
		const Quat dst = to * t;

		Quat q = src + dst;
		return q.GetNormalized();
	}

    /**
     * @brief Calculates the angle between two quaternions.
     *
     * Computes the angle in degrees between two quaternions
     * using the dot product.
     *
     * @param a The first quaternion
     * @param b The second quaternion
     * @return The angle in degrees between the two quaternions
     *
     * @note - The angle is always in the range [0, 180]
     * @note - This function uses the dot product to determine the angle
     * @note - If the quaternions are identical, the angle is 0
     * @note - If the quaternions are opposite, the angle is 180
     *
     * @code
     * Quat q1 = Quat::EulerDegrees(0, 0, 0);
     * Quat q2 = Quat::EulerDegrees(90, 0, 0);
     * float angle = Quat::Angle(q1, q2); // 90 degrees
     * @endcode
     */
    float Quat::Angle(const Quat& a, const Quat& b)
	{
		const float degrees = acosf((b * a.GetInverse()).w) * 2.0f * 57.2957795f;
		if (degrees > 180.0f)
			return 360.0f - degrees;
		return degrees;
	}

    /**
     * @brief Calculates the dot product of two quaternions.
     *
     * Computes the dot product between two quaternions.
     * The dot product is a measure of similarity between two rotations.
     *
     * @param a The first quaternion
     * @param b The second quaternion
     * @return The dot product value (1.0 = identical, -1.0 = opposite)
     *
     * @note - The dot product is always in the range [-1.0, 1.0]
     * @note - If the quaternions are identical, the dot product is 1.0
     * @note - If the quaternions are opposite, the dot product is -1.0
     * @note - This function is used in SLERP and angle calculation functions
     *
     * @code
     * Quat q1 = Quat::EulerDegrees(0, 0, 0);
     * Quat q2 = Quat::EulerDegrees(90, 0, 0);
     * float dot = Quat::Dot(q1, q2);
     * @endcode
     */
    float Quat::Dot(const Quat& a, const Quat& b)
	{
	    return a.Dot(b);
	}

    /**
     * @brief Creates a quaternion from an angle and axis.
     *
     * Creates a quaternion representing a rotation around a specified axis
     * by a given angle in degrees. The axis is expected to be a normalized vector.
     *
     * @param angle The rotation angle in degrees
     * @param axis The axis of rotation (should be normalized)
     * @return A quaternion representing the rotation
     *
     * @note - The axis vector is normalized internally to ensure correct behavior
     * @note - The angle is converted from degrees to radians internally
     * @note - This function uses the angle-axis representation of quaternions
     * @note - The resulting quaternion is normalized to ensure unit length
     *
     * @code
     * Quat rotation = Quat::AngleAxis(90, Vec4(0, 1, 0, 0)); // 90° rotation around Y-axis
     * @endcode
     */
    Quat Quat::AngleAxis(float angle, const Vec4& axis)
	{
		/// Normalize axis (x, y, z only)
		const float len = sqrtf(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
		Vec4 vn = axis;
		if (len > 0.0f)
		{
			const float inv = 1.0f / len;
			vn.x *= inv; vn.y *= inv; vn.z *= inv;
		}

		angle *= 0.0174532925f; /// degrees to radians
		angle *= 0.5f;
		const float sinAngle = sinf(angle);

		return {cosf(angle), vn.x * sinAngle, vn.y * sinAngle, vn.z * sinAngle};
	}

    /**
     * @brief Creates a quaternion from an angle and axis in radians.
     *
     * @details Creates a quaternion representing a rotation around a specified axis
     * by a given angle in radians. The axis is expected to be a normalized vector.
     *
     * @param angleRadians The rotation angle in radians
     * @param axis The axis of rotation (should be normalized)
     * @return A quaternion representing the rotation
     *
     * @note - The axis vector is normalized internally to ensure correct behavior
     * @note - The angle is expected to be in radians
     * @note - This function uses the angle-axis representation of quaternions
     * @note - The resulting quaternion is normalized to ensure unit length
     *
     * @code
     * Quat rotation = Quat::AngleAxisRadians(1.5708f, Vec3(0, 1, 0)); // 90° rotation around Y-axis
     * @endcode
     */
    Quat Quat::AngleAxisRadians(float angleRadians, const Vec3& axis)
	{
		/// Normalize axis
		float len2 = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
		Vec3 n(0.0f);
		if (len2 > 0.0f)
		{
			float inv = 1.0f / sqrtf(len2);
			n = Vec3(axis.x * inv, axis.y * inv, axis.z * inv);
		}
		const float half = angleRadians * 0.5f;
		const float s = sinf(half);
		return {cosf(half), n.x * s, n.y * s, n.z * s};
	}

    /**
     * @brief Calculates the inverse of this quaternion.
     *
     * Computes the inverse of the quaternion, which is used to reverse the rotation
     * represented by the quaternion. The inverse is defined as:
     * - For a quaternion (w, x, y, z), the inverse is (w, -x, -y, -z) / |q|^2
     * This function assumes the quaternion is normalized (unit length).
     *
     * @return A new quaternion that is the inverse of this quaternion
     *
     * @note - The inverse is used in SLERP and angle calculations to reverse rotations
     * @note - If the quaternion is not normalized, the result will not be a unit quaternion
     * @note - The inverse of a quaternion is also known as the conjugate for unit quaternions
     * @note - The inverse of the quaternion is used to reverse the rotation it represents
     *
     * @code
     * Quat rotation = Quat::EulerDegrees(45, 0, 0);
     * Quat inverse = rotation.GetInverse(); // Inverse rotation of 45° around X-axis
     * @endcode
     */
    Quat Quat::GetInverse() const
	{
		const float sqr = GetSqrMagnitude();
		const float invSqr = 1.0f / sqr;

		return {w * invSqr, -x * invSqr, -y * invSqr, -z * invSqr};
	}

    /**
     * @brief Returns the inverse of this quaternion.
     *
     * Calculates the inverse of the quaternion, which is used to reverse the rotation
     * represented by the quaternion. The inverse is defined as:
     * - For a quaternion (w, x, y, z), the inverse is (w, -x, -y, -z) / |q|^2
     * This function assumes the quaternion is normalized (unit length).
     *
     * @return A new quaternion that is the inverse of this quaternion
     *
     * @note - The inverse is used in SLERP and angle calculations to reverse rotations
     * @note - If the quaternion is not normalized, the result will not be a unit quaternion
     * @note - The inverse of a quaternion is also known as the conjugate for unit quaternions
     * @note - The inverse of the quaternion is used to reverse the rotation it represents
     *
     * @code
     * Quat rotation = Quat::EulerDegrees(45, 0, 0);
     * Quat inverse = rotation.inverse(); // Inverse rotation of 45° around X-axis
     * @endcode
     */
    Quat Quat::inverse() const
	{
		return GetInverse();
	}

    /**
     * @brief Creates a quaternion from Euler angles in degrees.
     *
     * Converts Euler angles (pitch, yaw, roll) in degrees to a quaternion.
     *
     * @param X Rotation around X-axis in degrees (pitch)
     * @param Y Rotation around Y-axis in degrees (yaw)
     * @param Z Rotation around Z-axis in degrees (roll)
     * @return A quaternion representing the rotation
     *
     * @note - The angles are converted from degrees to radians internally
     * @note - The rotation order is XYZ (Tait-Bryan angles)
     * @note - Euler angles can suffer from gimbal lock at certain orientations
     * @note - This function is equivalent to calling EulerRadians() with angles in radians
     *
     * @code
     * Quat rotation = Quat::EulerDegrees(90, 0, 0); // 90° rotation around X-axis
     * @endcode
     */
    Quat Quat::EulerDegrees(float X, float Y, float Z)
	{
		X *= 0.0174532925f; ///< To radians!
		Y *= 0.0174532925f; ///< To radians!
		Z *= 0.0174532925f; ///< To radians!

		return EulerRadians(X, Y, Z);
	}

    /**
     * @brief Creates a quaternion from Euler angles in radians.
     *
     * Converts Euler angles (pitch, yaw, roll) in radians to a quaternion.
     *
     * @param X Rotation around X-axis in radians (pitch)
     * @param Y Rotation around Y-axis in radians (yaw)
     * @param Z Rotation around Z-axis in radians (roll)
     * @return A quaternion representing the rotation
     *
     * @note - The rotation order is XYZ (Tait-Bryan angles)
     * @note - Euler angles can suffer from gimbal lock at certain orientations
     * @note - This function uses trigonometric functions to compute the quaternion components
     * @note - The angles are expected to be in radians
     *
     * @code
     * Quat rotation = Quat::EulerRadians(1.5708f, 0, 0); // 90° rotation around X-axis
     * @endcode
     */
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
	 *
	 * @note - The quaternion should ideally be normalized before conversion
	 * @note - The resulting matrix has translation components set to zero
	 * @note - This function is used in rendering pipelines to apply rotations
	 *
	 * @code
	 * Quat rotation = Quat::EulerDegrees(45, 0, 0);
	 * Mat4 rotMatrix = Quat::ToMatrix(rotation); // Convert to rotation matrix
	 * @endcode
	 */
	Mat4 Quat::ToMatrix(const Quat& q)
	{
		ZoneScoped;

		const float sqw = q.w * q.w;
		const float sqx = q.x * q.x;
		const float sqy = q.y * q.y;
		const float sqz = q.z * q.z;
		const float invs = 1.0f / (sqx + sqy + sqz + sqw);

		Mat4 matrix = Mat4::Identity();

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

    /**
     * @brief Calculates the angle in radians between a quaternion and the identity quaternion.
     *
     * Calculates the angle in radians between the given quaternion and the identity quaternion.
     *
     * @param x The quaternion to calculate the angle for
     * @return The angle in radians between the quaternion and the identity quaternion
     *
     * @note - The angle is always in the range [0, π]
     * @note - This function uses the arccosine of the w component to determine the angle
     *
     * @code
     * Quat rotation = Quat::EulerDegrees(90, 0, 0);
     * float angle = Quat::AngleRadians(rotation);
     * @endcode
     */
    float Quat::AngleRadians(const Quat &x)
    {
        /// Clamp to avoid NaNs from slight numerical errors. The acosf domain is
        /// [-1,1], but accumulated FP drift (or a slightly non‑normalized
        /// quaternion) can push w marginally outside. We clamp then scale.
        float cw = std::max(-1.0f, std::min(1.0f, x.w));
        return 2.0f * acosf(cw);
    }

    /**
     * @brief Calculates the axis of rotation for a quaternion.
     *
     * Calculates the axis of rotation represented by the quaternion.
     *
     * @param x The quaternion to calculate the axis for
     * @return The axis of rotation as a Vec3
     *
     * @note - The axis is normalized to unit length
     * @note - If the quaternion represents no rotation, returns (0, 0, 1)
     * @note - The axis is derived from the x, y, z components of the quaternion
     *
     * @code
     * Quat rotation = Quat::EulerDegrees(90, 0, 0);
     * Vec3 axis = Quat::Axis(rotation); // Axis of rotation
     * @endcode
     */
    Vec3 Quat::Axis(const Quat &x)
    {
        // tmp1 = sin^2(theta/2); if very small the axis direction is numerically unstable.
        float tmp1 = 1.0f - x.w * x.w;
        if (tmp1 <= 1e-12f)
            return Vec3(0.0f, 0.0f, 1.0f); // default axis when angle ~0
        float inv = 1.0f / sqrtf(tmp1);
        return Vec3(x.x * inv, x.y * inv, x.z * inv);
    }

    /**
     * @brief Rotates a quaternion by a given angle around a specified axis.
     *
     * Rotates the quaternion by a specified angle in radians around a given axis.
     *
     * @param q The quaternion to rotate
     * @param angleRadians The rotation angle in radians
     * @param axis The axis of rotation (should be normalized)
     * @return A new quaternion representing the rotated quaternion
     *
     * @note - The axis vector is expected to be normalized
     * @note - The angle is expected to be in radians
     * @note - This function uses the angle-axis representation of quaternions
     *
     * @code
     * Quat rotation = Quat::EulerDegrees(0, 0, 0);
     * Quat rotated = Quat::Rotate(rotation, 1.5708f, Vec3(0, 1, 0)); // Rotate 90° around Y-axis
     * @endcode
     */
    Quat Quat::Rotate(const Quat& q, float angleRadians, const Vec3& axis)
	{
		return q * AngleAxisRadians(angleRadians, axis);
	}

    /**
     * @brief Converts this quaternion to Euler angles in radians.
     *
     * Converts the quaternion to Euler angles (pitch, yaw, roll) in radians.
     *
     * @return A Vec3 representing the Euler angles in radians
     *
     * @note - The angles are in the order (pitch, yaw, roll)
     * @note - The conversion handles gimbal lock and singularities
     *
     * @code
     * Quat rotation = Quat::EulerDegrees(90, 0, 0);
     * Vec3 euler = rotation.ToEulerRadians(); // Convert to Euler angles in radians
     * @endcode
     */
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

    /**
     * @brief Converts this quaternion to Euler angles in degrees.
     *
     * Converts the quaternion to Euler angles (pitch, yaw, roll) in degrees.
     *
     * @return A Vec3 representing the Euler angles in degrees
     *
     * @note - The angles are in the order (pitch, yaw, roll)
     * @note - The conversion handles gimbal lock and singularities
     * @note - The angles are converted from radians to degrees internally
     *
     * @code
     * Quat rotation = Quat::EulerDegrees(90, 0, 0);
     * Vec3 euler = rotation.ToEulerDegrees(); // Convert to Euler angles in degrees
     * @endcode
     */
	Vec3 Quat::ToEulerDegrees() const
	{
	    return ToEulerRadians() * RAD_TO_DEG;
	}

    /**
     * @brief Multiplies a vector by a quaternion.
     *
     * Multiplies a vector by a quaternion, effectively applying the rotation
     * represented by the quaternion to the vector.
     *
     * @param v The vector to rotate
     * @param m The quaternion representing the rotation
     * @return A Vec4 representing the rotated vector
     *
     * @note - The vector is treated as a Vec4 with w = 1.0 for rotation
     * @note - The quaternion is converted to a rotation matrix internally
     * @note - This function is used to apply rotations to vectors in 3D space
     *
     * @code
     * Vec4 vec(1, 0, 0, 1);
     * Quat rotation = Quat::EulerDegrees(90, 0, 0);
     * Vec4 rotatedVec = vec * rotation; // Rotate the vector by 90° around X-axis
     * @endcode
     */
    Vec4 operator*(const Vec4& v, const Quat& m)
	{
	    return Quat::ToMatrix(m) * v;
	}

    /**
     * @brief Multiplies a vector by a quaternion.
     *
     * Multiplies a vector by a quaternion, effectively applying the rotation
     * represented by the quaternion to the vector.
     *
     * @param v The vector to rotate
     * @param m The quaternion representing the rotation
     * @return A Vec3 representing the rotated vector
     *
     * @note - The vector is treated as a Vec4 with w = 1.0 for rotation
     * @note - The quaternion is converted to a rotation matrix internally
     * @note - This function is used to apply rotations to vectors in 3D space
     *
     * @code
     * Vec3 vec(1, 0, 0);
     * Quat rotation = Quat::EulerDegrees(90, 0, 0);
     * Vec3 rotatedVec = vec * rotation; // Rotate the vector by 90° around X-axis
     * @endcode
     */
    Vec3 operator*(const Vec3& v, const Quat& m)
	{
	    return Quat::ToMatrix(m) * Vec4(v, 1.0f);
	}

    /**
     * @brief Multiplies a quaternion by a scalar.
     *
     * Multiplies a quaternion by a scalar value, effectively scaling
     * the quaternion's components. This is useful for scaling rotations
     * without changing the direction of the rotation.
     *
     * @param f The scalar value to multiply by
     * @param m The quaternion to scale
     * @return A new quaternion with scaled components
     *
     * @note - The resulting quaternion is not normalized after scaling
     * @note - This function is useful for adjusting rotation speeds or magnitudes
     * @note - The scalar multiplication does not change the direction of the rotation
     *
     * @code
     * Quat rotation = Quat::EulerDegrees(90, 0, 0);
     * Quat scaledRotation = 0.5f * rotation; // Scale the rotation by 0.5
     * @endcode
     */
    Quat operator*(const float f, const Quat& m)
	{
	    return {m.w * f, m.x * f, m.y * f, m.z * f};
	}

	/// -----------------------------------------------------

}

/// -----------------------------------------------------

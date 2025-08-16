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
#pragma once
#include <SceneryEditorX/utils/math/matrix.h>
#include <SceneryEditorX/utils/math/vector.h>

/// -----------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @brief A quaternion class for representing rotations in 3D space.
     *
     * Quaternions provide a compact and mathematically robust representation of rotations
     * that avoids gimbal lock and provides smooth interpolation. A quaternion is composed
     * of four components: x, y, z (vector part) and w (scalar part).
     *
     * The quaternion is stored as (x, y, z, w) where:
     * - (x, y, z) represents the rotation axis scaled by sin(θ/2)
     * - w represents cos(θ/2) where θ is the rotation angle
     *
     * This class provides comprehensive support for quaternion mathematics including:
     * - Construction from Euler angles, axis-angle, and vectors
     * - Conversion to/from matrices and Euler angles
     * - Interpolation methods (SLERP, LERP)
     * - Rotation composition and mathematical operations
     *
     * @note - Quaternions are expected to be normalized for proper rotation behavior.
     * @note - This implementation follows the convention where w is the scalar component.
     */
    class Quat
    {
    public:

        /// Convenience creators
        static Quat Identity() { return Quat(); }

        /**
         * @brief Default constructor creating an identity quaternion.
         *
         * Creates a quaternion representing no rotation (identity rotation).
         * The resulting quaternion has components (0, 0, 0, 1) which represents
         * a rotation of 0 degrees around any axis.
         *
         * @code
         * Quat identity; // Creates identity quaternion (0, 0, 0, 1)
         * @endcode
         */
		Quat();

        /**
         * @brief Constructs a quaternion from individual components.
         *
         * Creates a quaternion with the specified scalar and vector components.
         * The quaternion represents a rotation where the vector part (x, y, z)
         * encodes the rotation axis scaled by sin(θ/2) and the scalar part (w)
         * encodes cos(θ/2).
         *
         * @param w The scalar component (cos(θ/2))
         * @param x The x component of the vector part (axis.x * sin(θ/2))
         * @param y The y component of the vector part (axis.y * sin(θ/2))
         * @param z The z component of the vector part (axis.z * sin(θ/2))
         *
         * @note - For valid rotations, the quaternion should be normalized.
         *
         * @code
         * Quat q(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
         * Quat rotation(0.707f, 0.707f, 0.0f, 0.0f); // 90° rotation around X-axis
         * @endcode
         */
		Quat(float w, float x, float y, float z);

        /**
         * @brief Constructs a quaternion from a Vec4.
         *
         * Creates a quaternion by copying components from a Vec4 where:
         * - vec4.x becomes the quaternion's x component
         * - vec4.y becomes the quaternion's y component
         * - vec4.z becomes the quaternion's z component
         * - vec4.w becomes the quaternion's w component
         *
         * @param vec4 The Vec4 containing quaternion components (x, y, z, w)
         *
         * @code
         * Vec4 components(0.0f, 0.0f, 0.0f, 1.0f);
         * Quat q(components); // Creates quaternion from Vec4
         * @endcode
         */
        explicit Quat(const Vec4& vec4);

        /**
         * @brief Copy constructor.
         *
         * Creates a new quaternion by copying all components from another quaternion.
         *
         * @param q The quaternion to copy from
         *
         * @code
         * Quat original = Quat::EulerDegrees(45, 0, 0);
         * Quat copy(original); // Creates a copy of the original quaternion
         * @endcode
         */
		Quat(const Quat& q);

        /**
         * @brief Destructor.
         *
         * Cleans up the quaternion instance. Since quaternions only contain
         * primitive float values, no special cleanup is required.
         */
		~Quat();

        /**
         * @brief Copy assignment operator.
         *
         * Assigns the components of another quaternion to this quaternion.
         * Includes self-assignment protection.
         *
         * @param q The quaternion to copy from
         * @return Reference to this quaternion after assignment
         *
         * @code
         * Quat q1, q2 = Quat::EulerDegrees(90, 0, 0);
         * q1 = q2; // q1 now has the same rotation as q2
         * @endcode
         */
		Quat& operator=(const Quat& q);

        /**
         * @brief Equality comparison operator.
         *
         * Compares two quaternions for exact component-wise equality.
         * Returns true if all four components (x, y, z, w) are exactly equal.
         *
         * @param rhs The quaternion to compare against
         * @return true if all components are equal, false otherwise
         *
         * @note - This performs exact floating-point comparison, which may not be
         *       suitable for quaternions that have undergone floating-point arithmetic.
         *       Consider using an epsilon-based comparison for such cases.
         *
         * @code
         * Quat q1 = Quat::EulerDegrees(45, 0, 0);
         * Quat q2 = Quat::EulerDegrees(45, 0, 0);
         * bool same = (q1 == q2); // true if exactly equal
         * @endcode
         */
		bool operator==(const Quat& rhs) const;

        /**
         * @brief Inequality comparison operator.
         *
         * Compares two quaternions for inequality. Returns true if any of the
         * four components (x, y, z, w) are not exactly equal.
         *
         * @param rhs The quaternion to compare against
         * @return true if any components are not equal, false if all are equal
         *
         * @note - This is the logical negation of the equality operator.
         *
         * @code
         * Quat q1 = Quat::EulerDegrees(45, 0, 0);
         * Quat q2 = Quat::EulerDegrees(90, 0, 0);
         * bool different = (q1 != q2); // true, different rotations
         * @endcode
         */
		bool operator!=(const Quat& rhs) const;

        /**
         * @brief Quaternion multiplication assignment operator.
         *
         * Multiplies this quaternion by another quaternion and stores the result
         * in this quaternion. Quaternion multiplication represents the composition
         * of rotations, where the right-hand side rotation is applied first,
         * followed by the left-hand side rotation.
         *
         * @param rhs The quaternion to multiply by
         * @return Reference to this quaternion after multiplication
         *
         * @note - Quaternion multiplication is not commutative: A *= B ≠ B *= A
         * @note - The result represents applying rotation B first, then rotation A
         *
         * @code
         * Quat rotX = Quat::EulerDegrees(90, 0, 0);  // 90° around X
         * Quat rotY = Quat::EulerDegrees(0, 45, 0);  // 45° around Y
         * rotX *= rotY; // rotX now represents Y rotation followed by X rotation
         * @endcode
         */
		Quat& operator*=(const Quat& rhs);

        /**
         * @brief Quaternion multiplication operator.
         *
         * Multiplies two quaternions to compose their rotations. The resulting
         * quaternion represents applying the right-hand side rotation first,
         * followed by the left-hand side rotation.
         *
         * @param rhs The quaternion to multiply by
         * @return A new quaternion representing the composed rotation
         *
         * @note - Quaternion multiplication is not commutative: A * B ≠ B * A
         * @note - The mathematical formula used is Hamilton's quaternion product
         *
         * @code
         * Quat rotX = Quat::EulerDegrees(90, 0, 0);
         * Quat rotY = Quat::EulerDegrees(0, 45, 0);
         * Quat combined = rotX * rotY; // Apply rotY first, then rotX
         * @endcode
         */
		Quat operator*(const Quat& rhs) const;

        /**
         * @brief Scalar multiplication operator.
         *
         * Multiplies all components of the quaternion by a scalar value.
         * This operation is primarily used for mathematical computations and
         * interpolation, not for rotating by a scalar amount.
         *
         * @param rhs The scalar value to multiply by
         * @return A new quaternion with all components scaled
         *
         * @warning Scaling a unit quaternion will make it non-unit, requiring
         *          normalization for proper rotation behavior.
         *
         * @code
         * Quat q = Quat::EulerDegrees(45, 0, 0);
         * Quat scaled = q * 0.5f; // Scale all components by 0.5
         * @endcode
         */
		Quat operator*(float rhs) const;

        /**
         * @brief Quaternion addition operator.
         *
         * Performs component-wise addition of two quaternions. This operation
         * is primarily used in mathematical computations and interpolation
         * algorithms, not for combining rotations.
         *
         * @param rhs The quaternion to add
         * @return A new quaternion with component-wise sum
         *
         * @note - For combining rotations, use multiplication (*) instead
         * @warning Adding quaternions typically produces non-unit quaternions
         *
         * @code
         * Quat q1(1, 0, 0, 0);
         * Quat q2(0, 1, 0, 0);
         * Quat sum = q1 + q2; // Result: (1, 1, 0, 0)
         * @endcode
         */
		Quat operator+(const Quat& rhs) const;

        /**
         * @brief Quaternion subtraction operator.
         *
         * Performs component-wise subtraction of two quaternions. This operation
         * is primarily used in mathematical computations and for calculating
         * quaternion differences in interpolation algorithms.
         *
         * @param rhs The quaternion to subtract
         * @return A new quaternion with component-wise difference
         *
         * @note - This is not the same as inverse rotation composition
         * @warning Subtracting quaternions typically produces non-unit quaternions
         *
         * @code
         * Quat q1(1, 1, 0, 0);
         * Quat q2(0, 1, 0, 0);
         * Quat diff = q1 - q2; // Result: (1, 0, 0, 0)
         * @endcode
         */
		Quat operator-(const Quat& rhs) const;

    	/**
    	 * @brief Unary negation operator.
    	 *
    	 * Returns the additive inverse of this quaternion by negating all components.
    	 * Note that q and -q represent the same rotation, but this is useful when
    	 * ensuring the shortest path during interpolation (e.g., SLERP).
    	 */
    	Quat operator-() const;

        /**
         * @brief Transforms a Vec4 by this quaternion's rotation.
         *
         * Applies the rotation represented by this quaternion to a 4D vector.
         * The transformation is performed by converting the quaternion to a
         * rotation matrix and multiplying the vector by that matrix.
         *
         * @param rhs The Vec4 to transform
         * @return A new Vec4 containing the rotated vector
         *
         * @note - The w component of the Vec4 is preserved during rotation
         * @note - This operation treats the quaternion as a rotation matrix
         *
         * @code
         * Quat rotation = Quat::EulerDegrees(90, 0, 0);
         * Vec4 vector(1, 0, 0, 1);
         * Vec4 rotated = rotation * vector;
         * @endcode
         */
		Vec4 operator*(const Vec4& rhs) const;

        /**
         * @brief Transforms a Vec3 by this quaternion's rotation.
         *
         * Applies the rotation represented by this quaternion to a 3D vector.
         * The transformation is performed by converting the quaternion to a
         * rotation matrix and multiplying the vector (extended to homogeneous
         * coordinates with w=1) by that matrix.
         *
         * @param rhs The Vec3 to transform
         * @return A new Vec3 containing the rotated vector
         *
         * @note - This is the most common operation for rotating 3D vectors
         * @note - The vector is treated as a point (w=1) for transformation
         *
         * @code
         * Quat rotation = Quat::EulerDegrees(0, 90, 0); // 90° around Y-axis
         * Vec3 forward(0, 0, 1);
         * Vec3 rotated = rotation * forward; // Should be (-1, 0, 0)
         * @endcode
         */
		Vec3 operator*(const Vec3& rhs) const;

        /**
         * @brief Returns a normalized copy of this quaternion.
         *
         * Creates a new quaternion that represents the same rotation as this
         * quaternion but with unit length (magnitude = 1). Normalized quaternions
         * are required for proper rotation behavior.
         *
         * @return A new normalized quaternion
         *
         * @note - This method does not modify the original quaternion
         * @note - If the quaternion has zero magnitude, the result is undefined
         *
         * @code
         * Quat q(2, 4, 6, 8); // Non-unit quaternion
         * Quat normalized = q.GetNormalized(); // Unit quaternion with same rotation
         * @endcode
         */
        [[nodiscard]] Quat GetNormalized() const;

        /// GLM-style aliases that return new quaternions (do not modify this)
        [[nodiscard]] Quat normalize() const;   /// alias for GetNormalized()

        /**
         * @brief Normalizes this quaternion in-place.
         *
         * Modifies this quaternion to have unit length (magnitude = 1) while
         * preserving the rotation it represents. This is essential for proper
         * rotation behavior and should be called after operations that might
         * change the quaternion's magnitude.
         *
         * @note - This method modifies the quaternion in place
         * @note - If the quaternion has zero magnitude, the result is undefined
         *
         * @code
         * Quat q(2, 4, 6, 8); // Non-unit quaternion
         * q.Normalize(); // q is now a unit quaternion
         * @endcode
         */
		void Normalize();

        /**
         * @brief Returns the conjugate of this quaternion.
         *
         * The conjugate of a quaternion (x, y, z, w) is (-x, -y, -z, w).
         * For unit quaternions, the conjugate is equivalent to the inverse
         * and represents the opposite rotation.
         *
         * @return A new quaternion that is the conjugate of this quaternion
         *
         * @note - For unit quaternions: conjugate == inverse
         * @note - Conjugate of conjugate equals the original quaternion
         *
         * @code
         * Quat rotation = Quat::EulerDegrees(45, 0, 0);
         * Quat opposite = rotation.Conjugate(); // -45° rotation around X-axis
         * @endcode
         */
		Quat Conjugate(); /// Same as inverse

    	/// Magnitude helpers
        [[nodiscard]] float Length() const { return std::sqrt(GetSqrMagnitude()); }

        /**
         * @brief Sets this quaternion from Euler angles in degrees.
         *
         * Constructs a rotation quaternion from Euler angles specified in degrees.
         * The rotation order is typically XYZ (Tait-Bryan angles). This method
         * modifies the current quaternion to represent the specified rotation.
         *
         * @param X Rotation around the X-axis in degrees
         * @param Y Rotation around the Y-axis in degrees
         * @param Z Rotation around the Z-axis in degrees
         *
         * @note - The rotation order affects the final result
         * @note - Angles are converted to radians internally
         *
         * @code
         * Quat q;
         * q.SetEulerDegrees(90, 45, 30); // 90° X, 45° Y, 30° Z rotation
         * @endcode
         */
		void SetEulerDegrees(float X, float Y, float Z);

        /**
         * @brief Calculates the dot product with another quaternion.
         *
         * Computes the dot product between this quaternion and another quaternion.
         * The dot product can be used to measure the similarity between two
         * rotations and is used in interpolation algorithms.
         *
         * @param b The quaternion to compute dot product with
         * @return The dot product as a float value
         *
         * @note - Dot product of 1.0 indicates identical rotations
         * @note - Dot product of -1.0 indicates opposite rotations
         * @note - Used internally by Slerp and angle calculation functions
         *
         * @code
         * Quat q1 = Quat::EulerDegrees(45, 0, 0);
         * Quat q2 = Quat::EulerDegrees(90, 0, 0);
         * float similarity = q1.Dot(q2);
         * @endcode
         */
        [[nodiscard]] float Dot(const Quat& b) const;

        /**
         * @brief Converts this quaternion to a 4x4 rotation matrix.
         *
         * Creates a 4x4 transformation matrix that represents the same rotation
         * as this quaternion. The resulting matrix can be used for transforming
         * vertices, combining with other transformation matrices, or interfacing
         * with graphics APIs that expect matrix representations.
         *
         * @return A Matrix4x4 representing the same rotation
         *
         * @note - The quaternion should be normalized for accurate conversion
         * @note - The resulting matrix has translation components set to zero
         *
         * @code
         * Quat rotation = Quat::EulerDegrees(45, 0, 0);
         * Matrix4x4 rotMatrix = rotation.ToMatrix();
         * @endcode
         */
        [[nodiscard]] Mat4 ToMatrix() const;

        /**
         * @brief Creates a quaternion representing rotation from one vector to another.
         *
         * Calculates the shortest rotation that would align the 'from' vector
         * with the 'to' vector. This is useful for orienting objects to face
         * specific directions or aligning coordinate systems.
         *
         * @param from The source vector (will be normalized internally)
         * @param to The target vector (will be normalized internally)
         * @return A quaternion representing the rotation from 'from' to 'to'
         *
         * @note - Handles special cases like parallel and antiparallel vectors
         * @note - Both input vectors are normalized internally
         * @note - Returns identity quaternion if vectors are already aligned
         *
         * @code
         * Vec3 forward(0, 0, 1);
         * Vec3 target(1, 0, 0);
         * Quat rotation = Quat::FromToRotation(forward, target);
         * @endcode
         */
		static Quat FromToRotation(const Vec3& from, const Vec3& to);

        /**
         * @brief Creates a quaternion that looks in the specified direction.
         *
         * Generates a rotation quaternion that would orient an object to look
         * in the given direction. Assumes the object's forward direction is
         * initially along the positive Z-axis.
         *
         * @param lookAt The direction to look towards (will be normalized)
         * @return A quaternion representing the look rotation
         *
         * @note - This is equivalent to FromToRotation(Vec3(0,0,1), lookAt)
         * @note - The up vector is not constrained, potentially causing roll
         *
         * @code
         * Vec3 target(1, 0, 0); // Look towards positive X
         * Quat lookRotation = Quat::LookRotation(target);
         * @endcode
         */
		static Quat LookRotation(const Vec3& lookAt);

        /**
         * @brief Creates a quaternion that looks in a direction with up constraint.
         *
         * @deprecated Use LookRotation2 instead for better behavior.
         *
         * Generates a rotation quaternion that orients an object to look in the
         * specified direction while attempting to keep the specified up direction
         * aligned with the object's up vector.
         *
         * @param lookAt The direction to look towards
         * @param upDirection The desired up direction
         * @return A quaternion representing the constrained look rotation
         *
         * @note - This function may have issues with certain edge cases
         * @note - Consider using LookRotation2 for more robust behavior
         */
		static Quat LookRotation(const Vec3& lookAt, const Vec3& upDirection);


        /**
         * @brief Creates a quaternion from forward and up directions (improved).
         *
         * Generates a rotation quaternion that orients an object with the specified
         * forward and up directions. This is the recommended method for creating
         * look-at rotations with proper up vector constraints.
         *
         * @deprecated New LookRotation function. Do not use old one.
         * @param forward The forward direction vector
         * @param up The up direction vector
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
		static Quat LookRotation2(const Vec3& forward, const Vec3& up);

        /**
         * @brief Spherical linear interpolation between two quaternions.
         *
         * Performs spherical linear interpolation (SLERP) between two quaternions,
         * providing smooth rotation interpolation that maintains constant angular
         * velocity. This is the preferred method for animating rotations.
         *
         * @param from The starting quaternion
         * @param to The ending quaternion
         * @param t Interpolation parameter (0.0 = from, 1.0 = to)
         * @return A quaternion interpolated between from and to
         *
         * @note - SLERP provides constant angular velocity unlike linear interpolation
         * @note - t values outside [0,1] will extrapolate beyond the endpoints
         * @note - Automatically handles quaternion double-cover (chooses shortest path)
         *
         * @code
         * Quat start = Quat::EulerDegrees(0, 0, 0);
         * Quat end = Quat::EulerDegrees(90, 0, 0);
         * Quat middle = Quat::Slerp(start, end, 0.5f); // 45° rotation
         * @endcode
         */
		static Quat Slerp(const Quat& from, const Quat& to, float t);

        /**
         * @brief Linear interpolation between two quaternions.
         *
         * Performs linear interpolation between two quaternions by interpolating
         * each component separately. The result is normalized to maintain unit
         * length. LERP is faster than SLERP but doesn't provide constant angular
         * velocity.
         *
         * @param from The starting quaternion
         * @param to The ending quaternion
         * @param t Interpolation parameter (0.0 = from, 1.0 = to)
         * @return A quaternion linearly interpolated between from and to
         *
         * @note - LERP is faster than SLERP but provides non-uniform angular velocity
         * @note - The result should be normalized for proper rotation behavior
         * @note - Use SLERP for high-quality rotation animation
         *
         * @code
         * Quat start = Quat::EulerDegrees(0, 0, 0);
         * Quat end = Quat::EulerDegrees(90, 0, 0);
         * Quat lerped = Quat::Lerp(start, end, 0.5f);
         * @endcode
         */
		static Quat Lerp(const Quat& from, const Quat& to, float t);

        /**
         * @brief Calculates the angle between two quaternions.
         *
         * Computes the angular difference between two quaternions in degrees.
         * This represents the minimum rotation angle needed to rotate from
         * one orientation to the other.
         *
         * @param a The first quaternion
         * @param b The second quaternion
         * @return The angle between the quaternions in degrees [0, 180]
         *
         * @note - The result is always in the range [0, 180] degrees
         * @note - Both quaternions should be normalized for accurate results
         *
         * @code
         * Quat q1 = Quat::EulerDegrees(0, 0, 0);
         * Quat q2 = Quat::EulerDegrees(45, 0, 0);
         * float angle = Quat::Angle(q1, q2); // Should be 45.0
         * @endcode
         */
		static float Angle(const Quat& a, const Quat& b);

        /**
         * @brief Calculates the dot product between two quaternions.
         *
         * Computes the dot product of two quaternions, which is useful for
         * measuring the similarity between rotations and in interpolation
         * algorithms. This is a static version of the instance Dot method.
         *
         * @param a The first quaternion
         * @param b The second quaternion
         * @return The dot product as a float value
         *
         * @note - Dot product of 1.0 indicates identical rotations
         * @note - Dot product of -1.0 indicates opposite rotations
         * @note - Used internally by Slerp and Angle functions
         *
         * @code
         * Quat q1 = Quat::EulerDegrees(45, 0, 0);
         * Quat q2 = Quat::EulerDegrees(90, 0, 0);
         * float similarity = Quat::Dot(q1, q2);
         * @endcode
         */
		static float Dot(const Quat& a, const Quat& b);

        /**
         * @brief Creates a quaternion from an angle and axis of rotation.
         *
         * Constructs a quaternion representing a rotation of the specified angle
         * around the given axis. This is one of the most intuitive ways to create
         * rotation quaternions when you know the exact axis and angle.
         *
         * @param angle The rotation angle in degrees
         * @param axis The axis of rotation as a Vec4 (will be normalized)
         * @return A quaternion representing the specified rotation
         *
         * @note - The angle is specified in degrees and converted to radians internally
         * @note - The axis vector is normalized automatically
         * @note - Only the x, y, z components of the Vec4 axis are used
         *
         * @code
         * Vec4 xAxis(1, 0, 0, 0);
         * Quat rotation = Quat::AngleAxis(90.0f, xAxis); // 90° around X-axis
         * @endcode
         */
		static Quat AngleAxis(float angle, const Vec4& axis);

        /**
         * @brief Creates a quaternion from an angle and axis of rotation in radians.
         *
         * Constructs a quaternion representing a rotation of the specified angle
         * around the given axis, with the angle specified in radians. This method
         * provides direct access to radian-based angle inputs without degree-to-radian
         * conversion.
         *
         * @param angleRadians The rotation angle in radians
         * @param axis The axis of rotation as a Vec3 (will be normalized)
         * @return A quaternion representing the specified rotation
         *
         * @note - The axis vector is normalized automatically
         *
         * @code
         * Vec3 axis(1, 0, 0);
         * Quat rotation = Quat::AngleAxisRadians(M_PI_2, axis); // 90° around X-axis
         * @endcode
         */
        static Quat AngleAxisRadians(float angleRadians, const Vec3& axis);

        /**
         * @brief Returns the inverse of this quaternion.
         *
         * Calculates the inverse (reciprocal) of this quaternion. For unit
         * quaternions, the inverse is the same as the conjugate and represents
         * the opposite rotation.
         *
         * @return The inverse quaternion
         *
         * @note - For unit quaternions: inverse == conjugate
         * @note - Multiplying a quaternion by its inverse yields identity
         * @note - The original quaternion is not modified
         *
         * @code
         * Quat rotation = Quat::EulerDegrees(45, 0, 0);
         * Quat inverse = rotation.GetInverse(); // -45° rotation
         * Quat identity = rotation * inverse; // Should be identity
         * @endcode
         */
        [[nodiscard]] Quat GetInverse() const;

        /// GLM-style alias returning a new quaternion inverse
        [[nodiscard]] Quat inverse() const;     /// alias for GetInverse()

        /**
         * @brief Creates a quaternion from Euler angles in degrees.
         *
         * Constructs a rotation quaternion from Euler angles specified in degrees
         * using the XYZ rotation order (Tait-Bryan angles). This is one of the
         * most common ways to create rotations from user-friendly angle inputs.
         *
         * @param X Rotation around the X-axis in degrees (pitch)
         * @param Y Rotation around the Y-axis in degrees (yaw)
         * @param Z Rotation around the Z-axis in degrees (roll)
         * @return A quaternion representing the combined rotation
         *
         * @note - Angles are applied in XYZ order: X first, then Y, then Z
         * @note - Input angles are converted to radians internally
         * @note - Euler angles can suffer from gimbal lock at certain orientations
         *
         * @code
         * Quat rotation = Quat::EulerDegrees(90, 45, 30);
         * // 90° pitch, 45° yaw, 30° roll
         * @endcode
         */
		static Quat EulerDegrees(float X, float Y, float Z);

        /**
         * @brief Creates a quaternion from Euler angles in radians.
         *
         * Constructs a rotation quaternion from Euler angles specified in radians
         * using the XYZ rotation order. This method provides direct access to
         * radian-based angle inputs without degree-to-radian conversion.
         *
         * @param X Rotation around the X-axis in radians
         * @param Y Rotation around the Y-axis in radians
         * @param Z Rotation around the Z-axis in radians
         * @return A quaternion representing the combined rotation
         *
         * @note - More precise than degree-based version for mathematical calculations
         * @note - No internal unit conversion is performed
         *
         * @code
         * Quat rotation = Quat::EulerRadians(M_PI_2, M_PI_4, M_PI_6);
         * // 90°, 45°, 30° in radians
         * @endcode
         */
		static Quat EulerRadians(float X, float Y, float Z);

        /**
         * @brief Static version of matrix conversion.
         *
         * Converts the specified quaternion to a 4x4 rotation matrix. This static
         * method provides the same functionality as the instance ToMatrix() method
         * but can be called without a quaternion instance.
         *
         * @param q The quaternion to convert
         * @return A Matrix4x4 representing the same rotation
         *
         * @note - The quaternion should be normalized for accurate conversion
         * @note - Equivalent to calling q.ToMatrix()
         *
         * @code
         * Quat rotation = Quat::EulerDegrees(45, 0, 0);
         * Matrix4x4 matrix = Quat::ToMatrix(rotation);
         * @endcode
         */
		static Mat4 ToMatrix(const Quat& q);

		/// Common helpers mirroring GLM free functions
		static Quat Normalize(const Quat& q) { return q.GetNormalized(); }
		static Quat Inverse(const Quat& q) { return q.GetInverse(); }

        /**
         * @brief Calculates the angle of rotation in radians.
         *
         * Computes the angle of rotation represented by the quaternion. This
         * is useful for understanding how much rotation the quaternion represents
         * without needing to convert to Euler angles.
         *
         * @param x The quaternion to calculate the angle from
         * @return The angle of rotation in radians
         *
         * @note - The angle is calculated using the formula: angle = 2 * acos(w)
         *
         * @code
         * Quat rotation = Quat::EulerDegrees(90, 0, 0);
         * float angle = Quat::AngleRadians(rotation); // Should be 90° in radians
         * @endcode
         */
        static float AngleRadians(const Quat& x);

        /**
         * @brief Calculates the axis of rotation from a quaternion.
         *
         * Extracts the axis of rotation represented by the quaternion.
         *
         * @param x The quaternion to extract the axis from 
         * @return The axis of rotation as a Vec3
         *
         * @note - The axis is normalized internally
         *
         * @code
         * Quat rotation = Quat::EulerDegrees(90, 0, 0);
         * Vec3 axis = Quat::Axis(rotation); // Should be (1, 0, 0) for 90° around X
         * @endcode
         */
        static Vec3  Axis(const Quat& x);

		/**
		 * @brief Rotates a quaternion by a specified angle around an axis.
		 *
		 * Creates a new quaternion that represents the rotation of the
		 * given quaternion by the specified angle around the provided axis.
		 *
		 * @param q The quaternion to rotate
		 * @param angleRadians The rotation angle in radians
		 * @param axis The axis of rotation as a Vec3 (will be normalized)
		 * @return A new quaternion representing the rotated orientation
		 *
		 * @note - The axis vector is normalized internally
		 * @note - This is useful for applying additional rotations to an existing quaternion
		 *
		 * @code
		 * Quat original = Quat::EulerDegrees(45, 0, 0);
		 * Vec3 rotationAxis(0, 1, 0); // Rotate around Y-axis
		 * Quat rotated = Quat::Rotate(original, M_PI_4, rotationAxis); // Rotate by 45° around Y
		 * @endcode
		 */
		static Quat Rotate(const Quat& q, float angleRadians, const Vec3& axis);

        /**
         * @brief Converts this quaternion to Euler angles in radians.
         *
         * Extracts the Euler angles (in radians) that would produce the same
         * rotation as this quaternion. The angles follow the XYZ rotation order.
         * This is useful for interfacing with systems that expect Euler angles.
         *
         * @return A Vec3 containing (X, Y, Z) Euler angles in radians
         *
         * @note - The quaternion should be normalized for accurate conversion
         * @note - May suffer from gimbal lock representation issues
         * @note - The result uses XYZ rotation order
         *
         * @code
         * Quat rotation = Quat::EulerDegrees(90, 45, 30);
         * Vec3 angles = rotation.ToEulerRadians();
         * @endcode
         */
        [[nodiscard]] Vec3 ToEulerRadians() const;

        /**
         * @brief Converts this quaternion to Euler angles in degrees.
         *
         * Extracts the Euler angles (in degrees) that would produce the same
         * rotation as this quaternion. This provides user-friendly angle values
         * by converting the radian result to degrees.
         *
         * @return A Vec3 containing (X, Y, Z) Euler angles in degrees
         *
         * @note - Convenience method that calls ToEulerRadians() and converts to degrees
         * @note - The quaternion should be normalized for accurate conversion
         *
         * @code
         * Quat rotation = Quat::EulerRadians(M_PI_2, 0, 0);
         * Vec3 angles = rotation.ToEulerDegrees(); // Should be (90, 0, 0)
         * @endcode
         */
        [[nodiscard]] Vec3 ToEulerDegrees() const;

        /**
         * @brief Gets the squared magnitude of this quaternion.
         *
         * Calculates the squared magnitude (length squared) of the quaternion.
         * For normalized quaternions, this should be 1.0. This method is faster
         * than calculating the actual magnitude since it avoids the square root.
         *
         * @return The squared magnitude as a float
         *
         * @note - For unit quaternions, this should equal 1.0
         * @note - Useful for checking if normalization is needed
         * @note - Faster than calculating actual magnitude
         *
         * @code
         * Quat q(1, 2, 3, 4);
         * float sqrMag = q.GetSqrMagnitude(); // 1 + 4 + 9 + 16 = 30
         * @endcode
         */
        [[nodiscard]] float GetSqrMagnitude() const { return x * x + y * y + z * z + w * w; }

        /**
         * @brief Convenience method for creating quaternion from Vec3 Euler degrees.
         *
         * Creates a quaternion from Euler angles stored in a Vec3, where each
         * component represents rotation around the corresponding axis in degrees.
         *
         * @param euler A Vec3 containing (X, Y, Z) Euler angles in degrees
         * @return A quaternion representing the combined rotation
         *
         * @note - Equivalent to EulerDegrees(euler.x, euler.y, euler.z)
         * @note - Convenient for passing Vec3 angle representations
         *
         * @code
         * Vec3 angles(90, 45, 30);
         * Quat rotation = Quat::EulerDegrees(angles);
         * @endcode
         */
        static Quat EulerDegrees(Vec3 euler) { return EulerDegrees(euler.x, euler.y, euler.z); }

        /**
         * @brief Convenience method for creating quaternion from Vec3 Euler radians.
         *
         * Creates a quaternion from Euler angles stored in a Vec3, where each
         * component represents rotation around the corresponding axis in radians.
         *
         * @param euler A Vec3 containing (X, Y, Z) Euler angles in radians
         * @return A quaternion representing the combined rotation
         *
         * @note - Equivalent to EulerRadians(euler.x, euler.y, euler.z)
         * @note - Convenient for passing Vec3 angle representations
         *
         * @code
         * Vec3 angles(M_PI_2, M_PI_4, M_PI_6);
         * Quat rotation = Quat::EulerRadians(angles);
         * @endcode
         */
        static Quat EulerRadians(Vec3 euler) { return EulerRadians(euler.x, euler.y, euler.z); }

        /**
         * @brief Union providing multiple ways to access quaternion components.
         *
         * This union allows accessing the quaternion components either as named
         * fields (x, y, z, w) or as an array (xyzw[4]). This provides flexibility
         * for different use cases and interfacing with APIs that expect arrays.
         *
         * Component Layout:
         * - x, y, z: The vector part of the quaternion (rotation axis * sin(θ/2))
         * - w: The scalar part of the quaternion (cos(θ/2))
         * - xyzw[]: Array access to all four components in order [x, y, z, w]
         *
         * @note - The components can be accessed either by name or by array index
         * @note - Array access follows the order: [0]=x, [1]=y, [2]=z, [3]=w
         *
         * @code
         * Quat q(1, 2, 3, 4);
         * float x_val = q.x;        // Named access
         * float y_val = q.xyzw[1];  // Array access
         * @endcode
         */
        union
        {
            struct { float x, y, z, w; };
            float xyzw[4];
        };
    };


    /// Inline helper for readability when composing transforms with quaternions.
    /// Instead of writing q.ToMatrix() callers can use ToMat(q) mirroring other
    /// free-function helpers in the math API.
    inline Mat4 ToMat(const Quat& q) { return q.ToMatrix(); }

    // Ensure Mat4 * Quat and Quat * Mat4 interoperability helpers exist (may have been added earlier).
    inline Mat4 operator*(const Mat4& lhs, const Quat& rhs) { return lhs * rhs.ToMatrix(); }
    inline Mat4 operator*(const Quat& lhs, const Mat4& rhs) { return lhs.ToMatrix() * rhs; }


    /**
     * @brief Transforms a Vec4 by a quaternion's rotation (right-hand operator).
     *
     * Applies the rotation represented by the quaternion to a 4D vector.
     * This operator allows the syntax `vector * quaternion` in addition to
     * the standard `quaternion * vector`.
     *
     * @param v The Vec4 to transform
     * @param m The quaternion representing the rotation
     * @return A new Vec4 containing the rotated vector
     *
     * @note - This provides the same result as `m * v` but with reversed operand order
     * @note - Useful for chaining operations or when the syntax is more natural
     *
     * @code
     * Vec4 vector(1, 0, 0, 1);
     * Quat rotation = Quat::EulerDegrees(90, 0, 0);
     * Vec4 rotated = vector * rotation; // Alternative to rotation * vector
     * @endcode
     */
	Vec4 operator*(const Vec4& v, const Quat& m);

    /**
     * @brief Transforms a Vec3 by a quaternion's rotation (right-hand operator).
     *
     * Applies the rotation represented by the quaternion to a 3D vector.
     * This operator allows the syntax `vector * quaternion` in addition to
     * the standard `quaternion * vector`.
     *
     * @param v The Vec3 to transform
     * @param m The quaternion representing the rotation
     * @return A new Vec3 containing the rotated vector
     *
     * @note - This provides the same result as `m * v` but with reversed operand order
     * @note - The vector is treated as a point (w=1) for transformation
     *
     * @code
     * Vec3 forward(0, 0, 1);
     * Quat rotation = Quat::EulerDegrees(0, 90, 0);
     * Vec3 rotated = forward * rotation; // Alternative syntax
     * @endcode
     */
	Vec3 operator*(const Vec3& v, const Quat& m);

    /**
     * @brief Scalar multiplication with quaternion (left-hand operator).
     *
     * Multiplies all components of a quaternion by a scalar value. This operator
     * allows the syntax `scalar * quaternion` in addition to `quaternion * scalar`.
     *
     * @param f The scalar value to multiply by
     * @param m The quaternion to scale
     * @return A new quaternion with all components scaled
     *
     * @note - This provides the same result as `m * f` but with reversed operand order
     * @warning Scaling a unit quaternion will make it non-unit
     *
     * @code
     * Quat q = Quat::EulerDegrees(45, 0, 0);
     * Quat scaled = 0.5f * q; // Alternative to q * 0.5f
     * @endcode
     */
	Quat operator*(float f, const Quat& m);

}

/// -----------------------------------------------------


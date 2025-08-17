/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* projection.h
* -------------------------------------------------------
* Created: 13/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <cmath>
#include <Math/math_config.h>
#include <Math/includes/constants.h>
#include <Math/includes/matrix.h>
#include <Math/includes/vector.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    /**
     * @brief Creates a perspective projection matrix with field of view in radians.
     *
     * This function generates a perspective projection matrix that transforms 3D coordinates
     * from view space (camera space) to clip space. The matrix implements a standard
     * perspective projection with a symmetric frustum defined by field of view and aspect ratio.
     *
     *
     * @param fovyRadians The vertical field of view angle in radians.
     * @param width The width of the viewport or frustum.
     * @param height The height of the viewport or frustum.
     * @param zNear The distance to the near clipping plane. Must be positive.
     * @param zFar The distance to the far clipping plane. Must be positive and greater than zNear.
     * @return Mat4 A 4x4 perspective projection matrix suitable for use in graphics pipelines.
     *
     * @note - The Y-axis is inverted (negative scale) to match common graphics coordinate systems.
     * @note - Objects closer than the near plane or farther than the far plane will be clipped.
     *
     * @warning Ensure zNear and zFar are positive and zNear < zFar to avoid degenerate matrices.
     * @warning If height is zero, aspect ratio defaults to 1.0 to avoid division by zero.
     * @code
     * float fovy = Utils::ToRadians(60.0f); // 60 degrees in radians
     * Mat4 projection = SceneryEditorX::PerspectiveFov(fovy, 800.0f, 600.0f, 0.1f, 1000.0f);
     * @endcode
     */
    inline Mat4 PerspectiveFov(float fovyRadians, float width, float height, float zNear, float zFar)
    {
        const float aspect = (height != 0.0f) ? (width / height) : 1.0f;	/// Calculate aspect ratio, defaulting to 1.0 if height is zero
        const float f = 1.0f / std::tan(fovyRadians * 0.5f);				/// Calculate the cotangent of half the field of view
        const float A = f / aspect;											/// Scale X based on aspect ratio
        const float B = f;													/// Scale Y
        const float C = (zFar + zNear) / (zNear - zFar);					/// Scale Z
        const float D = (2.0f * zFar * zNear) / (zNear - zFar);				/// Translate factor

        return Mat4({
            { A,    0.0f, 0.0f, 0.0f }, /// Scale X
            { 0.0f, B,    0.0f, 0.0f }, /// Scale Y
            { 0.0f, 0.0f, C,    D    }, /// Scale Z and translation
            {0.0f, 0.0f, -1.0f, 0.0f}   /// Homogeneous coordinate (no translation)
        });
    }

    /**
     * @brief Creates a perspective projection matrix with field of view in radians.
     *
     * This function generates a perspective projection matrix that transforms 3D coordinates
     * from view space (camera space) to clip space. The matrix implements a standard
     * perspective projection with a symmetric frustum defined by field of view and aspect ratio.
     *
     * @param fovyRadians The vertical field of view angle in radians.
     * @param aspect The aspect ratio of the viewport (width / height). Must be positive.
     * @param zNear The distance to the near clipping plane. Must be positive.
     * @param zFar The distance to the far clipping plane. Must be positive and greater than zNear.
     * @return Mat4 A 4x4 perspective projection matrix suitable for use in graphics pipelines.
     *
     * @note - The Y-axis is inverted (negative scale) to match common graphics coordinate systems.
     * @note - Objects closer than the near plane or farther than the far plane will be clipped.
     *
     * @warning Ensure zNear and zFar are positive and zNear < zFar to avoid degenerate matrices.
     * @warning If aspect is zero, it defaults to 1.0 to avoid division by zero.
     *
     * @code
     * float fovy = Utils::ToRadians(60.0f); // 60 degrees in radians
     * Mat4 projection = SceneryEditorX::Perspective(fovy, 1.777f, 0.1f, 1000.0f);
     * @endcode
     */
    inline Mat4 Perspective(float fovyRadians, float aspect, float zNear, float zFar)
    {
        const float f = 1.0f / std::tan(fovyRadians * 0.5f); /// Calculate the cotangent of half the field of view
        const float A = f / ((aspect == 0.0f) ? 1.0f : aspect); /// Scale X based on aspect ratio, defaulting to 1.0 if aspect is zero
        const float B = f;                                      /// Scale Y
        const float C = (zFar + zNear) / (zNear - zFar);        /// Scale Z
        const float D = (2.0f * zFar * zNear) / (zNear - zFar); /// Translate factor

        return Mat4({
            { A,    0.0f, 0.0f, 0.0f }, /// Scale X
            { 0.0f, B,    0.0f, 0.0f }, /// Scale Y
            { 0.0f, 0.0f, C,    D    }, /// Scale Z and translation
            { 0.0f, 0.0f, -1.0f,0.0f }  /// Homogeneous coordinate (no translation)
        });
    }

    /**
     * @brief Creates an orthographic projection matrix.
     *
     * This function generates an orthographic projection matrix that transforms 3D coordinates
     * from view space (camera space) to clip space. The matrix implements a standard
     * orthographic projection with a symmetric frustum defined by left, right, bottom, top, near, and far planes.
     *
     * @param left
     * @param right
     * @param bottom
     * @param top
     * @param zNear
     * @param zFar
     * @return Mat4 A 4x4 orthographic projection matrix suitable for 2D rendering or technical drawing.
     *
     * @note - The coordinate system is right-handed with Y pointing up.
     * @note - Unlike perspective projection, parallel lines remain parallel after transformation.
     *
     * @warning Ensure right > left, top > bottom, and zFar != zNear to avoid degenerate matrices.
     * @warning If any of the dimensions (left, right, bottom, top) are zero, the resulting matrix will be degenerate.
     *
     * @code
     * Mat4 ortho = SceneryEditorX::Ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
     * @endcode
     */
    inline Mat4 Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        const float rl = (right - left);					/// Right - Left
        const float tb = (top - bottom);					/// Top - Bottom
        const float fn = (zFar - zNear);					/// Far - Near

        const float sx = rl != 0.0f ? 2.0f / rl : 0.0f;		/// Scale X
        const float sy = tb != 0.0f ? 2.0f / tb : 0.0f;		/// Scale Y
        const float sz = fn != 0.0f ? -2.0f / fn : 0.0f;	/// Scale Z (inverted for right-handed coordinate system)

        const float tx = -(right + left) / (rl == 0.0f ? 1.0f : rl); /// Translate X
        const float ty = -(top + bottom) / (tb == 0.0f ? 1.0f : tb); /// Translate Y
        const float tz = -(zFar + zNear) / (fn == 0.0f ? 1.0f : fn); /// Translate Z (inverted for right-handed coordinate system)

        return Mat4({
            { sx,   0.0f, 0.0f, tx },	/// Scale X and translation
            { 0.0f, sy,   0.0f, ty },	/// Scale Y and translation
            { 0.0f, 0.0f, sz,   tz },	/// Scale Z and translation
            { 0.0f, 0.0f, 0.0f, 1.0f }	/// Homogeneous coordinate (no translation)
        });
    }

    /**
     * @brief Creates a look-at matrix for camera transformations.
     *
     * This function generates a view matrix that transforms world coordinates
     * into camera space, effectively positioning and orienting the camera in 3D space.
     *
     * @param eye
     * @param center
     * @param up
     * @return Mat4 A 4x4 look-at matrix that can be used to transform world coordinates to camera space.
     *
     * @note - The camera is positioned at 'eye', looking towards 'center', with 'up' defining the camera's up direction.
     * @note - The resulting matrix is right-handed, meaning the camera's forward direction is negative Z.
     *
     * @warning Ensure 'up' is not parallel to the forward direction (eye to center) to avoid singularities.
     * @code
     * Vec3 eye(0.0f, 0.0f, 5.0f);		// Camera position
     * Vec3 center(0.0f, 0.0f, 0.0f);	// Point the camera is looking at
     * Vec3 up(0.0f, 1.0f, 0.0f);		// Camera's up direction
     * Mat4 viewMatrix = SceneryEditorX::LookAt(eye, center, up);
     * @endcode
     */
    inline Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
    {
        const Vec3 f = Normalize({ center.x - eye.x, center.y - eye.y, center.z - eye.z });
        /// side = normalize(cross(f, up))
        const Vec3 side = Normalize(Cross(f, up));
        const Vec3 up2 = Cross(side, f);

        /// Row-major with column-vector multiply; translation in last column
        return Mat4({
            { side.x,  side.y,  side.z,  - (side.x * eye.x + side.y * eye.y + side.z * eye.z) }, /// Right vector
            { up2.x,   up2.y,   up2.z,   - (up2.x * eye.x  + up2.y * eye.y  + up2.z * eye.z)  }, /// Up vector
            { -f.x,    -f.y,    -f.z,      (f.x * eye.x    + f.y * eye.y    + f.z * eye.z)    }, /// Forward vector (negative for right-handed)
            { 0.0f,    0.0f,    0.0f,    1.0f }
        });
    }

}

/// -------------------------------------------------------

// -------------------------------------------------------
// GLM compatibility shims
// -------------------------------------------------------
namespace glm
{
    inline ::SceneryEditorX::Mat4 perspectiveFov(float fovyRadians, float width, float height, float zNear, float zFar)
    {
        return ::SceneryEditorX::PerspectiveFov(fovyRadians, width, height, zNear, zFar);
    }
    inline ::SceneryEditorX::Mat4 perspective(float fovyRadians, float aspect, float zNear, float zFar)
    {
        return ::SceneryEditorX::Perspective(fovyRadians, aspect, zNear, zFar);
    }
    inline ::SceneryEditorX::Mat4 ortho(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        return ::SceneryEditorX::Ortho(left, right, bottom, top, zNear, zFar);
    }
    inline ::SceneryEditorX::Mat4 lookAt(const ::SceneryEditorX::Vec3& eye, const ::SceneryEditorX::Vec3& center, const ::SceneryEditorX::Vec3& up)
    {
        return ::SceneryEditorX::LookAt(eye, center, up);
    }
}

/// -------------------------------------------------------

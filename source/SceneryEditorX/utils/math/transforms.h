/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* transforms.h
* -------------------------------------------------------
* Created: 13/7/2025
* -------------------------------------------------------
*/
#pragma once

/// -------------------------------------------------------------

namespace SceneryEditorX::Utils
{
	/**
	 * @enum CoordSpace
	 * @brief Defines the coordinate space for transformation operations.
	 *
	 * This enumeration represents different coordinate spaces that can be used
	 * when performing transformations on objects in the scene.
	 *
	 * @value WorldSpace Global coordinate space of the scene
	 * @value LocalSpace Coordinate space relative to the parent object
	 * @value ObjectSpace Coordinate space relative to the object itself
	 * @value CameraSpace Coordinate space relative to the camera's perspective
	 */
	enum class CoordSpace : uint8_t
	{
		WorldSpace,
	    LocalSpace,
	    ObjectSpace,
		CameraSpace
	};

    /// -------------------------------------------------------------

    // Decompose a transform matrix into translation, rotation, and scale components.
    bool DecomposeTransform(const Mat4 &transform, Vec3 &translation, Quat &rotation, Vec3 &scale);

    // Compose a transform matrix from translation, rotation, and scale components.
    Mat4 ComposeTransform(const Vec3 &translation, const Quat &rotation, const Vec3 &scale);

}

/// -------------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* aabb.h
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once

/// -----------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Axis-Aligned Bounding Box (AABB) for efficient collision detection and spatial queries
	 *
	 * An AABB represents a rectangular box aligned with the coordinate axes, defined by
	 * minimum and maximum corner points. This is the most efficient bounding volume for
	 * broad-phase collision detection and spatial partitioning algorithms.
	 *
	 * The AABB is defined such that all points (x,y,z) within the box satisfy:
	 * Min.x <= x <= Max.x, Min.y <= y <= Max.y, Min.z <= z <= Max.z
	 *
	 * @note - For a valid AABB, Min should be component-wise less than or equal to Max
	 * @warning Empty or invalid AABBs (where Min > Max in any component) may cause
	 *          undefined behavior in some operations
	 */
	class AABB
	{
	public:

        /**
         * @enum Side
         * @brief Enumeration for the six sides of the AABB
         *
         * Represents the six sides of the axis-aligned bounding box.
         *
         * Each side corresponds to one of the coordinate axes, allowing easy
         * identification of which side a point or another AABB is relative to.
         */
        enum Side : uint8_t
		{
            Left = 0, ///< Left side of the AABB
            Right,    ///< Right side of the AABB
            Top,      ///< Top side of the AABB
            Bottom,   ///< Bottom side of the AABB
            Front,    ///< Front side of the AABB
            Back      ///< Back side of the AABB
		};

		Vec3 Min; ///< Minimum corner point of the bounding box
		Vec3 Max; ///< Maximum corner point of the bounding box

		/**
		 * @brief Default constructor creating an empty AABB at origin
		 *
		 * Creates an AABB with both Min and Max set to (0,0,0), representing
		 * an empty bounding box with zero volume.
		 */
		AABB() : Min(0.0f), Max(0.0f) {}

		/**
		 * @brief Constructs an AABB from minimum and maximum corner points
		 *
		 * @param min The minimum corner point (should be component-wise <= max)
		 * @param max The maximum corner point (should be component-wise >= min)
		 *
		 * @note - No validation is performed to ensure min <= max. Users should
		 *       ensure proper ordering for correct behavior.
		 */
		AABB(const Vec3& min, const Vec3& max) : Min(min), Max(max) {}

		/**
		 * @brief Calculates the size (dimensions) of the bounding box
		 *
		 * Returns the extent of the AABB in each dimension as a vector.
		 * For a valid AABB, all components should be non-negative.
		 *
		 * @return Vec3 containing the width, height, and depth of the bounding box
		 *
		 * @note - If Min > Max in any component, the corresponding size component will be negative
		 */
        [[nodiscard]] Vec3 Size() const { return Max - Min; }

        /**
         * @brief Calculates the center point of the bounding box
         *
         * Returns the geometric center of the AABB, which is the midpoint between
         * the minimum and maximum corners.
         *
         * @return Vec3 representing the center point of the bounding box
         *
         * @note - The center is calculated as Min + (Max - Min) * 0.5, which is
         *       equivalent to (Min + Max) * 0.5 but more numerically stable
         */
        [[nodiscard]] Vec3 Center() const { return Min + Size() * 0.5f; }

		/// TODO: For easy usability for future use add ability to get corners.
		/*
		[[nodiscard]] Vec3 Corner() const { return ; }
		*/

		/// TODO: For easy usability for future use add ability to get the top, bottom, left, right, front, and back sides.
        /*
		[[nodiscard]] Vec3 Side() const { return ; }
		*/

	};

}

/// -----------------------------------------------------

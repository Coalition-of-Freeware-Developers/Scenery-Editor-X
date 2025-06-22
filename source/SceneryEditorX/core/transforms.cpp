/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* transforms.cpp
* -------------------------------------------------------
* Created: 30/3/2025
* -------------------------------------------------------
*/

/// -------------------------------------------------------

namespace SceneryEditorX
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
	
	/// -------------------------------------------------------

    /**
	 * @struct TransformComponent
	 * @brief Represents the spatial transformation properties of an object in the scene.
	 * 
	 * This structure encapsulates the position, scale, and rotation of an object,
	 * storing the fundamental transformation values needed to place and orient objects
	 * in 3D space. It provides functionality to generate a transformation matrix
	 * that can be used in rendering and spatial calculations.
	 */
	struct TransformComponent
	{
	    /** @brief The position of the object in 3D space */
	    Vec3 translation{};
	    
	    /** @brief The scale of the object along each axis, defaults to (1,1,1) */
	    Vec3 scale{1.f, 1.f, 1.f};
	    
	    /** @brief The rotation of the object in radians around each axis */
	    Vec3 rotation{};
	
	    /**
	     * @brief Generates a 4x4 transformation matrix from the component's values.
	     * 
	     * Computes a combined transformation matrix that represents the translation,
	     * rotation, and scaling defined in this component. The rotation follows the
	     * Euler angle convention with the order: Y (rotation.y), X (rotation.x),
	     * Z (rotation.z).
	     * 
	     * @return glm::mat4 The resulting 4x4 transformation matrix
	     */
	    glm::mat4 mat4()
	    {
	        const float c3 = glm::cos(rotation.z);
	        const float s3 = glm::sin(rotation.z);
	        /// -------------------------------------------------------
	        const float c2 = glm::cos(rotation.x);
	        const float s2 = glm::sin(rotation.x);
	        /// -------------------------------------------------------
	        const float c1 = glm::cos(rotation.y);
	        const float s1 = glm::sin(rotation.y);
	        /// -------------------------------------------------------
	        return glm::mat4{
	            {
	                scale.x * (c1 * c3 + s1 * s2 * s3),
	                scale.x * (c2 * s3),
	                scale.x * (c1 * s2 * s3 - c3 * s1),
	                0.0f,
	            },
	            {
	               scale.y * (c3 * s1 * s2 - c1 * s3),
	               scale.y * (c2 * c3),
	               scale.y * (c1 * c3 * s3 * s1 * s3),
	               0.0f,
	            },
	            {
	                scale.z * (c2 * s1),
	                scale.z * (-s2),
	                scale.z * (c1 * c2),
	                0.0f,
	            },
	            {translation.x, translation.y, translation.z, 1.0f}};
	    }
	};

    /// -------------------------------------------------------

} // namespace SceneryEditorX

/// -------------------------------------------------------

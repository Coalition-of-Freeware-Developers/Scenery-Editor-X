/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* transform.cpp
* -------------------------------------------------------
* Created: 24/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/scene/transform.h>
#include <typeindex>

// -------------------------------------------------------

namespace Scene
{
    /*
    Transform::Transform(Node & n) : node{n} { }

    Node &Transform::get_node() { return node; }

    std::type_index Transform::get_type() { return typeid(Transform); }

	// -------------------------------------------------------

    void Transform::set_translation(const glm::vec3 &new_translation)
    {
        translation = new_translation;

        invalidate_world_matrix();
    }
    void Transform::set_rotation(const glm::quat &new_rotation)
    {
        rotation = new_rotation;

        invalidate_world_matrix();
    }
    void Transform::set_scale(const glm::vec3 &new_scale)
    {
        scale = new_scale;

        invalidate_world_matrix();
    }

    const glm::vec3 &Transform::get_translation()   const {return translation;}
    const glm::quat &Transform::get_rotation()		const {return rotation;}
    const glm::vec3 &Transform::get_scale()			const {return scale;}

    void Transform::set_matrix(const glm::mat4 &matrix)
    {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(matrix, scale, rotation, translation, skew, perspective);

        invalidate_world_matrix();
    }

    glm::mat4 Transform::get_matrix() const
    {
        return glm::translate(glm::mat4(1.0), translation) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0), scale);
    }

    glm::mat4 Transform::get_world_matrix()
    {
        update_world_transform();

        return world_matrix;
    }

    void Transform::invalidate_world_matrix() {update_world_matrix = true;}

    void Transform::update_world_transform()
    {
        if (!update_world_matrix)
        {
            return;
        }

        world_matrix = get_matrix();

        auto parent = node.get_parent();

        if (parent)
        {
            auto &transform = parent->get_component<Transform>();
            world_matrix = transform.get_world_matrix() * world_matrix;
        }

        update_world_matrix = false;
    }
    */

} // namespace Scene

// -------------------------------------------------------

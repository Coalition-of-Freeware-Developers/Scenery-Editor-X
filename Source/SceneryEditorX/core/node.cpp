/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* node.cpp
* -------------------------------------------------------
* Created: 24/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/core/node.h>

// -------------------------------------------------------

namespace Scene
{

    Node::Node(const size_t id, const std::string &name) : id{id}, name{name}, transform{*this}
    {
        set_component(transform);
    }

    const size_t Node::get_id() const
    {
        return id;
    }

    const std::string &Node::get_name() const
    {
        return name;
    }

    void Node::set_parent(Node & p)
    {
        parent = &p;

        transform.invalidate_world_matrix();
    }

    Node *Node::get_parent() const
    {
        return parent;
    }

    void Node::add_child(Node & child)
    {
        children.push_back(&child);
    }

    const std::vector<Node *> &Node::get_children() const
    {
        return children;
    }

    void Node::set_component(Component & component)
    {
        auto it = components.find(component.get_type());

        if (it != components.end())
        {
            it->second = &component;
        }
        else
        {
            components.insert(std::make_pair(component.get_type(), &component));
        }
    }

    Component &Node::get_component(const std::type_index index)
    {
        return *components.at(index);
    }

    bool Node::has_component(const std::type_index index)
    {
        return components.count(index) > 0;
    }

} // namespace SceneryEditorX

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* node.h
* -------------------------------------------------------
* Created: 24/3/2025
* -------------------------------------------------------
*/

#pragma once

// -------------------------------------------------------

namespace Scene
{

    class Component;

    /*
	 * @brief A leaf of the tree structure which can have children and a single parent.
	 */
    class Node
    {
    public:
        Node(const size_t id, const std::string &name);

        virtual ~Node() = default;

        const size_t get_id() const;

        const std::string &get_name() const;

        //Transform &get_transform()
        //{
        //    return transform;
        //}

        void set_parent(Node &parent);

        Node *get_parent() const;

        void add_child(Node &child);

        const std::vector<Node *> &get_children() const;

        void set_component(Component &component);

        template <class T>
        inline T &get_component()
        {
            return dynamic_cast<T &>(get_component(typeid(T)));
        }

        Component &get_component(const std::type_index index);

        template <class T>
        bool has_component()
        {
            return has_component(typeid(T));
        }

        bool has_component(const std::type_index index);

    private:
        size_t id;

        std::string name;

        //Transform transform;

        Node *parent{nullptr};

        std::vector<Node *> children;

        std::unordered_map<std::type_index, Component *> components;
    };
}

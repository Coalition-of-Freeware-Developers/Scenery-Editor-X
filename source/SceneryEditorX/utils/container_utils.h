/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* container_utils.h
* -------------------------------------------------------
* Created: 12/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /// Search operations
    template<typename Container, typename T>
    bool Contains(const Container& container, const T& item)
    {
        return std::find(std::begin(container), std::end(container), item) != std::end(container);
    }

    template<typename Key, typename Value>
    bool Contains(const std::unordered_map<Key, Value>& map, const Key& key)
    {
        return map.contains(key);
    }

    /// Modification operations
    template<typename Container, typename T>
    bool AppendIfNotPresent(Container& container, T&& item)
    {
        if (Contains(container, item))
            return false;

        container.push_back(std::forward<T>(item));
        return true;
    }

    template<typename Container, typename Predicate>
    bool RemoveIf(Container& container, Predicate predicate)
    {
        auto it = std::find_if(container.begin(), container.end(), predicate);
        if (it != container.end())
        {
            container.erase(it);
            return true;
        }
        return false;
    }

    /// Transform operations
    template<typename Container, typename Func>
    auto Transform(const Container& container, Func func) -> std::vector<decltype(func(*container.begin()))>
    {
        std::vector<decltype(func(*container.begin()))> result;
        result.reserve(container.size());

        std::transform(container.begin(), container.end(), std::back_inserter(result), func);
        return result;
    }

    /// Filtering operations
    template<typename Container, typename Predicate>
    Container Filter(const Container& container, Predicate predicate)
    {
        Container result;
        std::copy_if(container.begin(), container.end(), std::back_inserter(result), predicate);
        return result;
    }

}

/// -------------------------------------------------------

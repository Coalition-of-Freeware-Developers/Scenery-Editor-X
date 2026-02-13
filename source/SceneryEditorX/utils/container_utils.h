/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * container_utils.h
 * -------------------------------------------------------
 * Created: 12/7/2025
 * -------------------------------------------------------
 */
#pragma once
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
    // Search operations
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

    // Modification operations
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

    // Transform operations
    template<typename Container, typename Func>
    auto Transform(const Container& container, Func func) -> std::vector<decltype(func(*container.begin()))>
    {
        std::vector<decltype(func(*container.begin()))> result;
        result.reserve(container.size());

        std::transform(container.begin(), container.end(), std::back_inserter(result), func);
        return result;
    }

    // Filtering operations
    template<typename Container, typename Predicate>
    Container Filter(const Container& container, Predicate predicate)
    {
        Container result;
        std::copy_if(container.begin(), container.end(), std::back_inserter(result), predicate);
        return result;
    }

}

// -------------------------------------------------------

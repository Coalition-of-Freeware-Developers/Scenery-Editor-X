/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* type_reflection.h
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once
// Core reflection components
#include <SceneryEditorX/utils/reflection/type_descriptors.h>
#include <SceneryEditorX/utils/reflection/type_names.h>
#include <SceneryEditorX/utils/reflection/type_structs.h>
#include <SceneryEditorX/utils/reflection/type_utils.h>

/// -------------------------------------------------------

namespace SceneryEditorX::Reflection
{
    /// Main reflection interface
    template<typename T>
    constexpr auto GetTypeName() -> std::string_view
    {
        return type::name<T>();
    }

    template<typename T>
    constexpr auto GetTypeInfo() -> Types::Reflection::ClassInfo
    {
        return Types::Description<T>::GetClassInfo();
    }

    template<typename T>
    constexpr bool IsReflected() -> bool
    {
        return Types::Described<T>::value;
    }
}

/// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* types.h
* -------------------------------------------------------
* Created: 9/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <array>
#include <string_view>
#include <utility>
// ReSharper disable once CommentTypo
/**
 * @link https://bitwizeshift.github.io/posts/2021/03/09/getting-an-unmangled-type-name-at-compile-time/
 */

/// -------------------------------------------------------

namespace type
{
	namespace impl
    {
		template <std::size_t...Idxs>
		constexpr auto substring_as_array(std::string_view str, std::index_sequence<Idxs...>)
		{
			return std::array{ str[Idxs]..., '\0' };
		}

		template <bool keep_namespace, typename T>
		constexpr auto type_name_array()
		{
#if defined(__clang__)
			constexpr auto prefix = std::string_view{ "T = " };
			constexpr auto suffix = std::string_view{ "]" };
			constexpr auto function = std::string_view{ __PRETTY_FUNCTION__ };
#elif defined(__GNUC__)
			constexpr auto prefix = std::string_view{ "; T = " };
			constexpr auto suffix = std::string_view{ "]" };
			constexpr auto function = std::string_view{ __PRETTY_FUNCTION__ };
#elif defined(_MSC_VER)
			constexpr auto prefix = std::string_view{ "e," }; // tru'e', or fals'e',
			constexpr auto suffix = std::string_view{ ">(void)" };
			constexpr auto function = std::string_view{ __FUNCSIG__ };
#else
# error Unsupported compiler
#endif

			constexpr auto start = function.find(prefix) + prefix.size();
			constexpr auto end = function.rfind(suffix);

			static_assert(start < end);

			constexpr auto name = function.substr(start, (end - start));

			if constexpr (keep_namespace)
                return substring_as_array(name, std::make_index_sequence<name.size()>{});
            else if constexpr (constexpr auto new_start = name.find_last_of("::"); new_start != std::string_view::npos)
            {
                static_assert(new_start + 1 < end);
                constexpr auto stripped_name = name.substr(new_start + 1, name.size() - (new_start + 1));

                return substring_as_array(stripped_name, std::make_index_sequence<stripped_name.size()>{});
            }
            else
                return substring_as_array(name, std::make_index_sequence<name.size()>{});
        }

		template <typename T, bool keep_namespace>
		struct type_name_holder
		{
			static constexpr auto value = type_name_array<keep_namespace, T>();
		};

	}

	template <typename T>
	constexpr std::string_view type_name_keep_namespace()
    {
		constexpr auto& value = impl::type_name_holder<T, true>::value;
		return std::string_view{ value.data(), value.size() - 1 };
	}

	template <typename T>
	constexpr std::string_view type_name()
    {
		constexpr auto& value = impl::type_name_holder<T, false>::value;
		return std::string_view{ value.data(), value.size() - 1 };
	}

    /// -------------------------------------------------------

}

/// -------------------------------------------------------

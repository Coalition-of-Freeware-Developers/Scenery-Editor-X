/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* log_templates.h
* -------------------------------------------------------
* Created: 4/4/2025
* -------------------------------------------------------
*/

#pragma once

#include <filesystem>
#include <format>
#include <glm/glm.hpp>

//-------------------------------------------------------

namespace std
{

	template <>
	struct formatter<filesystem::path> : formatter<string>
	{
		template <typename FormatContext>
		FormatContext::iterator format(const filesystem::path& path, FormatContext& ctx) const
		{
			return formatter<string>::format(path.string(), ctx);
		}
	};


	template<>
	struct formatter<glm::vec2>
	{
		char presentation = 'f';

		constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
		{
			auto it = ctx.begin(), end = ctx.end();
			if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;

			if (it != end && *it != '}') throw format_error("invalid format");

			return it;
		}

		template <typename FormatContext>
		auto format(const glm::vec2& vec, FormatContext& ctx) const -> decltype(ctx.out())
		{
			return presentation == 'f'
				? format_to(ctx.out(), "({:.3f}, {:.3f})", vec.x, vec.y)
				: format_to(ctx.out(), "({:.3e}, {:.3e})", vec.x, vec.y);
		}
	};

	template<>
	struct formatter<glm::vec3>
	{
		char presentation = 'f';

		constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
		{
			auto it = ctx.begin(), end = ctx.end();
			if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;

			if (it != end && *it != '}') throw format_error("invalid format");

			return it;
		}

		template <typename FormatContext>
		auto format(const glm::vec3& vec, FormatContext& ctx) const -> decltype(ctx.out())
		{
			return presentation == 'f'
				? format_to(ctx.out(), "({:.3f}, {:.3f}, {:.3f})", vec.x, vec.y, vec.z)
				: format_to(ctx.out(), "({:.3e}, {:.3e}, {:.3e})", vec.x, vec.y, vec.z);
		}
	};

	template<>
	struct formatter<glm::vec4>
	{
		char presentation = 'f';

		constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
		{
			auto it = ctx.begin(), end = ctx.end();
			if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;

			if (it != end && *it != '}') throw format_error("invalid format");

			return it;
		}

		template <typename FormatContext>
		auto format(const glm::vec4& vec, FormatContext& ctx) const -> decltype(ctx.out())
		{
			return presentation == 'f'
				? format_to(ctx.out(), "({:.3f}, {:.3f}, {:.3f}, {:.3f})", vec.x, vec.y, vec.z, vec.w)
				: format_to(ctx.out(), "({:.3e}, {:.3e}, {:.3e}, {:.3e})", vec.x, vec.y, vec.z, vec.w);
		}
	};

} // namespace std

// -------------------------------------------------------

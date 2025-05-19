/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* formatter.h
* -------------------------------------------------------
* Created: 18/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <filesystem>
#include <fmt/core.h>
#include <format>
#include <glm/glm.hpp>

/// -------------------------------------------------------

template <>
struct fmt::formatter<Vec2>
{
    char presentation = 'f';
	
    /**
	         * @brief Parses the format specification for the vector.
	         * 
	         * Accepts 'f' for fixed-point notation (default) or 'e' for scientific notation.
	         * 
	         * @param ctx The format parse context
	         * @return Iterator pointing past the parsed format specification
	         * @throws format_error if the format specification is invalid
	         */
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e'))
            presentation = *it++;
	
        if (it != end && *it != '}')
            throw format_error("invalid format");
	
        return it;
    }
	
    /**
	         * @brief Formats the Vec2 object into the output.
	         * 
	         * Renders the vector as "(x, y)" with 3 decimal places in either
	         * fixed-point or scientific notation based on the presentation format.
	         * 
	         * @param vec The vector to format
	         * @param ctx The formatting context that receives the formatted output
	         * @return Iterator pointing past the end of the formatted output
	         */
    template <typename FormatContext>
    auto format(const Vec2 &vec, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return presentation == 'f' ? fmt::format_to(ctx.out(), "({:.3f}, {:.3f})", vec.x, vec.y)
                   : fmt::format_to(ctx.out(), "({:.3e}, {:.3e})", vec.x, vec.y);
    }
}; // namespace fmt

/// -------------------------------------------------------
///
namespace std
{
	/**
	 * @struct formatter
	 * @brief Specialization of the std::formatter for std::filesystem::path objects.
	 * 
	 * This formatter enables using std::format() and std::format_to() with
	 * filesystem::path objects in format strings. It inherits from formatter<string>
	 * to leverage string formatting capabilities for the path's string representation.
	 * 
	 * @example
	 *   std::filesystem::path myPath = "some/directory/file.txt";
	 *   std::string formatted = std::format("Path: {}", myPath);
	 */
	template <>
	struct formatter<filesystem::path> : formatter<string>
	{
	    /**
		 * Formats the filesystem::path object into the output.
		 * 
		 * @param path The filesystem path object to format
		 * @param ctx The formatting context that receives the formatted output
		 * @return Iterator pointing past the end of the formatted output
		 */
	    template <typename FormatContext>
	    typename FormatContext::iterator format(const filesystem::path &path, FormatContext &ctx) const
	    {
	        return formatter<string>::format(path.string(), ctx);
	    }
	};
	
	/// -------------------------------------------------------
	
	/**
	 * @struct formatter
	 * @brief Specialization of the std::formatter for glm::vec2 objects.
	 * 
	 * This formatter enables using std::format() and std::format_to() with
	 * glm::vec2 objects in format strings. Supports both fixed-point ('f')
	 * and scientific ('e') presentation formats.
	 * 
	 * @example
	 *   glm::vec2 position(1.2345f, 6.7890f);
	 *   std::string formatted = std::format("Position: {}", position);     // Uses fixed format: "(1.235, 6.789)"
	 *   std::string scientific = std::format("Position: {:e}", position);  // Uses scientific: "(1.235e+00, 6.789e+00)"
	 */
	template <>
	struct formatter<glm::vec2>
	{
	    char presentation = 'f';
	
	    /**
		 * @brief Parses the format specification for the vector.
		 * 
		 * Accepts 'f' for fixed-point notation (default) or 'e' for scientific notation.
		 * 
		 * @param ctx The format parse context
		 * @return Iterator pointing past the parsed format specification
		 * @throws format_error if the format specification is invalid
		 */
	    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
	    {
	        auto it = ctx.begin(), end = ctx.end();
	        if (it != end && (*it == 'f' || *it == 'e'))
	            presentation = *it++;
	
	        if (it != end && *it != '}')
	            throw format_error("invalid format");
	
	        return it;
	    }
	
	    /**
		 * @brief Formats the glm::vec2 object into the output.
		 * 
		 * Renders the vector as "(x, y)" with 3 decimal places in either
		 * fixed-point or scientific notation based on the presentation format.
		 * 
		 * @param vec The vector to format
		 * @param ctx The formatting context that receives the formatted output
		 * @return Iterator pointing past the end of the formatted output
		 */
	    template <typename FormatContext>
	    auto format(const glm::vec2 &vec, FormatContext &ctx) const -> decltype(ctx.out())
	    {
	        return presentation == 'f' ? format_to(ctx.out(), "({:.3f}, {:.3f})", vec.x, vec.y)
	                                   : format_to(ctx.out(), "({:.3e}, {:.3e})", vec.x, vec.y);
	    }
	};
	
	/// -------------------------------------------------------
	
	/**
	 * @struct formatter
	 * @brief Specialization of the std::formatter for glm::vec3 objects.
	 * 
	 * This formatter enables using std::format() and std::format_to() with
	 * glm::vec3 objects in format strings. Supports both fixed-point ('f')
	 * and scientific ('e') presentation formats.
	 * 
	 * @example
	 *   glm::vec3 position(1.2345f, 6.7890f, 9.1234f);
	 *   std::string formatted = std::format("Position: {}", position);     // Uses fixed format: "(1.235, 6.789, 9.123)"
	 *   std::string scientific = std::format("Position: {:e}", position);  // Uses scientific: "(1.235e+00, 6.789e+00, 9.123e+00)"
	 */
	template <>
	struct formatter<glm::vec3>
	{
	    char presentation = 'f';
	
	    /**
		 * @brief Parses the format specification for the vector.
		 * 
		 * Accepts 'f' for fixed-point notation (default) or 'e' for scientific notation.
		 * 
		 * @param ctx The format parse context
		 * @return Iterator pointing past the parsed format specification
		 * @throws format_error if the format specification is invalid
		 */
	    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
	    {
	        auto it = ctx.begin(), end = ctx.end();
	        if (it != end && (*it == 'f' || *it == 'e'))
	            presentation = *it++;
	
	        if (it != end && *it != '}')
	            throw format_error("invalid format");
	
	        return it;
	    }
	
	    /**
		 * @brief Formats the glm::vec3 object into the output.
		 * 
		 * Renders the vector as "(x, y, z)" with 3 decimal places in either
		 * fixed-point or scientific notation based on the presentation format.
		 * 
		 * @param vec The vector to format
		 * @param ctx The formatting context that receives the formatted output
		 * @return Iterator pointing past the end of the formatted output
		 */
	    template <typename FormatContext>
	    auto format(const glm::vec3 &vec, FormatContext &ctx) const -> decltype(ctx.out())
	    {
	        return presentation == 'f' ? format_to(ctx.out(), "({:.3f}, {:.3f}, {:.3f})", vec.x, vec.y, vec.z)
	                                   : format_to(ctx.out(), "({:.3e}, {:.3e}, {:.3e})", vec.x, vec.y, vec.z);
	    }
	};
	
	/// -------------------------------------------------------
	/**
	 * @struct formatter
	 * @brief Specialization of the std::formatter for glm::vec4 objects.
	 * 
	 * This formatter enables using std::format() and std::format_to() with
	 * glm::vec4 objects in format strings. Supports both fixed-point ('f')
	 * and scientific ('e') presentation formats.
	 * 
	 * @example
	 *   glm::vec4 color(1.0f, 0.5f, 0.2f, 1.0f);
	 *   std::string formatted = std::format("Color: {}", color);     // Uses fixed format: "(1.000, 0.500, 0.200, 1.000)"
	 *   std::string scientific = std::format("Color: {:e}", color);  // Uses scientific: "(1.000e+00, 5.000e-01, 2.000e-01, 1.000e+00)"
	 */
	template <>
	struct formatter<glm::vec4>
	{
	    char presentation = 'f';
	
	    /**
		 * @brief Parses the format specification for the vector.
		 * 
		 * Accepts 'f' for fixed-point notation (default) or 'e' for scientific notation.
		 * 
		 * @param ctx The format parse context
		 * @return Iterator pointing past the parsed format specification
		 * @throws format_error if the format specification is invalid
		 */
	    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
	    {
	        auto it = ctx.begin(), end = ctx.end();
	        if (it != end && (*it == 'f' || *it == 'e'))
	            presentation = *it++;
	
	        if (it != end && *it != '}')
	            throw format_error("invalid format");
	
	        return it;
	    }
	
	    /**
		 * @brief Formats the glm::vec4 object into the output.
		 * 
		 * Renders the vector as "(x, y, z, w)" with 3 decimal places in either
		 * fixed-point or scientific notation based on the presentation format.
		 * 
		 * @param vec The vector to format
		 * @param ctx The formatting context that receives the formatted output
		 * @return Iterator pointing past the end of the formatted output
		 */
	    template <typename FormatContext>
	    auto format(const glm::vec4 &vec, FormatContext &ctx) const -> decltype(ctx.out())
	    {
	        return presentation == 'f'
	                   ? format_to(ctx.out(), "({:.3f}, {:.3f}, {:.3f}, {:.3f})", vec.x, vec.y, vec.z, vec.w)
	                   : format_to(ctx.out(), "({:.3e}, {:.3e}, {:.3e}, {:.3e})", vec.x, vec.y, vec.z, vec.w);
	    }
	};
	
} // namespace std

// -------------------------------------------------------

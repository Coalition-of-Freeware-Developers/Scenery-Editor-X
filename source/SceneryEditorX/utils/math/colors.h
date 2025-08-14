/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* colors.h
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/utils/math/math_utils.h>

/// -----------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @brief RGBA color representation with floating-point precision
	 *
	 * The Color class provides a comprehensive color representation system with support for
	 * multiple color spaces (RGB, HSV), format conversions, and mathematical operations.
	 * Colors are stored internally as floating-point values in the range [0.0, 1.0] for
	 * maximum precision and ease of GPU shader integration.
	 *
	 * Memory layout is optimized for GPU usage with components stored in reverse order
	 * (a, b, g, r) to match common graphics API expectations.
	 *
	 * @note - Color values are not automatically clamped to [0.0, 1.0] range to allow
	 *       HDR (High Dynamic Range) color operations
	 * @warning Negative color values or values > 1.0 are valid for HDR workflows but
	 *          may produce unexpected results when converted to integer formats
	 */
	class Color
    {
    public:
    	/**
    	 * @brief Default constructor creating a transparent black color
    	 *
    	 * Creates a color with all components set to 0, representing
    	 * transparent black (0, 0, 0, 0).
    	 */
        Color() : a(0), b(0), g(0), r(0) {}

		/**
		 * @brief Constructs a color from RGBA components with alpha
		 *
		 * Constructs a color with specified RGBA values.
		 * Values outside the [0.0, 1.0] range are valid for HDR workflows.
		 * This allows for high dynamic range colors that can exceed standard limits.
		 *
		 * @param r Red component [0.0, 1.0] (values outside range are valid for HDR)
		 * @param g Green component [0.0, 1.0] (values outside range are valid for HDR)
		 * @param b Blue component [0.0, 1.0] (values outside range are valid for HDR)
		 * @param a Alpha (opacity) component [0.0, 1.0] where 0=transparent, 1=opaque
		 *
		 * @note - This constructor allows for HDR colors with values outside [0.0, 1.0]
		 * @warning - Negative values or values > 1.0 may produce unexpected results
		 *          when converted to integer formats (e.g., RGBA8)
		 *
		 * @code
		 * Color semiTransparentRed(1.0f, 0.0f, 0.0f, 0.5f); // Red with 50% opacity
		 * Color brightGreen(0.0f, 2.0f, 0.0f, 1.0f); // Green with HDR value
		 * Color blue(0.0f, 0.0f, 1.0f, 1.0f); // Fully opaque blue
		 * Color transparentBlack(0.0f, 0.0f, 0.0f, 0.0f); // Transparent black
		 * Color hdrColor(1.5f, 0.5f, 0.0f, 1.0f); // HDR color with red > 1.0
		 * Color negativeBlue(-0.5f, 0.0f, 1.0f, 1.0f); // HDR color with negative blue
		 * @endcode
		 */
        Color(const float r, const float g, const float b, const float a) : a(a), b(b), g(g), r(r) {}

        /**
         * @brief Constructs an opaque color from RGB components
         *
         * Creates a color with the specified RGB values and alpha set to 1.0 (fully opaque).
         *
         * @param r Red component [0.0, 1.0] (values outside range are valid for HDR)
         * @param g Green component [0.0, 1.0] (values outside range are valid for HDR)
         * @param b Blue component [0.0, 1.0] (values outside range are valid for HDR)
         *
         * @note - This constructor allows for HDR colors with values outside [0.0, 1.0]
         *
         * @warning - Negative values or values > 1.0 may produce unexpected results
         *
         * @code
         * Color brightRed(2.0f, 0.0f, 0.0f); // HDR red with value > 1.0
         * Color green(0.0f, 1.0f, 0.0f); // Fully opaque green
         * Color blue(0.0f, 0.0f, 1.0f); // Fully opaque blue
         * Color negativeGreen(-0.5f, 1.0f, 0.0f); // HDR green with negative value
         * Color hdrBlue(0.0f, 0.0f, 1.5f); // HDR blue with value > 1.0
         * Color transparentBlack(0.0f, 0.0f, 0.0f); // Transparent black (alpha=1.0)
         * @endcode
         */
        Color(const float r, const float g, const float b) : a(1), b(b), g(g), r(r) {}

        /**
         * @brief Constructs a color from a 4D vector (Vec4)
         *
         * Constructs a color with specified RGBA values from a Vec4.
         *
         * @param v Vec4 where x=red, y=green, z=blue, w=alpha
         *
         * @note - This constructor allows for HDR colors with values outside [0.0, 1.0]
         * @warning - Negative values or values > 1.0 may produce unexpected results
         *
         * @code
         * Color semiTransparentBlue(Vec4(0.0f, 0.0f, 1.0f, 0.5f)); // Blue with 50% opacity
         * Color brightYellow(Vec4(2.0f, 2.0f, 0.0f, 1.0f)); // HDR yellow with red and green > 1.0
         * Color opaqueRed(Vec4(1.0f, 0.0f, 0.0f, 1.0f)); // Fully opaque red
         * Color negativeRed(Vec4(-1.0f, 0.0f, 0.0f, 1.0f)); // HDR red with negative value
         * Color hdrGreen(Vec4(0.0f, 1.5f, 0.0f, 1.0f)); // HDR green with value > 1.0
         * Color transparentBlack(Vec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent black
         * @endcode
         */
        explicit Color(const Vec4 v) : a(v.w), b(v.z), g(v.y), r(v.x) {}

        /**
         * @brief Constructs an opaque color from a 3D vector (Vec3)
         *
         * Constructs a color with specified RGB values and alpha set to 1.0 (fully opaque).
         *
         * @param v Vec3 where x=red, y=green, z=blue (alpha is set to 1.0)
         * @note - This constructor allows for HDR colors with values outside [0.0, 1.0]
         * @warning - Negative values or values > 1.0 may produce unexpected results
         *
         * @code
         * Color brightCyan(Vec3(0.0f, 1.0f, 1.0f)); // Fully opaque cyan
         * Color hdrPurple(Vec3(1.5f, 0.0f, 1.5f)); // HDR purple with red and blue > 1.0
         * Color opaqueYellow(Vec3(1.0f, 1.0f, 0.0f)); // Fully opaque yellow
         * Color negativeBlue(Vec3(0.0f, 0.0f, -1.0f)); // HDR blue with negative value
         * Color hdrRed(Vec3(2.0f, 0.0f, 0.0f)); // HDR red with value > 1.0
         * Color transparentBlack(Vec3(0.0f, 0.0f, 0.0f)); // Transparent black (alpha=1.0)
         * @endcode
         */
        explicit Color(const Vec3 v) : a(1), b(v.z), g(v.y), r(v.x) {}

		/**
		 * @brief Creates a color from 8-bit RGBA values
		 *
		 * Converts 8-bit integer color components to floating-point representation.
		 * This is useful when working with standard image formats or UI systems.
		 *
		 * @param r Red component [0, 255]
		 * @param g Green component [0, 255]
		 * @param b Blue component [0, 255]
		 * @param a Alpha component [0, 255] (default: 255 = fully opaque)
		 * @return Color with components normalized to [0.0, 1.0] range
		 *
		 * @code
		 * Color red = Color::RGBA8(255, 0, 0);          // Fully opaque red
		 * Color green = Color::RGBA8(0, 255, 0);        // Fully opaque green
		 * Color blue = Color::RGBA8(0, 0, 255);         // Fully opaque blue
		 * Color semiTransparentYellow = Color::RGBA8(255, 255, 0, 128); // Yellow with 50% opacity
		 * Color transparentBlack = Color::RGBA8(0, 0, 0, 0); // Transparent black
		 * Color hdrColor = Color::RGBA8(300, -50, 100, 255); // HDR color with out-of-range values
		 * @endcode
		 */
		static Color RGBA8(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

		/**
		 * @brief Creates a color from a 24-bit RGB hexadecimal value
		 *
		 * Converts a packed RGB hexadecimal value to a Color object.
		 * Alpha is set to 1.0 (fully opaque).
		 *
		 * @param hex RGB value in format 0xRRGGBB (e.g., 0xFF0000 for red)
		 * @return Color with alpha set to 1.0
		 *
		 * @code
		 * Color red = Color::RGBHex(0xFF0000);     // Pure red
		 * Color green = Color::RGBHex(0x00FF00);   // Pure green
		 * Color blue = Color::RGBHex(0x0000FF);    // Pure blue
		 * Color yellow = Color::RGBHex(0xFFFF00);  // Pure yellow
		 * Color cyan = Color::RGBHex(0x00FFFF);    // Pure cyan
		 * @endcode 
		 */
        static Color RGBHex(uint32_t hex);

        // ReSharper disable once CommentTypo
        /**
         * @brief Creates a color from a 32-bit RGBA hexadecimal value
         *
         * Converts a packed RGBA hexadecimal value to a Color object.
         *
         * @param hex RGBA value in format 0xRRGGBBAA
         * @return Color with all components extracted from hex value
         *
         * @code
         * Color semiRed = Color::RGBAHex(0xFF000080);  // Red with 50% alpha
         * Color opaqueGreen = Color::RGBAHex(0x00FF00FF); // Fully opaque green
         * Color blue = Color::RGBAHex(0x0000FFFF); // Fully opaque blue
         * Color transparentBlack = Color::RGBAHex(0x00000000); // Transparent black
         * @endcode 
         */
		static Color RGBAHex(uint32_t hex);

		/**
		 * @brief Creates a color from HSV (Hue, Saturation, Value) color space
		 *
		 * Converts HSV color representation to RGB. HSV is often more intuitive
		 * for color selection and manipulation than RGB.
		 *
		 * @param h Hue in degrees [0.0, 360.0) - color wheel position
		 * @param s Saturation [0.0, 1.0] - color intensity (0=grayscale, 1=full color)
		 * @param v Value (brightness) [0.0, 1.0] - overall lightness
		 * @return Color converted from HSV to RGB with alpha=1.0
		 *
		 * @note - Hue values >= 360 are automatically wrapped to [0, 360) range
		 *
		 * @code
		 * Color red = Color::HSV(0.0f, 1.0f, 1.0f); // Pure red
		 * Color green = Color::HSV(120.0f, 1.0f, 1.0f); // Pure green
		 * Color blue = Color::HSV(240.0f, 1.0f, 1.0f); // Pure blue
		 * Color yellow = Color::HSV(60.0f, 1.0f, 1.0f); // Pure yellow
		 * @endcode 
		 */
        static Color HSV(float h, float s, float v);

        /**
         * @brief Convenience alias for RGBA8() function
         *
         * @param r Red component [0, 255]
         * @param g Green component [0, 255]
         * @param b Blue component [0, 255]
         * @param a Alpha component [0, 255] (default: 255 = fully opaque)
         * @return Color with components normalized to [0.0, 1.0] range
         *
         * @note - This is a convenience alias for RGBA8() to match common usage patterns
         *
         * @code
         * Color red = Color::RGBA(255, 0, 0);          // Fully opaque red
         * Color green = Color::RGBA(0, 255, 0);        // Fully opaque green
         * Color blue = Color::RGBA(0, 0, 255);         // Fully opaque blue
         * Color semiTransparentYellow = Color::RGBA(255, 255, 0, 128); // Yellow with 50% opacity
         * @endcode 
         */
		static Color RGBA(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 255) { return RGBA8(r, g, b, a); }

		/**
		 * @brief Equality comparison operator
		 *
		 * @param rhs Color to compare against
		 * @return true if all components are exactly equal, false otherwise
		 *
		 * @note - Uses exact floating-point comparison. For approximate comparison
		 *       with tolerance, consider implementing a separate function.
		 *
		 * @code
		 * Color c1(1.0f, 0.0f, 0.0f, 1.0f); // Red
		 * Color c2(1.0f, 0.0f, 0.0f, 1.0f); // Another red
		 * Color c3(0.5f, 0.5f, 0.5f, 1.0f); // Gray
		 * if (c1 == c2) {
		 *    // c1 and c2 are exactly equal
		 * }
		 * if (c1 != c3) {
		 *   // c1 and c3 are not equal
		 * }
		 * @endcode
		 */
        bool operator==(const Color& rhs) const { return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a; }

        /**
         * @brief Inequality comparison operator
         *
         * @param rhs Color to compare against
         * @return true if any component differs, false if all components are equal
         *
         * @note - This operator is intentionally implemented to return the same result as operator==.
         *      This is a common practice to avoid confusion, but it may not be intuitive.
         *
         * @code
         * Color c1(1.0f, 0.0f, 0.0f, 1.0f); // Red
         * Color c2(1.0f, 0.0f, 0.0f, 1.0f); // Another red
         * Color c3(0.5f, 0.5f, 0.5f, 1.0f); // Gray
         * if (c1 != c2) {
         *   // c1 and c2 are not equal (this will not happen)
         * }
         * if (c1 != c3) {
         *  // c1 and c3 are not equal
         * }
         * @endcode
         */
        bool operator!=(const Color& rhs) const { return operator==(rhs); }

		/**
		 * @brief Scalar multiplication with signed integer
		 *
		 * @param value Scalar multiplier
		 * @return New color with all components multiplied by value
		 *
		 * @note - This operation does not clamp values to [0.0, 1.0] range,
		 * allowing for HDR colors.
		 *
		 * @warning - Negative values or values > 1.0 may produce unexpected results
		 *         when converted to integer formats (e.g., RGBA8)
		 *
		 * @code
		 * Color red(1.0f, 0.0f, 0.0f, 1.0f); // Red color
		 * Color brightRed = red * 2; // Bright red with components (2.0, 0.0, 0.0, 1.0)
		 * Color darkGreen = Color::Green() * -0.5f; // Dark green with components (0.0, -0.5, 0.0, 1.0)
		 * Color semiTransparentBlue = Color::Blue() * 0.5f; // Blue with components (0.0, 0.0, 0.5, 1.0)
		 * Color hdrColor = Color::Red() * 1.5f; // HDR red with components (1.5, 0.0, 0.0, 1.0)
		 * @endcode
		 */
        Color operator*(const int32_t value) const { return {value * r, value * g, value * b, value * a}; }

        /**
         * @brief Scalar multiplication with unsigned integer
         *
         * @param value Scalar multiplier
         * @return New color with all components multiplied by value
         *
         * @note - This operation does not clamp values to [0.0, 1.0] range,
         * allowing for HDR colors.
         *
         * @warning - Negative values or values > 1.0 may produce unexpected results
         *         when converted to integer formats (e.g., RGBA8)
         *
         * @code
         * Color red(1.0f, 0.0f, 0.0f, 1.0f); // Red color
         * Color brightRed = red * 2u; // Bright red with components (2.0, 0.0, 0.0, 1.0)
         * Color darkGreen = Color::Green() * 0u; // Dark green with components (0.0, 0.0, 0.0, 1.0)
         * Color semiTransparentBlue = Color::Blue() * 0.5f; // Blue with components (0.0, 0.0, 0.5, 1.0)
         * Color hdrColor = Color::Red() * 1.5f; // HDR red with components (1.5, 0.0, 0.0, 1.0)
         * @endcode
         */
        Color operator*(const uint32_t value) const { return {value * r, value * g, value * b, value * a}; }

        /**
         * @brief Scalar multiplication with floating-point value
         *
         * @param value Scalar multiplier
         * @return New color with all components multiplied by value
         *
         * @note - This operation does not clamp values to [0.0, 1.0] range,
         * allowing for HDR colors.
         *
         * @warning - Negative values or values > 1.0 may produce unexpected results
         *        when converted to integer formats (e.g., RGBA8)
         *
         * @code
         * Color red(1.0f, 0.0f, 0.0f, 1.0f); // Red color
         * Color brightRed = red * 2.0f; // Bright red with components (2.0, 0.0, 0.0, 1.0)
         * Color darkGreen = Color::Green() * -0.5f; // Dark green with components (0.0, -0.5, 0.0, 1.0)
         * Color semiTransparentBlue = Color::Blue() * 0.5f; // Blue with components (0.0, 0.0, 0.5, 1.0)
         * Color hdrColor = Color::Red() * 1.5f; // HDR red with components (1.5, 0.0, 0.0, 1.0)
         * @endcode
         */
        Color operator*(const float value) const { return {value * r, value * g, value * b, value * a}; }

		/**
		 * @brief In-place scalar multiplication with signed integer
		 *
		 * @param value Scalar multiplier
		 * @return Reference to this color after multiplication
		 *
		 * @note - This operation does not clamp values to [0.0, 1.0] range,
		 * allowing for HDR colors.
		 * @warning - Negative values or values > 1.0 may produce unexpected results
		 *
		 * @code
		 * Color red(1.0f, 0.0f, 0.0f, 1.0f); // Red color
		 * red *= 2; // Bright red with components (2.0, 0.0, 0.0, 1.0)
		 * Color green(0.0f, 1.0f, 0.0f, 1.0f); // Green color
		 * green *= -0.5f; // Dark green with components (0.0, -0.5, 0.0, 1.0)
		 * Color blue(0.0f, 0.0f, 1.0f, 1.0f); // Blue color
		 * blue *= 0.5f; // Semi-transparent blue with components (0.0, 0.0, 0.5, 1.0)
		 * Color hdrColor(1.0f, 0.0f, 0.0f, 1.0f); // HDR red color
		 * hdrColor *= 1.5f; // HDR red with components (1.5, 0.0, 0.0, 1.0)
		 * @endcode
		 */
        Color operator*=(const int32_t value)
        {
            *this = *this * value;
            return *this;
        }

		/**
		 * @brief In-place scalar multiplication with unsigned integer
		 *
		 * @param value Scalar multiplier
		 * @return Reference to this color after multiplication
		 *
		 * @note - This operation does not clamp values to [0.0, 1.0] range,
		 * allowing for HDR colors.
		 *
		 * @warning - Negative values or values > 1.0 may produce unexpected results
		 *        when converted to integer formats (e.g., RGBA8)
		 *
		 * @code
		 * Color red(1.0f, 0.0f, 0.0f, 1.0f); // Red color
		 * red *= 2u; // Bright red with components (2.0, 0.0, 0.0, 1.0)
		 * Color green(0.0f, 1.0f, 0.0f, 1.0f); // Green color
		 * green *= 0u; // Dark green with components (0.0, 0.0, 0.0, 1.0)
		 * Color blue(0.0f, 0.0f, 1.0f, 1.0f); // Blue color
		 * blue *= 0.5f; // Semi-transparent blue with components (0.0, 0.0, 0.5, 1.0)
		 * Color hdrColor(1.0f, 0.0f, 0.0f, 1.0f); // HDR red color
		 * hdrColor *= 1.5f; // HDR red with components (1.5, 0.0, 0.0, 1.0)
		 * @endcode
		 */
        Color operator*=(const uint32_t value)
        {
            *this = *this * value;
            return *this;
        }

		/**
		 * @brief In-place scalar multiplication with floating-point value
		 *
		 * @param value Scalar multiplier
		 * @return Reference to this color after multiplication
		 *
		 * @note - This operation does not clamp values to [0.0, 1.0] range,
		 * allowing for HDR colors.
		 *
		 * @warning - Negative values or values > 1.0 may produce unexpected results
		 *       when converted to integer formats (e.g., RGBA8)
		 *
		 * @code
		 * Color red(1.0f, 0.0f, 0.0f, 1.0f); // Red color
		 * red *= 2.0f; // Bright red with components (2.0, 0.0, 0.0, 1.0)
		 * Color green(0.0f, 1.0f, 0.0f, 1.0f); // Green color
		 * green *= -0.5f; // Dark green with components (0.0, -0.5, 0.0, 1.0)
		 * Color blue(0.0f, 0.0f, 1.0f, 1.0f); // Blue color
		 * blue *= 0.5f; // Semi-transparent blue with components (0.0, 0.0, 0.5, 1.0)
		 * Color hdrColor(1.0f, 0.0f, 0.0f, 1.0f); // HDR red color
		 * hdrColor *= 1.5f; // HDR red with components (1.5, 0.0, 0.0, 1.0)
		 * @endcode
		 */
        Color operator*=(const float value)
        {
            *this = *this * value;
            return *this;
        }

		/**
		 * @brief Scalar division with floating-point value
		 *
		 * @param value Divisor (should not be zero)
		 * @return New color with all components divided by value
		 *
		 * @warning Division by zero will result in undefined behavior (likely infinity/NaN)
		 *
		 * @note - This operation does not clamp values to [0.0, 1.0] range,
		 * allowing for HDR colors.
		 *
		 * @warning - Negative values or values > 1.0 may produce unexpected results
		 *
		 * @code
		 * Color red(1.0f, 0.0f, 0.0f, 1.0f); // Red color
		 * Color halfRed = red / 2.0f; // Half red with components (0.5, 0.0, 0.0, 1.0)
		 * Color darkGreen = Color::Green() / -2.0f; // Dark green with components (0.0, -0.5, 0.0, 1.0)
		 * Color semiTransparentBlue = Color::Blue() / 0.5f; // Blue with components (0.0, 0.0, 2.0, 1.0)
		 * Color hdrColor = Color::Red() / 1.5f; // HDR red with components (0.6667, 0.0, 0.0, 1.0)
		 * @endcode
		 */
        Color operator/(const float value) const { return {r / value, g / value, b / value, a / value}; }

		/**
		 * @brief In-place scalar division with floating-point value
		 *
		 * @param value Divisor (should not be zero)
		 * @return Reference to this color after division
		 *
		 * @warning Division by zero will result in undefined behavior (likely infinity/NaN)
		 *
		 * @note - This operation does not clamp values to [0.0, 1.0] range,
		 * allowing for HDR colors.
		 *
		 * @warning - Negative values or values > 1.0 may produce unexpected results
		 *       when converted to integer formats (e.g., RGBA8)
		 *
		 * @code
		 * Color red(1.0f, 0.0f, 0.0f, 1.0f); // Red color
		 * red /= 2.0f; // Half red with components (0.5, 0.0, 0.0, 1.0)
		 * Color green(0.0f, 1.0f, 0.0f, 1.0f); // Green color
		 * green /= -2.0f; // Dark green with components (0.0, -0.5, 0.0, 1.0)
		 * Color blue(0.0f, 0.0f, 1.0f, 1.0f); // Blue color
		 * blue /= 0.5f; // Semi-transparent blue with components (0.0, 0.0, 2.0, 1.0)
		 * Color hdrColor(1.0f, 0.0f, 0.0f, 1.0f); // HDR red color
		 * hdrColor /= 1.5f; // HDR red with components (0.6667, 0.0, 0.0, 1.0)
		 * @endcode
		 */
        Color operator/=(const float value)
        {
            *this = *this / value;
            return *this;
        }

		/**
		 * @brief Component access operator with bounds checking
		 *
		 * Provides array-style access to color components by index.
		 *
		 * @param index Component index (0=red, 1=green, 2=blue, 3=alpha)
		 * @return Component value, or 0.0 if index is out of range
		 *
		 * @note - Returns 0.0 for invalid indices instead of throwing exception
		 * @warning - This operator does not perform bounds checking on the index.
		 *
		 * @code
		 * Color color(0.5f, 0.25f, 0.75f, 1.0f); // Color with RGBA components
		 * float red = color[0];   // Access red component (0.5)
		 * float green = color[1]; // Access green component (0.25)
		 * float blue = color[2];  // Access blue component (0.75)
		 * float alpha = color[3]; // Access alpha component (1.0)
		 * float invalid = color[4]; // Access out-of-bounds index (returns 0.0)
		 * @endcode
		 */
        float operator[](const uint32_t index) const
        {
            if (index == 0)
                return r;

            if (index == 1)
                return g;

            if (index == 2)
                return b;

            if (index == 3)
                return a;

            return 0;
        }

        /**
         * Preset Colors
         * Static factory methods for commonly used colors
         */
        static Color Red() { return {1, 0, 0, 1}; }     ///< Pure red (1,0,0,1)
        static Color Green() { return {0, 1, 0, 1}; }   ///< Pure green (0,1,0,1)
        static Color Blue() { return {0, 0, 1, 1}; }    ///< Pure blue (0,0,1,1)
        static Color Black() { return {0, 0, 0, 1}; }   ///< Opaque black (0,0,0,1)
        static Color White() { return {1, 1, 1, 1}; }   ///< Opaque white (1,1,1,1)
        static Color Yellow() { return {1, 1, 0, 1}; }  ///< Pure yellow (1,1,0,1)
        static Color Clear() { return {0, 0, 0, 0}; }   ///< Transparent black (0,0,0,0)
        static Color Cyan() { return {0, 1, 1, 1}; }    ///< Pure cyan (0,1,1,1)

		/**
		 * @brief Converts color to 32-bit unsigned integer representation
		 *
		 * Packs the color components into a single 32-bit value in RGBA order.
		 * Components are clamped to [0.0, 1.0] range and scaled to [0, 255].
		 *
		 * @return 32-bit packed color in format 0xRRGGBBAA
		 *
		 * @note - Values outside [0.0, 1.0] range will be clamped during conversion
		 * @note - This conversion may lose precision for HDR colors
		 *
		 * @code
		 * Color color(0.5f, 0.25f, 0.75f, 1.0f); // Color with RGBA components
		 * uint32_t packedColor = color.ToU32(); // Converts to 0x3F7FBF7F (0.5, 0.25, 0.75, 1.0)
		 * Color hdrColor(1.5f, -0.5f, 2.0f, 1.0f); // HDR color with out-of-range values
		 * uint32_t packedHdrColor = hdrColor.ToU32(); // Converts to 0xFF0000FF (clamped values)
		 * Color transparentBlack(0.0f, 0.0f, 0.0f, 0.0f); // Transparent black
		 * uint32_t packedTransparentBlack = transparentBlack.ToU32(); // Converts to 0x00000000
		 * @endcode
		 */
        [[nodiscard]] uint32_t ToU32() const;

        /**
         * @brief Converts color to Vec4 representation
         *
         * Converts the color to a 4D vector (Vec4) with components:
         * - r: red component
         * - g: green component
         * - b: blue component
         * - a: alpha component
         *
         * @return Vec4 with r=red, g=green, b=blue, a=alpha
         *
         * @note - This conversion does not clamp values to [0.0, 1.0] range,
         * allowing for HDR colors.
         * @note - Negative values or values > 1.0 may produce unexpected results
         *
         */
        [[nodiscard]] Vec4 ToVec4() const;

        /**
         * @brief Converts RGB color to HSV color space
         *
         * Converts the current RGB color to HSV (Hue, Saturation, Value) representation.
         * Alpha channel is ignored in this conversion.
         *
         * @return Vec3 with x=hue[0,360), y=saturation[0,1], z=value[0,1]
         */
        [[nodiscard]] Vec3 ToHSV() const;

		/**
		 * @brief Linear interpolation between two colors
		 *
		 * Performs component-wise linear interpolation between two colors.
		 * All four components (RGBA) are interpolated independently.
		 *
		 * @param from Starting color (t=0.0)
		 * @param to Ending color (t=1.0)
		 * @param t Interpolation parameter [0.0, 1.0] (values outside range are valid)
		 * @return Interpolated color
		 *
		 * @note - t is not clamped to [0,1] range, allowing extrapolation
		 *
		 * @code
		 * Color start(1.0f, 0.0f, 0.0f, 1.0f); // Red
		 * Color end(0.0f, 0.0f, 1.0f, 1.0f); // Blue
		 * Color mid = Color::Lerp(start, end, 0.5f); // Midpoint color (0.5, 0.0, 0.5, 1.0)
		 * Color extrapolated = Color::Lerp(start, end, 1.5f); // Extrapolated color (0.5, 0.0, 1.5, 1.0)
		 * Color negativeExtrapolated = Color::Lerp(start, end, -0.5f); // Extrapolated color (-0.5, 0.0, -0.5, 1.0)
		 * Color transparent = Color::Lerp(start, end, 0.0f); // Start color (1.0, 0.0, 0.0, 1.0)
		 * Color fullyExtrapolated = Color::Lerp(start, end, 2.0f); // Extrapolated color (0.0, 0.0, 2.0, 1.0)
		 * @endcode
		 */
        static Color Lerp(const Color& from, const Color& to, const float t)
		{
			return {
			    Math::Lerp(from.r, to.r, t),
                Math::Lerp(from.g, to.g, t),
                Math::Lerp(from.b, to.b, t),
                Math::Lerp(from.a, to.a, t)
            };
		}

        /*
        [[nodiscard]] size_t GetHash() const
        {
            size_t hash = GetHash(r);
            CombineHash(hash, g);
            CombineHash(hash, b);
            CombineHash(hash, a);
            return hash;
        }*/

		/**
		 * @brief Creates a new color with modified alpha value
		 *
		 * Returns a copy of the current color with the alpha component
		 * replaced by the specified value. RGB components remain unchanged.
		 *
		 * @param alpha New alpha value [0.0, 1.0] where 0=transparent, 1=opaque
		 * @return New color with modified alpha
		 *
		 * @note - This does not modify the original color, but returns a new instance
		 * @note - Alpha value is not clamped to [0.0, 1.0] range, allowing for HDR effects
		 * @note - Negative values or values > 1.0 may produce unexpected results
		 * 
		 * @code
		 * Color original(1.0f, 0.5f, 0.0f, 1.0f); // Original color (orange)
		 * Color semiTransparent = original.WithAlpha(0.5f); // New color with 50% opacity
		 * Color fullyTransparent = original.WithAlpha(0.0f); // New color fully transparent
		 * Color hdrAlpha = original.WithAlpha(1.5f); // New color with HDR alpha (1.5)
		 * Color negativeAlpha = original.WithAlpha(-0.5f); // New color with negative alpha
		 * @endcode
		 */
        [[nodiscard]] Color WithAlpha(float alpha) const { return {r, g, b, alpha}; }

		/**
		 * @brief Color component storage union
		 *
		 * Provides multiple ways to access color components:
		 * - Individual components: a, b, g, r
		 * - Array access: rgba[0-3]
		 *
		 * @note - Components are stored in reverse order (a,b,g,r) for GPU compatibility
		 * @note - This union allows for both structured and array-style access to color components
		 * @note - Modifying components directly may lead to unexpected results if not used carefully
		 *
		 * @code
		 * Color color(1.0f, 0.5f, 0.0f, 1.0f); // Original color (orange)
		 * float red = color.r; // Access red component (1.0)
		 * float green = color.g; // Access green component (0.5)
		 * float blue = color.b; // Access blue component (0.0)
		 * float alpha = color.a; // Access alpha component (1.0)
		 * float rgbaArray[4] = {color.r, color.g, color.b, color.a}; // Array-style access
		 * color.rgba[0] = 0.8f; // Modify red component (now 0.8, 0.5, 0.0, 1.0)
		 * color.rgba[3] = 0.5f; // Modify alpha component (now 0.8, 0.5, 0.0, 0.5)
		 * @endcode
		 * 
		 * @note - This union is designed for performance and convenience, but care should be taken
		 *       to avoid modifying components in a way that breaks the intended color representation.
		 *       For example, setting alpha to 0.0 will make the color fully transparent, while setting
		 *       it to a value > 1.0 will create an HDR color.
		 */
        union
	    {
            struct { float a, b, g, r; };	///< Individual component access (reverse order)
            float rgba[4] = { 0, 0, 0, 0 }; ///< Array-style component access
        };
    };

}

/// -----------------------------------------------------

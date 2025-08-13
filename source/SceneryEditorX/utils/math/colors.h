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

namespace SceneryEditorX::Utils
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
	 * @note Color values are not automatically clamped to [0.0, 1.0] range to allow
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
		 * @param r Red component [0.0, 1.0] (values outside range are valid for HDR)
		 * @param g Green component [0.0, 1.0] (values outside range are valid for HDR)
		 * @param b Blue component [0.0, 1.0] (values outside range are valid for HDR)
		 * @param a Alpha (opacity) component [0.0, 1.0] where 0=transparent, 1=opaque
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
         */
        Color(const float r, const float g, const float b) : a(1), b(b), g(g), r(r) {}

        /**
         * @brief Constructs a color from a 4D vector (Vec4)
         *
         * @param v Vec4 where x=red, y=green, z=blue, w=alpha
         */
        explicit Color(const Vec4 v) : a(v.w), b(v.z), g(v.y), r(v.x) {}

        /**
         * @brief Constructs an opaque color from a 3D vector (Vec3)
         *
         * @param v Vec3 where x=red, y=green, z=blue (alpha is set to 1.0)
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
		 * @example
		 * Color red = Color::RGBHex(0xFF0000);     // Pure red
		 * Color green = Color::RGBHex(0x00FF00);   // Pure green
		 * Color blue = Color::RGBHex(0x0000FF);    // Pure blue
		 */
        static Color RGBHex(uint32_t hex);

        /**
         * @brief Creates a color from a 32-bit RGBA hexadecimal value
         *
         * Converts a packed RGBA hexadecimal value to a Color object.
         *
         * @param hex RGBA value in format 0xRRGGBBAA
         * @return Color with all components extracted from hex value
         *
         * @example
         * Color semiRed = Color::RGBAHex(0xFF000080);  // Red with 50% alpha
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
		 * @note Hue values >= 360 are automatically wrapped to [0, 360) range
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
         */
		static Color RGBA(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 255) { return RGBA8(r, g, b, a); }

		/**
		 * @brief Equality comparison operator
		 *
		 * @param rhs Color to compare against
		 * @return true if all components are exactly equal, false otherwise
		 *
		 * @note Uses exact floating-point comparison. For approximate comparison
		 *       with tolerance, consider implementing a separate function.
		 */
        bool operator==(const Color& rhs) const { return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a; }

        /**
         * @brief Inequality comparison operator
         *
         * @param rhs Color to compare against
         * @return true if any component differs, false if all components are equal
         */
        bool operator!=(const Color& rhs) const { return operator==(rhs); }

		/**
		 * @brief Scalar multiplication with signed integer
		 *
		 * @param value Scalar multiplier
		 * @return New color with all components multiplied by value
		 */
        Color operator*(const int32_t value) const { return {value * r, value * g, value * b, value * a}; }

        /**
         * @brief Scalar multiplication with unsigned integer
         *
         * @param value Scalar multiplier
         * @return New color with all components multiplied by value
         */
        Color operator*(const uint32_t value) const { return {value * r, value * g, value * b, value * a}; }

        /**
         * @brief Scalar multiplication with floating-point value
         *
         * @param value Scalar multiplier
         * @return New color with all components multiplied by value
         */
        Color operator*(const float value) const { return {value * r, value * g, value * b, value * a}; }

		/**
		 * @brief In-place scalar multiplication with signed integer
		 *
		 * @param value Scalar multiplier
		 * @return Reference to this color after multiplication
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
		 */
        Color operator/(const float value) const { return {r / value, g / value, b / value, a / value}; }

		/**
		 * @brief In-place scalar division with floating-point value
		 *
		 * @param value Divisor (should not be zero)
		 * @return Reference to this color after division
		 *
		 * @warning Division by zero will result in undefined behavior (likely infinity/NaN)
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
		 * @note Returns 0.0 for invalid indices instead of throwing exception
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

        /** @name Preset Colors
         * Static factory methods for commonly used colors
         * @{
         */
        static Color Red() { return {1, 0, 0, 1}; }     ///< Pure red (1,0,0,1)
        static Color Green() { return {0, 1, 0, 1}; }   ///< Pure green (0,1,0,1)
        static Color Blue() { return {0, 0, 1, 1}; }    ///< Pure blue (0,0,1,1)
        static Color Black() { return {0, 0, 0, 1}; }   ///< Opaque black (0,0,0,1)
        static Color White() { return {1, 1, 1, 1}; }   ///< Opaque white (1,1,1,1)
        static Color Yellow() { return {1, 1, 0, 1}; }  ///< Pure yellow (1,1,0,1)
        static Color Clear() { return {0, 0, 0, 0}; }   ///< Transparent black (0,0,0,0)
        static Color Cyan() { return {0, 1, 1, 1}; }    ///< Pure cyan (0,1,1,1)
        /** @} */

		/**
		 * @brief Converts color to 32-bit unsigned integer representation
		 *
		 * Packs the color components into a single 32-bit value in RGBA order.
		 * Components are clamped to [0.0, 1.0] range and scaled to [0, 255].
		 *
		 * @return 32-bit packed color in format 0xRRGGBBAA
		 *
		 * @note Values outside [0.0, 1.0] range will be clamped during conversion
		 */
        [[nodiscard]] uint32_t ToU32() const;

        /**
         * @brief Converts color to Vec4 representation
         *
         * @return Vec4 with x=red, y=green, z=blue, w=alpha
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
		 * @note t is not clamped to [0,1] range, allowing extrapolation
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
		 */
        [[nodiscard]] Color WithAlpha(float alpha) const { return {r, g, b, alpha}; }

		/**
		 * @brief Color component storage union
		 *
		 * Provides multiple ways to access color components:
		 * - Individual components: a, b, g, r
		 * - Array access: rgba[0-3]
		 *
		 * @note Components are stored in reverse order (a,b,g,r) for GPU compatibility
		 */
        union
	    {
            struct { float a, b, g, r; }; ///< Individual component access (reverse order)
            float rgba[4] = { 0, 0, 0, 0 }; ///< Array-style component access
        };
    };

}

/// -----------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* colors.cpp
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
// ReSharper disable CommentTypo
#include "colors.h"
#include <algorithm>
#include <cmath>

/// -----------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Predefined hue values for HSV color wheel calculations
	 *
	 * These values represent the primary and secondary colors at specific
	 * hue positions on the color wheel, used internally by the HSV conversion
	 * algorithm to interpolate between color segments.
	 *
	 * @note - The array contains colors in the order of Red, Yellow, Green,
	 *        Cyan, Blue, Magenta, and wraps back to Red.
	 * @note - This array is used for HSV calculations and should not be modified.
	 *
	 * @code
	 * Color red = hueValues[0];    // Red color (1, 0, 0)
	 * Color yellow = hueValues[1]; // Yellow color (1, 1, 0)
	 * Color green = hueValues[2];  // Green color (0, 1, 0)
	 * Color cyan = hueValues[3];   // Cyan color (0, 1, 1)
	 * Color blue = hueValues[4];   // Blue color (0, 0, 1)
	 * Color magenta = hueValues[5]; // Magenta color (1, 0, 1)
	 * @endcode 
	 */
	static const Color hueValues[] = {
		Color(1, 0, 0), Color(1, 1, 0), Color(0, 1, 0),
    	Color(0, 1, 1), Color(0, 0, 1), Color(1, 0, 1),
    	Color(1, 0, 0) };

	Color Color::RGBA8(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
	{
		return Color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
	}

	Color Color::RGBHex(const uint32_t hex)
	{
		return RGBA8(static_cast<uint8_t>(hex >> 16), static_cast<uint8_t>(hex >> 8), static_cast<uint8_t>(hex));
	}

	Color Color::RGBAHex(const uint32_t hex)
	{
		return RGBA8(static_cast<uint8_t>(hex >> 24), static_cast<uint8_t>(hex >> 16), static_cast<uint8_t>(hex >> 8), static_cast<uint8_t>(hex));
	}

	Color Color::HSV(float h, float s, float v)
	{
        Color rgb{};
        rgb.a = 1.0f;

		/// Clamp hue to valid range [0, 360)
		if (h >= 360)
			h = 359.999f;

		const float rgbRange = v * s;					/// Chroma (color intensity)
		const float maxRGB = v;							/// Maximum RGB component (brightness)
		const float minRGB = v - rgbRange;				/// Minimum RGB component
		const float hPrime = h / 60.0;					/// Hue sector [0, 6)
		const float x1 = fmod(hPrime, 1.0);		/// Rising interpolation factor
		const float x2 = 1.0 - fmod(hPrime, 1.0);  /// Falling interpolation factor

		/// Determine RGB values based on hue sector
		if ((hPrime >= 0) && (hPrime < 1))
		{
			/// Red to Yellow sector
			rgb.r = maxRGB;
			rgb.g = (x1 * rgbRange) + minRGB;
			rgb.b = minRGB;
		}
		else if ((hPrime >= 1) && (hPrime < 2))
		{
			/// Yellow to Green sector
			rgb.r = (x2 * rgbRange) + minRGB;
			rgb.g = maxRGB;
			rgb.b = minRGB;
		}
		else if ((hPrime >= 2) && (hPrime < 3))
		{
			/// Green to Cyan sector
			rgb.r = minRGB;
			rgb.g = maxRGB;
			rgb.b = (x1 * rgbRange) + minRGB;
		}
		else if ((hPrime >= 3) && (hPrime < 4))
		{
			/// Cyan to Blue sector
			rgb.r = minRGB;
			rgb.g = (x2 * rgbRange) + minRGB;
			rgb.b = maxRGB;
		}
		else if ((hPrime >= 4) && (hPrime < 5))
		{
			/// Blue to Magenta sector
			rgb.r = (x1 * rgbRange) + minRGB;
			rgb.g = minRGB;
			rgb.b = maxRGB;
		}
		else if ((hPrime >= 5) && (hPrime < 6))
		{
			/// Magenta to Red sector
			rgb.r = maxRGB;
			rgb.g = minRGB;
			rgb.b = (x2 * rgbRange) + minRGB;
		}

		return rgb;
	}

	uint32_t Color::ToU32() const
    {
		return static_cast<uint32_t>(r * 255) | (static_cast<uint32_t>(g * 255) << 8) | (static_cast<uint32_t>(b * 255) << 16) | (static_cast<uint32_t>(a * 255) << 24);
    }

    Vec4 Color::ToVec4() const
	{
	    return {r, g, b, a};
	}

    Vec3 Color::ToHSV() const
    {
        const float cmax = std::max({r,g, b});	/// Maximum RGB component
        const float cmin = std::min({r,g, b});	/// Minimum RGB component
        const float diff = cmax - cmin;				/// Chroma (difference between max and min)
        float h = -1, s;

        /// Calculate Hue
        if (cmax == cmin)
        {
        	/// Achromatic (gray) - no hue
            h = 0;
        }
        else if (cmax == r)
        {
        	/// Red is dominant - hue in red-yellow-green range
            h = fmod(60 * ((g - b) / diff) + 360, 360);
        }
        else if (cmax == g)
        {
        	/// Green is dominant - hue in green-cyan-blue range
            h = fmod(60 * ((b - r) / diff) + 120, 360);
        }
        else if (cmax == b)
        {
        	/// Blue is dominant - hue in blue-magenta-red range
            h = fmod(60 * ((r - g) / diff) + 240, 360);
        }

        /// Calculate Saturation
        if (cmax == 0)
        {
        	/// Black color - no saturation
            s = 0;
        }
        else
        {
        	/// Saturation as ratio of chroma to value
            s = (diff / cmax) * 100;
        }

        /// Calculate Value (brightness)
        const float v = cmax * 100;

        /// Return HSV with saturation and value normalized to [0,1]
        return {h, s / 100.0f, v / 100.0f};
    }

}

/// -----------------------------------------------------

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
#pragma once
#include "colors.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

/// -----------------------------------------------------

namespace SceneryEditorX::Utils
{
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
		return RGBA8((uint8_t)(hex >> 16), (uint8_t)(hex >> 8), (uint8_t)(hex));
	}

	Color Color::RGBAHex(const uint32_t hex)
	{
		return RGBA8((uint8_t)(hex >> 24), (uint8_t)(hex >> 16), (uint8_t)(hex >> 8), (uint8_t)(hex));
	}

	Color Color::HSV(float h, float s, float v)
	{
        Color rgb{};
        rgb.a = 1.0f;

		if (h >= 360)
			h = 359.999f;

		float rgbRange = v * s;
		float maxRGB = v;
		float minRGB = v - rgbRange;
		float hPrime = h / 60.0;
		float x1 = fmod(hPrime, 1.0);
		float x2 = 1.0 - fmod(hPrime, 1.0);

		if ((hPrime >= 0) && (hPrime < 1))
		{
			rgb.r = maxRGB;
			rgb.g = (x1 * rgbRange) + minRGB;
			rgb.b = minRGB;
		}
		else if ((hPrime >= 1) && (hPrime < 2))
		{
			rgb.r = (x2 * rgbRange) + minRGB;
			rgb.g = maxRGB;
			rgb.b = minRGB;
		}
		else if ((hPrime >= 2) && (hPrime < 3))
		{
			rgb.r = minRGB;
			rgb.g = maxRGB;
			rgb.b = (x1 * rgbRange) + minRGB;
		}
		else if ((hPrime >= 3) && (hPrime < 4))
		{
			rgb.r = minRGB;
			rgb.g = (x2 * rgbRange) + minRGB;
			rgb.b = maxRGB;
		}
		else if ((hPrime >= 4) && (hPrime < 5))
		{
			rgb.r = (x1 * rgbRange) + minRGB;
			rgb.g = minRGB;
			rgb.b = maxRGB;
		}
		else if ((hPrime >= 5) && (hPrime < 6))
		{
			rgb.r = maxRGB;
			rgb.g = minRGB;
			rgb.b = (x2 * rgbRange) + minRGB;
		}

		return rgb;
	}

	uint32_t Color::ToU32() const
    {
		return ((uint32_t)(r * 255)) | ((uint32_t)(g * 255) << 8) | ((uint32_t)(b * 255) << 16) | ((uint32_t)(a * 255) << 24);
    }

    Vec4 Color::ToVec4() const { return {r, g, b, a}; }

    Vec3 Color::ToHSV() const
    {
        float cmax = std::max({r,g, b}); ///< maximum of r, g, b
        float cmin = std::min({r,g, b}); ///< minimum of r, g, b
        float diff = cmax - cmin;             ///< diff of cmax and cmin.
        float h = -1, s = -1;

        // if cmax and cmax are equal then h = 0
        if (cmax == cmin)
            h = 0;

        // if cmax equal r then compute h
        else if (cmax == r)
            h = fmod(60 * ((g - b) / diff) + 360, 360);

        // if cmax equal g then compute h
        else if (cmax == g)
            h = fmod(60 * ((b - r) / diff) + 120, 360);

        // if cmax equal b then compute h
        else if (cmax == b)
            h = fmod(60 * ((r - g) / diff) + 240, 360);

        // if cmax equal zero
        if (cmax == 0)
            s = 0;
        else
            s = (diff / cmax) * 100;

        float v = cmax * 100;

        return Vec3(h, s / 100.0f, v / 100.0f);
    }

}

/// -----------------------------------------------------

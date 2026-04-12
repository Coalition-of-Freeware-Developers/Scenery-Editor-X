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
 * standards.cpp
 * -------------------------------------------------------
 * Created: 21/03/2026
 * -------------------------------------------------------
 */
#include "standards.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX
{
	Standard::Standard(const float temperature_kelvin, const float alpha)
	{
		const float temperature = std::clamp(temperature_kelvin, 1000.0f, 40000.0f) / 100.0f;

		if (temperature <= 66.0f)
		{
			r = 1.0f;
			g = std::clamp(0.390081578769f * std::log(temperature) - 0.631841443788f, 0.0f, 1.0f);
			if (temperature <= 19.0f)
			{
				b = 0.0f;
			}
			else
			{
				b = std::clamp(0.543206789110f * std::log(temperature - 10.0f) - 1.19625408914f, 0.0f, 1.0f);
			}
		}
		else
		{
			r = std::clamp(1.29293618606f * std::pow(temperature - 60.0f, -0.1332047592f), 0.0f, 1.0f);
			g = std::clamp(1.12989086089f * std::pow(temperature - 60.0f, -0.0755148492f), 0.0f, 1.0f);
			b = 1.0f;
		}

		a = std::clamp(alpha, 0.0f, 1.0f);
	}

	Standard::Standard(const float red, const float green, const float blue, const float alpha)
		: r(std::clamp(red, 0.0f, 1.0f))
		, g(std::clamp(green, 0.0f, 1.0f))
		, b(std::clamp(blue, 0.0f, 1.0f))
		, a(std::clamp(alpha, 0.0f, 1.0f))
	{
	}
	
	// materials
	const Standard Standard::MATERIAL_ALUMINUM			= Standard(0.912f, 0.914f, 0.920f); // metallic: 1.0
	const Standard Standard::MATERIAL_BLOOD				= Standard(0.644f, 0.003f, 0.005f);
	const Standard Standard::MATERIAL_BONE				= Standard(0.793f, 0.793f, 0.664f);
	const Standard Standard::MATERIAL_BRASS				= Standard(0.887f, 0.789f, 0.434f);
	const Standard Standard::MATERIAL_BRICK				= Standard(0.262f, 0.095f, 0.061f);
	const Standard Standard::MATERIAL_CHARCOAL			= Standard(0.020f, 0.020f, 0.020f);
	const Standard Standard::MATERIAL_CHOCOLATE			= Standard(0.162f, 0.091f, 0.060f);
	const Standard Standard::MATERIAL_CHROMIUM			= Standard(0.550f, 0.556f, 0.554f); // metallic: 1.0
	const Standard Standard::MATERIAL_COBALT			= Standard(0.662f, 0.655f, 0.634f);
	const Standard Standard::MATERIAL_CONCRETE			= Standard(0.510f, 0.510f, 0.510f);
	const Standard Standard::MATERIAL_COOKING_OIL		= Standard(0.738f, 0.687f, 0.091f);
	const Standard Standard::MATERIAL_COPPER			= Standard(0.926f, 0.721f, 0.504f);
	const Standard Standard::MATERIAL_DIAMOND			= Standard(1.000f, 1.000f, 1.000f);
	const Standard Standard::MATERIAL_EGG_SHELL			= Standard(0.610f, 0.624f, 0.631f);
	const Standard Standard::MATERIAL_EYE_CORNEA		= Standard(1.000f, 1.000f, 1.000f);
	const Standard Standard::MATERIAL_EYE_LENS			= Standard(1.000f, 1.000f, 1.000f);
	const Standard Standard::MATERIAL_EYE_SCLERA		= Standard(0.680f, 0.490f, 0.370f);
	const Standard Standard::MATERIAL_GLASS				= Standard(1.000f, 1.000f, 1.000f);
	const Standard Standard::MATERIAL_GOLD				= Standard(0.944f, 0.776f, 0.373f);
	const Standard Standard::MATERIAL_GRAY_CARD			= Standard(0.180f, 0.180f, 0.180f);
	const Standard Standard::MATERIAL_HONEY				= Standard(0.831f, 0.397f, 0.038f);
	const Standard Standard::MATERIAL_ICE				= Standard(1.000f, 1.000f, 1.000f);
	const Standard Standard::MATERIAL_IRON				= Standard(0.531f, 0.512f, 0.496f); // metallic: 1.0
	const Standard Standard::MATERIAL_KETCHUP			= Standard(0.164f, 0.006f, 0.002f);
	const Standard Standard::MATERIAL_LEAD				= Standard(0.632f, 0.626f, 0.641f);
	const Standard Standard::MATERIAL_MERCURY			= Standard(0.781f, 0.779f, 0.779f);
	const Standard Standard::MATERIAL_MILK				= Standard(0.604f, 0.584f, 0.497f);
	const Standard Standard::MATERIAL_NICKEL			= Standard(0.649f, 0.610f, 0.541f);
	const Standard Standard::MATERIAL_OFFICE_PAPER		= Standard(0.738f, 0.768f, 1.000f);
	const Standard Standard::MATERIAL_PLASTIC_PC		= Standard(1.000f, 1.000f, 1.000f); // specular: 0.640
	const Standard Standard::MATERIAL_PLASTIC_PET		= Standard(1.000f, 1.000f, 1.000f); // specular: 0.623
	const Standard Standard::MATERIAL_PLASTIC_ACRYLIC	= Standard(1.000f, 1.000f, 1.000f); // specular: 0.462
	const Standard Standard::MATERIAL_PLASTIC_PP		= Standard(1.000f, 1.000f, 1.000f); // specular: 0.487
	const Standard Standard::MATERIAL_PLASTIC_PVC		= Standard(1.000f, 1.000f, 1.000f); // specular: 0.550
	const Standard Standard::MATERIAL_PLATINUM			= Standard(0.679f, 0.642f, 0.588f);
	const Standard Standard::MATERIAL_SALT				= Standard(0.800f, 0.800f, 0.800f);
	const Standard Standard::MATERIAL_SAND				= Standard(0.440f, 0.386f, 0.231f);
	const Standard Standard::MATERIAL_SAPPHIRE			= Standard(0.670f, 0.764f, 0.855f);
	const Standard Standard::MATERIAL_SILVER			= Standard(0.962f, 0.949f, 0.922f);
	const Standard Standard::MATERIAL_SKIN_1			= Standard(0.847f, 0.638f, 0.552f);
	const Standard Standard::MATERIAL_SKIN_2			= Standard(0.799f, 0.485f, 0.347f);
	const Standard Standard::MATERIAL_SKIN_3			= Standard(0.600f, 0.310f, 0.220f);
	const Standard Standard::MATERIAL_SKIN_4			= Standard(0.430f, 0.200f, 0.130f);
	const Standard Standard::MATERIAL_SKIN_5			= Standard(0.360f, 0.160f, 0.080f);
	const Standard Standard::MATERIAL_SKIN_6			= Standard(0.090f, 0.050f, 0.020f);
	const Standard Standard::MATERIAL_SNOW				= Standard(0.810f, 0.810f, 0.810f);
	const Standard Standard::MATERIAL_TIRE				= Standard(0.023f, 0.023f, 0.023f); // metallic: 0.0, specular 0.5
	const Standard Standard::MATERIAL_TITANIUM			= Standard(0.616f, 0.582f, 0.544f);
	const Standard Standard::MATERIAL_TUNGSTEN			= Standard(0.925f, 0.835f, 0.757f);
	const Standard Standard::MATERIAL_VANADIUM			= Standard(0.945f, 0.894f, 0.780f);
	const Standard Standard::MATERIAL_WATER				= Standard(1.000f, 1.000f, 1.000f);
	const Standard Standard::MATERIAL_ZINC				= Standard(0.875f, 0.867f, 0.855f);
	
	// lights
	const Standard Standard::LIGHT_SKY_CLEAR				= Standard(15000);	// intensity: 20000  lx
	const Standard Standard::LIGHT_SKY_DAYLIGHT_OVERCAST	= Standard(6500);	// intensity: 2000   lx
	const Standard Standard::LIGHT_SKY_MOONLIGHT			= Standard(4000);	// intensity: 0.1    lx
	const Standard Standard::LIGHT_SKY_SUNRISE				= Standard(2000);
	const Standard Standard::LIGHT_CANDLE_FLAME				= Standard(1850);	// intensity: 13     lm
	const Standard Standard::LIGHT_DIRECT_SUNLIGHT			= Standard(5778);   // intensity: 120000 lx
	const Standard Standard::LIGHT_DIGITAL_DISPLAY			= Standard(6500);   // intensity: 200    cd/m2
	const Standard Standard::LIGHT_FLUORESCENT_TUBE_LIGHT	= Standard(5000);	// intensity: 1000   lm
	const Standard Standard::LIGHT_KEROSENE_LAMP			= Standard(1850);   // intensity: 50     lm
	const Standard Standard::LIGHT_LIGHT_BULB				= Standard(2700);   // intensity: 800    lm
	const Standard Standard::LIGHT_PHOTO_FLASH				= Standard(5500);   // intensity: 20000  lm
	
} // namespace SceneryEditorX

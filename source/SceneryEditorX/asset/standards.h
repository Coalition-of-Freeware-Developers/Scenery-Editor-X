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
 * standards.h
 * -------------------------------------------------------
 * Created: 21/03/2026
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @struct Standard
	 * @brief A struct that holds standard color and temperature values for materials and lights. 
	 * The RGB values are in the range [0, 1], and the alpha value is also in the range [0, 1] where 1 is fully opaque and 0 is fully transparent. 
	 * For light temperatures, the RGB values are derived from the color temperature in Kelvin using a standard conversion formula. 
	 * This struct provides a convenient way to access commonly used material colors and light colors/temperatures in the renderer.
	 */
	struct Standard
	{
		Standard() = default;
	    ~Standard() = default;

		/**
		 * @brief Copy constructor
		 * @param value The Standard object to copy
		 */
		Standard(const Standard& value) = default;

		/**
		 * @brief Constructor that initializes the Standard object with a color temperature in Kelvin.
		 * @param temperature_kelvin The color temperature in Kelvin.
		 * @param a The alpha value, default is 1.0f.
		 */
		Standard(const float temperature_kelvin, const float a = 1.0f);

		/**
		 * @brief Constructor that initializes the Standard object with RGBA values.
		 * @param r The red component of the color.
		 * @param g The green component of the color.
		 * @param b The blue component of the color.
		 * @param a The alpha value, default is 1.0f.
		 */
		Standard(const float r, const float g, const float b, const float a = 1.0f);

		/**
		 * @brief Equality operator
		 * @param rhs The Standard object to compare with
		 * @return True if the objects are equal, false otherwise
		 */
		bool operator==(const Standard& rhs) const
		{
			return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
		}

		/**
		 * @brief Inequality operator
		 * @param rhs The Standard object to compare with
		 * @return True if the objects are not equal, false otherwise
		 */
		bool operator!=(const Standard& rhs) const
		{
			return !(*this == rhs);
		}
		
		float r = 0.0f; // Red component of the color, in the range [0, 1]
		float g = 0.0f; // Green component of the color, in the range [0, 1]
		float b = 0.0f; // Blue component of the color, in the range [0, 1]
		float a = 0.0f; // Alpha component of the color, in the range [0, 1] where 1 is fully opaque and 0 is fully transparent

		/**
		 * @brief Returns a pointer to the underlying RGBA data.
		 * @return A pointer to the RGBA data.
		 */
		[[nodiscard]] const float* Data() const { return &r; }

#pragma region Custom Material Values

		static const Standard MATERIAL_ALUMINUM;
		static const Standard MATERIAL_BLOOD;
		static const Standard MATERIAL_BONE;
		static const Standard MATERIAL_BRASS;
		static const Standard MATERIAL_BRICK;
		static const Standard MATERIAL_CHARCOAL;
		static const Standard MATERIAL_CHOCOLATE;
		static const Standard MATERIAL_CHROMIUM;
		static const Standard MATERIAL_COBALT;
		static const Standard MATERIAL_CONCRETE;
		static const Standard MATERIAL_COOKING_OIL;
		static const Standard MATERIAL_COPPER;
		static const Standard MATERIAL_DIAMOND;
		static const Standard MATERIAL_EGG_SHELL;
		static const Standard MATERIAL_EYE_CORNEA;
		static const Standard MATERIAL_EYE_LENS;
		static const Standard MATERIAL_EYE_SCLERA;
		static const Standard MATERIAL_GLASS;
		static const Standard MATERIAL_GOLD;
		static const Standard MATERIAL_GRAY_CARD;
		static const Standard MATERIAL_HONEY;
		static const Standard MATERIAL_ICE;
		static const Standard MATERIAL_IRON;
		static const Standard MATERIAL_KETCHUP;
		static const Standard MATERIAL_LEAD;
		static const Standard MATERIAL_MERCURY;
		static const Standard MATERIAL_MILK;
		static const Standard MATERIAL_NICKEL;
		static const Standard MATERIAL_OFFICE_PAPER;
		static const Standard MATERIAL_PLASTIC_PC;
		static const Standard MATERIAL_PLASTIC_PET;
		static const Standard MATERIAL_PLASTIC_ACRYLIC;
		static const Standard MATERIAL_PLASTIC_PP;
		static const Standard MATERIAL_PLASTIC_PVC;
		static const Standard MATERIAL_PLATINUM;
		static const Standard MATERIAL_SALT;
		static const Standard MATERIAL_SAND;
		static const Standard MATERIAL_SAPPHIRE;
		static const Standard MATERIAL_SILVER;
		static const Standard MATERIAL_SKIN_1;
		static const Standard MATERIAL_SKIN_2;
		static const Standard MATERIAL_SKIN_3;
		static const Standard MATERIAL_SKIN_4;
		static const Standard MATERIAL_SKIN_5;
		static const Standard MATERIAL_SKIN_6;
		static const Standard MATERIAL_SNOW;
		static const Standard MATERIAL_TIRE;
		static const Standard MATERIAL_TITANIUM;
		static const Standard MATERIAL_TUNGSTEN;
		static const Standard MATERIAL_VANADIUM;
		static const Standard MATERIAL_WATER;
		static const Standard MATERIAL_ZINC;

#pragma endregion
#pragma region Custom Light Temperatures

		static const Standard LIGHT_SKY_CLEAR;
		static const Standard LIGHT_SKY_DAYLIGHT_OVERCAST;
		static const Standard LIGHT_SKY_MOONLIGHT;
		static const Standard LIGHT_SKY_SUNRISE;
		static const Standard LIGHT_CANDLE_FLAME;
		static const Standard LIGHT_DIGITAL_DISPLAY;
		static const Standard LIGHT_DIRECT_SUNLIGHT;
		static const Standard LIGHT_FLUORESCENT_TUBE_LIGHT;
		static const Standard LIGHT_KEROSENE_LAMP;
		static const Standard LIGHT_LIGHT_BULB;
		static const Standard LIGHT_PHOTO_FLASH;

#pragma endregion

	};

}

// -------------------------------------------------------

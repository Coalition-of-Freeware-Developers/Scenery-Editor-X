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
#include <cstdint>

#include <SceneryEditorX/core/identifiers/hash.h>
#include <SceneryEditorX/utils/math/math_utils.h>

/// -----------------------------------------------------

namespace SceneryEditorX::Utils
{
	/*
	struct ColorBase
	{
		float R, G, B, A;

		Color() : R(0.0f), G(0.0f), B(0.0f), A(1.0f) {}
		Color(float r, float g, float b, float a = 1.0f) : R(r), G(g), B(b), A(a) {}

		static Color Red() { return Color(1.0f, 0.0f, 0.0f); }
		static Color Green() { return Color(0.0f, 1.0f, 0.0f); }
		static Color Blue() { return Color(0.0f, 0.0f, 1.0f); }
		static Color White() { return Color(1.0f, 1.0f, 1.0f); }
		static Color Black() { return Color(0.0f, 0.0f, 0.0f); }
	};
	*/

	class Color
    {
    public:
        Color() : a(0), b(0), g(0), r(0) {}

        Color(const float r, const float g, const float b, const float a) : a(a), b(b), g(g), r(r) {}
        Color(const float r, const float g, const float b) : a(1), b(b), g(g), r(r) {}
        explicit Color(const Vec4 v) : a(v.w), b(v.z), g(v.y), r(v.x) {}
        explicit Color(const Vec3 v) : a(1), b(v.z), g(v.y), r(v.x) {}

		static Color RGBA8(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
        static Color RGBHex(uint32_t hex);
		static Color RGBAHex(uint32_t hex);
        static Color HSV(float h, float s, float v);
		static Color RGBA(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 255) { return RGBA8(r, g, b, a); }

        bool operator==(const Color& rhs) const { return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a; }
        bool operator!=(const Color& rhs) const { return operator==(rhs); }

        Color operator*(const int32_t value) const { return {value * r, value * g, value * b, value * a}; }
        Color operator*(const uint32_t value) const { return {value * r, value * g, value * b, value * a}; }
        Color operator*(const float value) const { return {value * r, value * g, value * b, value * a}; }

        Color operator*=(const int32_t value)
        {
            *this = *this * value;
            return *this;
        }

        Color operator*=(const uint32_t value)
        {
            *this = *this * value;
            return *this;
        }

        Color operator*=(const float value)
        {
            *this = *this * value;
            return *this;
        }

        Color operator/(const float value) const { return {r / value, g / value, b / value, a / value}; }

        Color operator/=(const float value)
        {
            *this = *this / value;
            return *this;
        }

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

        // - Preset Colors -

        static Color Red() { return {1, 0, 0, 1}; }
        static Color Green() { return {0, 1, 0, 1}; }
        static Color Blue() { return {0, 0, 1, 1}; }
        static Color Black() { return {0, 0, 0, 1}; }
        static Color White() { return {1, 1, 1, 1}; }
        static Color Yellow() { return {1, 1, 0, 1}; }
        static Color Clear() { return {0, 0, 0, 0}; }
        static Color Cyan() { return {0, 1, 1, 1}; }

        [[nodiscard]] uint32_t ToU32() const;
        [[nodiscard]] Vec4 ToVec4() const;
        [[nodiscard]] Vec3 ToHSV() const;

        static Color Lerp(const Color& from, const Color& to, float t)
		{
			return {
			    Math::Lerp(from.r, to.r, t),
			    Math::Lerp(from.g, to.g, t),
			    Math::Lerp(from.b, to.b, t),
			    Math::Lerp(from.a, to.a, t)
			};
		}

        [[nodiscard]] size_t GetHash() const
        {
            size_t hash = GetHash(r);
            CombineHash(hash, g);
            CombineHash(hash, b);
            CombineHash(hash, a);
            return hash;
        }

        [[nodiscard]] Color WithAlpha(float alpha) const { return {r, g, b, alpha}; }

        union
	    {
            struct { float a, b, g, r; };
            float rgba[4] = { 0, 0, 0, 0 };
        };
    };

}

/// -----------------------------------------------------

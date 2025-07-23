/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* math_utils.h - Mathematical utility functions
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/base.hpp>
#include <cmath>
#include <cstdint>

/// -------------------------------------------------------

namespace SceneryEditorX::Utils
{
    ///< Constants
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float HALF_PI = PI * 0.5f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;

    ///< Angle conversions
    //constexpr float ToRadians(const float degrees) { return degrees * DEG_TO_RAD; }
    //constexpr float ToDegrees(const float radians) { return radians * RAD_TO_DEG; }

    ///< Utility functions
    template<typename T>
    constexpr T Clamp(const T& value, const T& min, const T& max)
    {
        return (value < min) ? min : (value > max) ? max : value;
    }

    template<typename T>
    constexpr T Lerp(const T& a, const T& b, float t)
    {
        return a + t * (b - a);
    }

    ///< Comparison functions
    bool IsEqual(float a, float b, float epsilon = 1e-6f);
    bool IsZero(float value, float epsilon = 1e-6f);

    ///< Vector operations (complement to transforms.h)
    float Distance(const Vec3& a, const Vec3& b);
    Vec3 Normalize(const Vec3& vector);
    float Dot(const Vec3& a, const Vec3& b);
    Vec3 Cross(const Vec3& a, const Vec3& b);

    /// ---------------------------------------------------------

	class Math
	{
	public:
	    Math() = delete;
        ~Math() = delete;

        static constexpr float PI = M_PI;

        static uint16_t ToFloat16(float floatValue)
        {
            float* ptr = &floatValue;
            const unsigned int fltInt32 = *reinterpret_cast<uint32_t *>(ptr);

            uint16_t fltInt16 = (fltInt32 >> 31) << 5;
            uint16_t tmp = fltInt32 >> 23 & 0xff;
            tmp = tmp - 0x70 & static_cast<unsigned int>(0x70 - tmp >> 4) >> 27;
            fltInt16 = (fltInt16 | tmp) << 10;
            fltInt16 |= fltInt32 >> 13 & 0x3ff;

            return fltInt16;
        }

        ///< Needs to be tested!
        static float ToFloat32(uint16_t float16);
        static bool ApproxEquals(const float a, const float b) { return fpclassify(a - b) == FP_ZERO; }
    	static bool ApproxEquals(const double a, const double b) { return fpclassify(a - b) == FP_ZERO; }
    	static bool ApproxEquals(const int32_t a, const int32_t b) { return a == b; }

		///< TODO: Implement infinity operations later
    	//static constexpr float Infinity() { return NumericLimits<float>::Infinity(); }

        static float ToDegrees(float radians) { return RAD_TO_DEG(radians); }
        static float ToRadians(float degrees) { return DEG_TO_RAD(degrees); }
        static float Sin(const float radians) { return sin(radians); }
        static float ASin(const float sine) { return asin(sine); }
        static float Cos(const float radians) { return cos(radians); }
        static float ACos(const float cosine) { return acos(cosine); }
        static float Tan(const float radians) { return tan(radians); }
        static float ATan(const float tangent) { return atan(tangent); }
        static float Abs(const float value) { return abs(value); }
        static double Abs(const double value) { return abs(value); }
        static int32_t Abs(const int32_t value) { return abs(value); }
        static int64_t Abs(const int64_t value) { return abs(value); }
        static float Sqrt(const float value) { return sqrt(value); }

    	template<typename T1, typename T2>
        static auto Pow(T1 base, T2 power) { return pow(base, power); }

    	template<typename T>
        static T Min(std::initializer_list<T> list)
    	{
    	    auto min = std::numeric_limits<T>::max();

    	    for (auto entry : list)
    	    {
    	        if (entry < min)
    	            min = entry;
    	    }

    	    return min;
    	}

    	template<typename T>
        static T Max(std::initializer_list<T> list)
    	{
    	    auto max = std::numeric_limits<T>::min();
    	    for (auto entry : list)
    	    {
    	        if (entry > max)
    	            max = entry;
    	    }

    	    return max;
    	}

    	static float Round(const float value)  { return std::round(value); }
    	static double Round(const double value) { return std::round(value); }
    	static int32_t RoundToInt(const float value) { return std::lround(value); }
    	static int64_t RoundToInt64(const float value) { return std::llround(value); }
    	static int32_t RoundToInt(const double value) { return std::lround(value); }
    	static int64_t RoundToInt64(const double value) { return std::llround(value); }

    	template<typename T>
        static T Min(T a, T b)
    	{
    	    return a < b ? a : b;
    	}

    	template<typename T>
        static T Max(T a, T b)
    	{
    	    return a > b ? a : b;
    	}

    	template<typename T>
        static T Clamp(T value, T min, T max)
    	{
    	    if (min > max)
    	        std::swap(min, max);
		    return Min(Max(value, min), max);
    	}

    	template<typename T>
        static T Clamp01(T value) { return Clamp<T>(value, 0, 1); }

    	static float Lerp(const float from, const float to, const float t) { return from * (1 - Clamp01(t)) + to * Clamp01(t); }
    	static float LerpUnclamped(const float from, const float to, const float t) { return from * (1 - t) + to * (t); }

    };

    inline float Math::ToFloat32(const uint16_t float16)
    {

        uint32_t t1 = float16 & 0x7fffu;       // Non-sign bits
        uint32_t t2 = float16 & 0x8000u;       // Sign bit
        const uint32_t t3 = float16 & 0x7c00u; // Exponent

        t1 <<= 13u;            // Align mantissa on MSB
        t2 <<= 16u;            // Shift sign a bit into position
        t1 += 0x38000000;      // Adjust bias
        t1 = t3 == 0 ? 0 : t1; // Denormals-as-zero
        t1 |= t2;              // Re-insert sign bit

        return *reinterpret_cast<float *>(&t1);
    }

	template<typename T>
    bool IsNan(T value)
	{
		return std::isnan(value);
	}


}

/// -------------------------------------------------------

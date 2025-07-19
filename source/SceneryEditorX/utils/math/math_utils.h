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
    // Constants
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float HALF_PI = PI * 0.5f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;

    // Angle conversions
    constexpr float ToRadians(float degrees) { return degrees * DEG_TO_RAD; }
    constexpr float ToDegrees(float radians) { return radians * RAD_TO_DEG; }

    // Utility functions
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

    // Comparison functions
    bool IsEqual(float a, float b, float epsilon = 1e-6f);
    bool IsZero(float value, float epsilon = 1e-6f);

    // Vector operations (complement to transforms.h)
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
            unsigned int fltInt32 = *((uint32_t*)ptr);

            uint16_t fltInt16 = (fltInt32 >> 31) << 5;
            uint16_t tmp = (fltInt32 >> 23) & 0xff;
            tmp = (tmp - 0x70) & ((unsigned int)((int)(0x70 - tmp) >> 4) >> 27);
            fltInt16 = (fltInt16 | tmp) << 10;
            fltInt16 |= (fltInt32 >> 13) & 0x3ff;

            return fltInt16;
        }

        // Needs to be tested!
        static float ToFloat32(uint16_t float16)
        {
            uint32_t t1;
            uint32_t t2;
            uint32_t t3;

            t1 = float16 & 0x7fffu;                       // Non-sign bits
            t2 = float16 & 0x8000u;                       // Sign bit
            t3 = float16 & 0x7c00u;                       // Exponent

            t1 <<= 13u;                              // Align mantissa on MSB
            t2 <<= 16u;                              // Shift sign bit into position
            t1 += 0x38000000;                       // Adjust bias
            t1 = (t3 == 0 ? 0 : t1);                // Denormals-as-zero
            t1 |= t2;                               // Re-insert sign bit

            return *(float*)(&t1);
        }

    	static bool ApproxEquals(float a, float b) { return fpclassify(a - b) == FP_ZERO; }
    	static bool ApproxEquals(double a, double b) { return fpclassify(a - b) == FP_ZERO; }
    	static bool ApproxEquals(int32_t a, int32_t b) { return a == b; }

    	static constexpr float Infinity() { return NumericLimits<float>::Infinity(); }

    	inline static float ToDegrees(float radians) { return TO_DEGREES(radians); }
    	inline static float ToRadians(float degrees) { return TO_RADIANS(degrees); }
    	inline static float Sin(float radians) { return sin(radians); }
    	inline static float ASin(float sine) { return asin(sine); }
    	inline static float Cos(float radians) { return cos(radians); }
    	inline static float ACos(float cosine) { return acos(cosine); }
    	inline static float Tan(float radians) { return tan(radians); }
    	inline static float ATan(float tangent) { return atan(tangent); }
    	inline static float Abs(float value) { return abs(value); }
    	inline static double Abs(double value) { return abs(value); }
    	inline static int32_t Abs(int32_t value) { return abs(value); }
    	inline static int64_t Abs(int64_t value) { return abs(value); }
    	inline static float Sqrt(float value) { return sqrt(value); }

    	template<typename T1, typename T2>
    	inline static auto Pow(T1 base, T2 power) { return pow(base, power); }

    	template<typename T>
    	inline static T Min(std::initializer_list<T> list)
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
    	inline static T Max(std::initializer_list<T> list)
    	{
    	    auto max = std::numeric_limits<T>::min();
    	    for (auto entry : list)
    	    {
    	        if (entry > max)
    	            max = entry;
    	    }

    	    return max;
    	}

    	static float Round(float value)  { return std::round(value); }
    	static double Round(double value) { return std::round(value); }
    	static int32_t RoundToInt(float value) { return std::lround(value); }
    	static int64_t RoundToInt64(float value) { return std::llround(value); }
    	static int32_t RoundToInt(double value) { return std::lround(value); }
    	static int64_t RoundToInt64(double value) { return std::llround(value); }

    	template<typename T>
    	inline static T Min(T a, T b)
    	{
    	    return a < b ? a : b;
    	}

    	template<typename T>
    	inline static T Max(T a, T b)
    	{
    	    return a > b ? a : b;
    	}

    	template<typename T>
    	inline static T Clamp(T value, T min, T max)
    	{
    	    if (min > max)
    	        std::swap(min, max);
		    return Min(Max(value, min), max);
    	}

    	template<typename T>
    	inline static T Clamp01(T value) { return Clamp<T>(value, 0, 1); }

    	inline static float Lerp(float from, float to, float t) { return from * (1 - Clamp01(t)) + to * Clamp01(t); }
    	inline static float LerpUnclamped(float from, float to, float t) { return from * (1 - t) + to * (t); }

    };

	template<typename T>
	inline bool IsNan(T value)
	{
		return std::isnan(value);
	}


}

/// -------------------------------------------------------

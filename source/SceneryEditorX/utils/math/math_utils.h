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
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/utils/math/vector.h>
#include <SceneryEditorX/utils/math/constants.h>
#include <glm/gtc/quaternion.hpp>

/// -------------------------------------------------------

namespace SceneryEditorX::Utils
{

    // Angle conversion helpers
    constexpr float ToRadians(float degrees) { return degrees * DEG_TO_RAD; }
    constexpr float ToDegrees(float radians) { return radians * RAD_TO_DEG; }

    // Floating-point helpers
    bool IsEqual(float a, float b, float epsilon = 1e-6f);
    bool IsZero(float value, float epsilon = 1e-6f);

    // Vector operations (Vec3)
    float Distance(const Vec3& a, const Vec3& b);
    Vec3 Normalize(const Vec3& vector);
    float Dot(const Vec3& a, const Vec3& b);
    Vec3 Cross(const Vec3& a, const Vec3& b);

    // Rounding helpers
    template <typename T>
    constexpr T RoundDown(T x, T fac)
    {
        if constexpr (std::is_floating_point_v<T>)
        {
            return fac == static_cast<T>(0)
                ? x
                : std::floor(x / fac) * fac;
        }
        else if constexpr (std::is_integral_v<T>)
        {
            return fac == static_cast<T>(0)
                ? x
                : static_cast<T>(x - (x % fac));
        }
        else
        {
            return x;
        }
    }

    template <typename T>
    constexpr T RoundUp(T x, T fac)
    {
        if constexpr (std::is_floating_point_v<T>)
        {
            return fac == static_cast<T>(0)
                ? x
                : std::ceil(x / fac) * fac;
        }
        else if constexpr (std::is_integral_v<T>)
        {
            if (fac == static_cast<T>(0))
                return x;
            return static_cast<T>(((x + (fac - 1)) / fac) * fac);
        }
        else
        {
            return x;
        }
    }

    // Compatibility wrappers and common math helpers
    class Math
    {
    public:
        Math() = delete; ~Math() = delete;

        // TRS wrappers (glm::quat compatibility)
        static bool DecomposeTransform(const Mat4 &mat, Vec3 &translation, glm::quat &rotation, Vec3 &scale);
        static Mat4 ComposeTransform(const Vec3 &translation, const glm::quat &rotation, const Vec3 &scale);

        // Absolute value overloads
        static float   Abs(float v)   { return std::fabs(v); }
        static double  Abs(double v)  { return std::fabs(v); }
        static int32_t Abs(int32_t v) { return v < 0 ? -v : v; }
        static int64_t Abs(int64_t v) { return v < 0 ? -v : v; }

        // Root and power
        static float Sqrt(float v) { return std::sqrt(v); }
        template<typename T1, typename T2>
        static auto Pow(T1 base, T2 power) { return std::pow(base, power); }

        // Min/Max over initializer_list
        template<typename T>
        static T Min(std::initializer_list<T> list)
        {
            auto it = list.begin();
            T minVal = *it;
            for (++it; it != list.end(); ++it) minVal = (*it < minVal) ? *it : minVal;
            return minVal;
        }

        template<typename T>
        static T Max(std::initializer_list<T> list)
        {
            auto it = list.begin();
            T maxVal = *it;
            for (++it; it != list.end(); ++it) maxVal = (*it > maxVal) ? *it : maxVal;
            return maxVal;
        }

        // Binary Min/Max
        template<typename T>
        static T Min(T a, T b) { return a < b ? a : b; }

        template<typename T>
        static T Max(T a, T b) { return a > b ? a : b; }

        // Clamping
        template<typename T>
        static T Clamp(T value, T min, T max)
        {
            if (min > max) std::swap(min, max);
            return Min(Max(value, min), max);
        }

        template<typename T>
        static T Clamp01(T value) { return Clamp<T>(value, 0, 1); }

        // Rounding
        static float   Round(float v)   { return std::round(v); }
        static double  Round(double v)  { return std::round(v); }
        static int32_t RoundToInt(float v)  { return static_cast<int32_t>(std::lround(v)); }
        static int64_t RoundToInt64(float v) { return static_cast<int64_t>(std::llround(v)); }
        static int32_t RoundToInt(double v) { return static_cast<int32_t>(std::lround(v)); }
        static int64_t RoundToInt64(double v){ return static_cast<int64_t>(std::llround(v)); }

        // Interpolation
        static float Lerp(float from, float to, float t)
        {
            const float tt = Clamp01(t);
            return from * (1.0f - tt) + to * tt;
        }
        static float LerpUnclamped(float from, float to, float t)
        {
            return from * (1.0f - t) + to * t;
        }

        // Float16 -> Float conversion
        static float ToFloat32(uint16_t float16)
        {
            uint32_t t1 = float16 & 0x7fffu;        // Non-sign bits
            uint32_t t2 = float16 & 0x8000u;        // Sign bit
            const uint32_t t3 = float16 & 0x7c00u;  // Exponent

            t1 <<= 13u;                             // Align mantissa on MSB
            t2 <<= 16u;                             // Shift sign into position
            t1 += 0x38000000;                       // Adjust bias
            t1 = t3 == 0 ? 0 : t1;                  // Denormals-as-zero
            t1 |= t2;                               // Re-insert sign bit
            return *reinterpret_cast<float *>(&t1);
        }
    };

    // NaN check
    template<typename T>
    inline bool IsNan(T value)
    {
        if constexpr (std::is_floating_point_v<T>)
        {
            return std::isnan(value);
        }
        else
        {
            return false;
        }
    }

}

/// -------------------------------------------------------

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

/// -------------------------------------------------------

namespace SceneryEditorX::Utils
{
	///////////////////////////////////////////////////////////
	///					Mathematical Constants				///
	///////////////////////////////////////////////////////////

    constexpr float PI = 3.14159265358979323846f;        ///< Pi constant (π)
    constexpr float TWO_PI = 2.0f * PI;                  ///< Two times pi (2π)
    constexpr float HALF_PI = PI * 0.5f;                 ///< Half pi (π/2)
    constexpr float DEG_TO_RAD = PI / 180.0f;            ///< Degrees to radians conversion factor
    constexpr float RAD_TO_DEG = 180.0f / PI;            ///< Radians to degrees conversion factor
	
    ///////////////////////////////////////////////////////////
    ///				Angle Conversion Functions				///
    ///////////////////////////////////////////////////////////

    constexpr float ToRadians(const float degrees)
    {
        return degrees * DEG_TO_RAD;
    }
    constexpr float ToDegrees(const float radians)
    {
        return radians * RAD_TO_DEG;
    }

    ///////////////////////////////////////////////////////////
    ///				Generic Utility Functions				///
    ///////////////////////////////////////////////////////////

    /**
     * @brief Clamps a value between minimum and maximum bounds
     *
     * @tparam T Type of the values (must support comparison operators)
     * @param value The value to clamp
     * @param min The minimum allowed value
     * @param max The maximum allowed value
     * @return The clamped value within [min, max] range
     */
    template<typename T>
    constexpr T Clamp(const T& value, const T& min, const T& max)
    {
        return (value < min) ? min : (value > max) ? max : value;
    }

    /**
     * @brief Linear interpolation between two values
     *
     * @tparam T Type of the values to interpolate (must support arithmetic operations)
     * @param a Starting value (t=0.0)
     * @param b Ending value (t=1.0)
     * @param t Interpolation parameter [0.0, 1.0] (values outside range are valid)
     * @return Interpolated value
     *
     * @note t is not clamped, allowing extrapolation beyond the [a, b] range
     */
    template<typename T>
    constexpr T Lerp(const T& a, const T& b, float t)
    {
        return a + t * (b - a);
    }

    ///////////////////////////////////////////////////////////
    ///			Floating-Point Comparison Functions			///
    ///////////////////////////////////////////////////////////

    /**
     * @brief Tests if two floating-point values are approximately equal
     *
     * @param a First value to compare
     * @param b Second value to compare
     * @param epsilon Tolerance for comparison (default: 1e-6f)
     * @return true if |a - b| <= epsilon, false otherwise
     */
    bool IsEqual(float a, float b, float epsilon = 1e-6f);

    /**
     * @brief Tests if a floating-point value is approximately zero
     *
     * @param value Value to test
     * @param epsilon Tolerance for comparison (default: 1e-6f)
     * @return true if |value| <= epsilon, false otherwise
     */
    bool IsZero(float value, float epsilon = 1e-6f);

	
    ///////////////////////////////////////////////////////////
    ///					 Vector Operations					///
    ///////////////////////////////////////////////////////////
    /// These functions complement the transforms.h vector operations

    /**
     * @brief Calculates Euclidean distance between two 3D points
     *
     * @param a First point
     * @param b Second point
     * @return Distance between the points
     */
    float Distance(const Vec3& a, const Vec3& b);

    /**
     * @brief Normalizes a 3D vector to unit length
     *
     * @param vector Vector to normalize
     * @return Normalized vector with length 1.0, or zero vector if input has zero length
     *
     * @warning Returns zero vector if input vector has zero or near-zero length
     */
    Vec3 Normalize(const Vec3& vector);

    /**
     * @brief Calculates dot product of two 3D vectors
     *
     * @param a First vector
     * @param b Second vector
     * @return Dot product (scalar value)
     */
    float Dot(const Vec3& a, const Vec3& b);

    /**
     * @brief Calculates cross product of two 3D vectors
     *
     * @param a First vector
     * @param b Second vector
     * @return Cross product vector perpendicular to both input vectors
     */
    Vec3 Cross(const Vec3& a, const Vec3& b);

    /**
	 * @brief Rounds a value down to the nearest multiple of the specified factor.
	 * 
	 * This function rounds down the input value to the nearest multiple of the given factor
	 * using integer division followed by multiplication. For example, RoundDown(17, 5) = 15.
	 * 
	 * @tparam T The numeric type of both the value and factor (int, float, etc.)
	 * @param x The value to round down
	 * @param fac The factor to which x should be rounded (must be non-zero)
	 * @return T The value of x rounded down to the nearest multiple of fac
	 */
    template <typename T>
    T RoundDown(T x, T fac)
    {
        return x / fac * fac;
    }

    /**
	 * @brief Rounds a value up to the nearest multiple of the specified factor.
	 * 
	 * This function rounds up the input value to the nearest multiple of the given factor.
	 * It adds (fac - 1) to x before rounding down to ensure rounding up.
	 * For example, RoundUp(17, 5) = 20.
	 * 
	 * @tparam T The numeric type of both the value and factor (int, float, etc.)
	 * @param x The value to round up
	 * @param fac The factor to which x should be rounded (must be non-zero)
	 * @return T The value of x rounded up to the nearest multiple of fac
	 */
    template <typename T>
    T RoundUp(T x, T fac)
    {
        return RoundDown(x + fac - 1, fac);
    }

    /// ---------------------------------------------------------

	/**
	 * @brief Comprehensive mathematical utility class with static methods
	 *
	 * The Math class provides a centralized collection of mathematical functions
	 * commonly used in 3D graphics, game development, and scientific computing.
	 * All methods are static and the class cannot be instantiated.
	 *
	 * Features include:
	 * - Trigonometric functions with consistent naming
	 * - Type-safe numeric conversions
	 * - Template-based min/max operations
	 * - Half-precision float support for GPU optimization
	 * - Interpolation and clamping utilities
	 *
	 * @note This class is designed as a static utility class and cannot be instantiated
	 */
	class Math
	{
	public:
		/**
		 * @brief Deleted constructor - this class is not meant to be instantiated
		 */
	    Math() = delete;

	    /**
	     * @brief Deleted destructor - this class is not meant to be instantiated
	     */
        ~Math() = delete;

		/**
		 * @brief Decomposes a transformation matrix into translation, rotation, and scale components
		 */
        void DecomposeTransform(const Mat4 mat, Vec3 vec, glm::quat qua, Vec3 vec3);

        /**
		 * @brief Mathematical constant Pi (π)
		 *
		 * High-precision value of π for mathematical calculations.
		 */
        static constexpr float PI = M_PI;

	    ///////////////////////////////////////////////////////////
        ///			  Half-Precision Float Conversion			///
        ///////////////////////////////////////////////////////////

        /**
         * @brief Converts 32-bit float to 16-bit half-precision float
         *
         * Performs IEEE 754 half-precision conversion, useful for GPU memory
         * optimization and shader interoperability. The conversion uses bit
         * manipulation for optimal performance.
         *
         * @param floatValue 32-bit float value to convert
         * @return 16-bit half-precision representation
         *
         * @note Precision is lost in conversion. Suitable for graphics applications
         *       where memory bandwidth is more important than precision.
         */
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

        /**
         * @brief Converts 16-bit half-precision float to 32-bit float
         *
         * @param float16 16-bit half-precision value
         * @return 32-bit float representation
         *
         * @warning This function needs testing before production use
         */
        static float ToFloat32(uint16_t float16);

	    ///////////////////////////////////////////////////////////
        ///				Approximate Equality Testing			///
        ///////////////////////////////////////////////////////////

        /**
         * @brief Tests if two float values are approximately equal
         *
         * Uses floating-point classification to determine if the difference
         * between two values is effectively zero, accounting for floating-point
         * precision limitations.
         *
         * @param a First value
         * @param b Second value
         * @return true if values are approximately equal
         */
        static bool ApproxEquals(const float a, const float b) { return fpclassify(a - b) == FP_ZERO; }

        /**
         * @brief Tests if two double values are approximately equal
         *
         * @param a First value
         * @param b Second value
         * @return true if values are approximately equal
         */
    	static bool ApproxEquals(const double a, const double b) { return fpclassify(a - b) == FP_ZERO; }

    	/**
    	 * @brief Tests if two integer values are equal
    	 *
    	 * @param a First value
    	 * @param b Second value
    	 * @return true if values are exactly equal
    	 */
    	static bool ApproxEquals(const int32_t a, const int32_t b) { return a == b; }

		/// TODO: Implement infinity operations later
    	//static constexpr float Infinity() { return NumericLimits<float>::Infinity(); }

	    ///////////////////////////////////////////////////////////
        ///					Angle Conversions					///
        ///////////////////////////////////////////////////////////

        /**
         * @brief Converts radians to degrees
         *
         * @param radians Angle in radians
         * @return Angle in degrees
         */
        /*static float ToDegrees(float radians)
        {
            return RAD_TO_DEG(radians);
        }*/

        /**
         * @brief Converts degrees to radians
         *
         * @param degrees Angle in degrees
         * @return Angle in radians
         */
        /*static float ToRadians(float degrees)
        {
            return DEG_TO_RAD(degrees);
        }*/

	    ///////////////////////////////////////////////////////////
        ///					Trigonometric Functions				///
        ///////////////////////////////////////////////////////////

        /**
         * @brief Calculates sine of an angle
         *
         * @param radians Angle in radians
         * @return Sine value [-1.0, 1.0]
         */
        static float Sin(const float radians) { return sin(radians); }

        /**
         * @brief Calculates arcsine (inverse sine) of a value
         *
         * @param sine Sine value [-1.0, 1.0]
         * @return Angle in radians [-π/2, π/2]
         *
         * @warning Input must be in range [-1.0, 1.0] or result is undefined
         */
        static float ASin(const float sine) { return asin(sine); }

        /**
         * @brief Calculates cosine of an angle
         *
         * @param radians Angle in radians
         * @return Cosine value [-1.0, 1.0]
         */
        static float Cos(const float radians) { return cos(radians); }

        /**
         * @brief Calculates arccosine (inverse cosine) of a value
         *
         * @param cosine Cosine value [-1.0, 1.0]
         * @return Angle in radians [0, π]
         *
         * @warning Input must be in range [-1.0, 1.0] or result is undefined
         */
        static float ACos(const float cosine) { return acos(cosine); }

        /**
         * @brief Calculates tangent of an angle
         *
         * @param radians Angle in radians
         * @return Tangent value
         *
         * @warning Result is undefined for angles where cosine = 0 (e.g., π/2, 3π/2)
         */
        static float Tan(const float radians) { return tan(radians); }

        /**
         * @brief Calculates arctangent (inverse tangent) of a value
         *
         * @param tangent Tangent value
         * @return Angle in radians [-π/2, π/2]
         */
        static float ATan(const float tangent) { return atan(tangent); }
		
	    ///////////////////////////////////////////////////////////
        ///				 Absolute Value Functions				///
        ///////////////////////////////////////////////////////////

        /**
         * @brief Calculates absolute value of a float
         *
         * @param value Input value
         * @return Non-negative absolute value
         */
        static float Abs(const float value) { return abs(value); }

        /**
         * @brief Calculates absolute value of a double
         *
         * @param value Input value
         * @return Non-negative absolute value
         */
        static double Abs(const double value) { return abs(value); }

        /**
         * @brief Calculates absolute value of a 32-bit integer
         *
         * @param value Input value
         * @return Non-negative absolute value
         */
        static int32_t Abs(const int32_t value) { return abs(value); }

        /**
         * @brief Calculates absolute value of a 64-bit integer
         *
         * @param value Input value
         * @return Non-negative absolute value
         */
        static int64_t Abs(const int64_t value) { return abs(value); }

	    ///////////////////////////////////////////////////////////
        ///				 Root and Power Functions				///
        ///////////////////////////////////////////////////////////

        /**
         * @brief Calculates square root of a value
         *
         * @param value Input value (must be non-negative)
         * @return Square root of the input
         *
         * @warning Input must be non-negative or result is undefined
         */
        static float Sqrt(const float value) { return sqrt(value); }

        /**
         * @brief Calculates power of a base raised to an exponent
         *
         * @tparam T1 Type of the base value
         * @tparam T2 Type of the exponent value
         * @param base Base value
         * @param power Exponent value
         * @return base^power
         */
    	template<typename T1, typename T2>
        static auto Pow(T1 base, T2 power)
        {
            return pow(base, power);
        }

	    ///////////////////////////////////////////////////////////
        ///		Min/Max Functions with Initializer Lists		///
        ///////////////////////////////////////////////////////////

        /**
         * @brief Finds minimum value from an initializer list
         *
         * @tparam T Type of values (must support comparison operators)
         * @param list Initializer list of values to compare
         * @return Minimum value from the list
         *
         * @example
         * float min = Math::Min({1.5f, 2.3f, 0.8f, 3.1f}); // Returns 0.8f
         */
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

        /**
         * @brief Finds maximum value from an initializer list
         *
         * @tparam T Type of values (must support comparison operators)
         * @param list Initializer list of values to compare
         * @return Maximum value from the list
         *
         * @example
         * float max = Math::Max({1.5f, 2.3f, 0.8f, 3.1f}); // Returns 3.1f
         */
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

	    ///////////////////////////////////////////////////////////
        ///					Rounding Functions					///
        ///////////////////////////////////////////////////////////

        /**
         * @brief Rounds a float to the nearest integer value
         *
         * @param value Float value to round
         * @return Rounded float value
         */
    	static float Round(const float value) { return std::round(value); }

    	/**
    	 * @brief Rounds a double to the nearest integer value
    	 *
    	 * @param value Double value to round
    	 * @return Rounded double value
    	 */
    	static double Round(const double value) { return std::round(value); }

    	/**
    	 * @brief Rounds a float to the nearest 32-bit integer
    	 *
    	 * @param value Float value to round
    	 * @return Rounded 32-bit integer
    	 */
    	static int32_t RoundToInt(const float value) { return std::lround(value); }

    	/**
    	 * @brief Rounds a float to the nearest 64-bit integer
    	 *
    	 * @param value Float value to round
    	 * @return Rounded 64-bit integer
    	 */
    	static int64_t RoundToInt64(const float value) { return std::llround(value); }

    	/**
    	 * @brief Rounds a double to the nearest 32-bit integer
    	 *
    	 * @param value Double value to round
    	 * @return Rounded 32-bit integer
    	 */
    	static int32_t RoundToInt(const double value) { return std::lround(value); }

    	/**
    	 * @brief Rounds a double to the nearest 64-bit integer
    	 *
    	 * @param value Double value to round
    	 * @return Rounded 64-bit integer
    	 */
    	static int64_t RoundToInt64(const double value) { return std::llround(value); }

	    ///////////////////////////////////////////////////////////
        ///				 Binary Min/Max Functions				///
        ///////////////////////////////////////////////////////////

        /**
         * @brief Returns the smaller of two values
         *
         * @tparam T Type of values (must support comparison operators)
         * @param a First value
         * @param b Second value
         * @return The smaller value
         */
    	template<typename T>
        static T Min(T a, T b)
    	{
    	    return a < b ? a : b;
    	}

        /**
         * @brief Returns the larger of two values
         *
         * @tparam T Type of values (must support comparison operators)
         * @param a First value
         * @param b Second value
         * @return The larger value
         */
    	template<typename T>
        static T Max(T a, T b)
    	{
    	    return a > b ? a : b;
    	}

	    ///////////////////////////////////////////////////////////
        ///					 Clamping Functions					///
        ///////////////////////////////////////////////////////////

        /**
         * @brief Clamps a value between minimum and maximum bounds
         *
         * @tparam T Type of values (must support comparison operators)
         * @param value Value to clamp
         * @param min Minimum bound
         * @param max Maximum bound
         * @return Clamped value within [min, max] range
         *
         * @note If min > max, the values are automatically swapped
         */
    	template<typename T>
        static T Clamp(T value, T min, T max)
    	{
    	    if (min > max)
    	        std::swap(min, max);
		    return Min(Max(value, min), max);
    	}

        /**
         * @brief Clamps a value between 0 and 1
         *
         * @tparam T Type of value (must support comparison operators)
         * @param value Value to clamp
         * @return Clamped value within [0, 1] range
         */
    	template<typename T>
        static T Clamp01(T value)
        {
            return Clamp<T>(value, 0, 1);
        }

	    ///////////////////////////////////////////////////////////
        ///				  Interpolation Functions				///
        ///////////////////////////////////////////////////////////

        /**
         * @brief Linear interpolation with automatic clamping
         *
         * Performs linear interpolation between two values with the interpolation
         * parameter automatically clamped to [0, 1] range.
         *
         * @param from Starting value (t=0.0)
         * @param to Ending value (t=1.0)
         * @param t Interpolation parameter (automatically clamped to [0, 1])
         * @return Interpolated value
         */
    	static float Lerp(const float from, const float to, const float t)
        {
            return from * (1 - Clamp01(t)) + to * Clamp01(t);
        }

    	/**
    	 * @brief Linear interpolation without clamping
    	 *
    	 * Performs linear interpolation between two values without clamping the
    	 * interpolation parameter, allowing extrapolation beyond the [from, to] range.
    	 *
    	 * @param from Starting value (t=0.0)
    	 * @param to Ending value (t=1.0)
    	 * @param t Interpolation parameter (not clamped, allows extrapolation)
    	 * @return Interpolated/extrapolated value
    	 */
    	static float LerpUnclamped(const float from, const float to, const float t)
        {
            return from * (1 - t) + to * (t);
        }

    };

    /**
     * @brief Converts a 16-bit half-precision float to a 32-bit float
     *
     * This function performs the conversion from IEEE 754 half-precision format
     * to standard 32-bit float format. It handles denormalized values and
     * preserves the sign bit, exponent, and mantissa correctly.
     *
     * @param float16 16-bit half-precision float value to convert
     * @return 32-bit float representation of the input
     */
    inline float Math::ToFloat32(const uint16_t float16)
    {

        uint32_t t1 = float16 & 0x7fffu;		/// Non-sign bits
        uint32_t t2 = float16 & 0x8000u;		/// Sign bit
        const uint32_t t3 = float16 & 0x7c00u;	/// Exponent

        t1 <<= 13u;								/// Align mantissa on MSB
        t2 <<= 16u;								/// Shift sign a bit into position
        t1 += 0x38000000;						/// Adjust bias
        t1 = t3 == 0 ? 0 : t1;					/// Denormals-as-zero
        t1 |= t2;								/// Re-insert sign bit

        return *reinterpret_cast<float *>(&t1);
    }

	/**
	 * @brief Tests if a numeric value is Not-a-Number (NaN)
	 *
	 * @tparam T Numeric type (must support std::isnan)
	 * @param value Value to test
	 * @return true if value is NaN, false otherwise
	 *
	 * @note Useful for validating floating-point computation results
	 */
	template<typename T>
    bool IsNan(T value)
	{
		return std::isnan(value);
	}


}

/// -------------------------------------------------------

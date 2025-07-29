/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vector.h
* -------------------------------------------------------
* Created: 15/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/identifiers/hash.h>
#include <SceneryEditorX/utils/math/math_utils.h>

/// -------------------------------------------------------

namespace SceneryEditorX::Utils
{

    /*
    template<typename T>
    class TVector2;

    template<typename T>
    class TVector3;

    template<typename T>
    class TVector4;

    using Vec2 = TVector2<float>;
    using Vec3 = TVector3<float>;
    using Vec4 = TVector4<float>;

    using Vec2i = TVector2<int32_t>;
    using Vec3i = TVector3<int32_t>;
    using Vec4i = TVector4<int32_t>;

	using Range = Vec2;
	using RangeInt = Vec2i;
	*/

    /**
     * @brief Checks if two floating-point numbers are approximately equal.
     *
     * This function compares two floating-point numbers and returns true if their absolute difference is less than a small epsilon value.
     * This is useful for comparing floating-point numbers where precision errors may occur.
     *
     * @param lhs
     * @param rhs 
     * @return Returns true if the two numbers are approximately equal, false otherwise.
     */
    /*
    inline bool ApproxEquals(const float lhs, const float rhs)
    {
        return std::abs(lhs - rhs) < std::numeric_limits<float>::epsilon();
    }
    */

    /**
     * @brief Represents a 2D vector or a point in 2D space.
     * @tparam T 
     */
    /*
    template<typename T>
    class TVector2
    {
    public:

        TVector2() : x(0), y(0) {}

        TVector2(T x, T y) : x(x), y(y) {}

        explicit TVector2(TVector3<T> vec3);
        explicit TVector2(TVector4<T> vec4);

        union
        {
            struct {
                T x, y;
            };
			struct {
				T min, max;
			};
			struct {
				T left, right;
			};
			struct {
				T top, bottom;
			};
			struct {
				T width, height;
			};
            T xy[2];
        };

        inline size_t GetHash() const
		{
			return GetCombinedHashes({ GetHash(x), GetHash(y) });
		}

        [[nodiscard]] Vec2 ToVec2() const { return Vec2(x, y); }
        [[nodiscard]] TVector2<int32_t> ToVec2i() const { return {(int32_t)x, (int32_t)y}; }

        inline TVector2 operator+(const TVector2& rhs) const { return TVector2(x + rhs.x, y + rhs.y); }
        inline TVector2 operator-(const TVector2& rhs) const { return TVector2(x - rhs.x, y - rhs.y);  }
        inline TVector2 operator+() const { return *this; }
        inline TVector2 operator-() const { return TVector2(-x, -y); }
        inline TVector2 operator+=(const TVector2& rhs)
        {
            *this = *this + rhs;
            return *this;
        }

        inline TVector2 operator-=(const TVector2& rhs)
        {
            *this = *this - rhs;
            return *this;
        }

        inline bool operator==(const TVector2& rhs) const { return Math::ApproxEquals(x, rhs.x) && Math::ApproxEquals(y, rhs.y); }
        inline bool operator!=(const TVector2& rhs) const { return !operator==(rhs); }
        inline TVector2 operator*(int32_t value) const { return TVector2(value * x, value * y); }
        inline TVector2 operator*(uint32_t value) const { return TVector2(value * x, value * y); }
        inline TVector2 operator*(float value) const { return TVector2(value * x, value * y); }
        inline TVector2 operator*(TVector2 value) const { return TVector2(value.x * x, value.y * y); }

        inline TVector2 operator*=(int32_t value)
        {
            *this = *this * value;
            return *this;
        }

        inline TVector2 operator*=(uint32_t value)
        {
            *this = *this * value;
            return *this;
        }

        inline TVector2 operator*=(float value)
        {
            *this = *this * value;
            return *this;
        }

        inline TVector2 operator*=(TVector2 value)
        {
            *this = *this * value;
            return *this;
        }

        inline TVector2 operator/(float value) const { return TVector2(x / value, y / value); }

        inline TVector2 operator/=(float value)
        {
            *this = *this / value;
            return *this;
        }

        inline float GetSqrMagnitude() const { return x * x + y * y; }
        inline float GetMagnitude() const { return sqrt(GetSqrMagnitude()); }
        inline TVector2 GetNormalized() const { return *this / GetMagnitude(); }

        [[nodiscard]] float GetMax() const { return Math::Max({x, y}); }

        static float SqrDistance(TVector2 a, TVector2 b) { return (b - a).GetSqrMagnitude(); }
        static float Distance(TVector2 a, TVector2 b) { return (b - a).GetMagnitude(); }
        inline static T Dot(TVector2 a, TVector2 b) { return a.x * b.x + a.y * b.y; }
        inline float Dot(TVector2 b) const { return x * b.x + y * b.y; }

        /// Signed angle in radians between 2 vectors. ///TODO 
        inline static float SignedAngle(const TVector2 a, const TVector2 b) { return Math::ACos(Dot(a, b) / (a.GetMagnitude() * b.GetMagnitude())); } 

        /// Signed angle in radians between 2 vectors
        inline float SignedAngle(const TVector2 b) const { return SignedAngle(*this, b); }

        inline static TVector2<float> Lerp(TVector2 from, TVector2 to, float t)
        { return TVector2<float>(Math::Lerp(from.x, to.x, t), Math::Lerp(from.y, to.y, t));  }

        inline static TVector2<float> LerpUnclamped(TVector2 from, TVector2 to, float t)
        {
            return TVector2<float>(Math::LerpUnclamped(from.x, to.x, t), Math::LerpUnclamped(from.y, to.y, t));
        }

        inline std::string ToString() const { return std::string ("({}, {})", x, y); }
    };
    */

    /**
     * @brief Represents a 3D vector or a point in 3D space.
     * @tparam T 
     */
    /*
    template<typename T>
    class TVector3
    {
    public:

        TVector3() : x(0), y(0), z(0) {}
        TVector3(T x, T y) : x(x), y(y), z(0) {}
        TVector3(T x, T y, T z) : x(x), y(y), z(z) {}

        explicit TVector3(TVector2<T> vec2);
        explicit TVector3(TVector4<T> vec4);

        union
        {
            struct
            {
                T x, y, z;
            };
            T xyz[4]; ///< size/alignment is same as Vector4
        };

		inline size_t GetHash() const { return GetCombinedHashes({ SceneryEditorX::GetHash(x), SceneryEditorX::GetHash(y), SceneryEditorX::GetHash(z) }); }
        inline T operator[](int index) const { return xyz[index]; }

        [[nodiscard]] Vec3 ToVec3() const
        {
            return Vec3(x, y, z);
        }

        [[nodiscard]] TVector3<int32_t> ToVec3i() const
        {
            return {(int32_t)x, (int32_t)y, (int32_t)z};
        }

        inline TVector3 operator+(const TVector3& rhs) const
        {
            return TVector3(x + rhs.x, y + rhs.y, z + rhs.z);
        }

        inline TVector3 operator-(const TVector3& rhs) const
        {
            return TVector3(x - rhs.x, y - rhs.y, z - rhs.z);
        }

        inline TVector3 operator+() const
        {
            return *this;
        }

        inline TVector3 operator-() const
        {
            return TVector3(-x, -y, -z);
        }

        inline TVector3 operator+=(const TVector3& rhs)
        {
            *this = *this + rhs;
            return *this;
        }

        inline TVector3 operator-=(const TVector3& rhs)
        {
            *this = *this - rhs;
            return *this;
        }

        inline bool operator==(const TVector3& rhs) const
        {
            return Math::ApproxEquals(x, rhs.x) && Math::ApproxEquals(y, rhs.y) && Math::ApproxEquals(z, rhs.z);
        }

        inline bool operator!=(const TVector3& rhs) const
        {
            return !operator==(rhs);
        }

        inline TVector3 operator*(int32_t value) const
        {
            return TVector3(value * x, value * y, value * z);
        }

        inline TVector3 operator*(uint32_t value) const
        {
            return TVector3(value * x, value * y, value * z);
        }

        inline TVector3 operator*(float value) const
        {
            return TVector3(value * x, value * y, value * z);
        }

        inline TVector3 operator*(const TVector3& value) const
        {
            return TVector3(value.x * x, value.y * y, value.z * z);
        }

        inline TVector3 operator*=(int32_t value)
        {
            *this = *this * value;
            return *this;
        }

        inline TVector3 operator*=(uint32_t value)
        {
            *this = *this * value;
            return *this;
        }

        inline TVector3 operator*=(float value)
        {
            *this = *this * value;
            return *this;
        }

        inline TVector3 operator*=(const TVector3& value)
        {
            *this = *this * value;
            return *this;
        }

        inline TVector3 operator/(float value) const
        {
            return TVector3(x / value, y / value, z / value);
        }

        inline TVector3 operator/=(float value)
        {
            *this = *this / value;
            return *this;
        }

        inline float GetSqrMagnitude() const
        {
            return x * x + y * y + z * z;
        }

        inline float GetMagnitude() const
        {
            return sqrt(GetSqrMagnitude());
        }

        inline TVector3 GetNormalized() const
        {
            return *this / GetMagnitude();
        }

        [[nodiscard]] float GetMax() const
        {
            return Math::Max({x, y, z});
        }

        inline static T Dot(TVector3 a, TVector3 b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        inline float Dot(TVector3 b) const
        {
            return x * b.x + y * b.y + z * b.z;
        }

        inline static TVector3 Cross(TVector3 a, TVector3 b)
        {
            return TVector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
        }

        inline TVector3 Cross(TVector3 b) const
        {
            return TVector3(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x);
        }

        /// Signed angle in radians between 2 vectors
        inline static float SignedAngle(const TVector3 a, const TVector3 b)
        {
            return Math::ACos(Dot(a, b) / (a.GetMagnitude() * b.GetMagnitude())); // TODO
        }

        /// Signed angle in radians between 2 vectors
        inline float SignedAngle(const TVector3 b) const
        {
            return SignedAngle(*this, b);
        }

        static float SqrDistance(TVector3 a, TVector3 b)
        {
            return (b - a).GetSqrMagnitude();
        }

        static float Distance(TVector3 a, TVector3 b)
        {
            return (b - a).GetMagnitude();
        }

        inline static TVector3<float> Lerp(TVector3 from, TVector3 to, float t)
        {
            return TVector3<float>(Math::Lerp(from.x, to.x, t), Math::Lerp(from.y, to.y, t), Math::Lerp(from.z, to.z, t));
        }

        inline static TVector3<float> LerpUnclamped(TVector3 from, TVector3 to, float t)
        {
            return TVector3<float>(Math::LerpUnclamped(from.x, to.x, t), Math::LerpUnclamped(from.y, to.y, t), Math::LerpUnclamped(from.z, to.z, t));
        }

        inline std::string ToString() const
        {
            return std::string ("({}, {}, {})", x, y, z);
        }
    };
    */

    /**
     * @brief Represents a 4D vector or a rectangle defined by its four components.
     * @tparam T The type of the components (e.g., float, int).
     */
    /*
    template<typename T>
    class TVector4
    {
    public:

        TVector4() : x(0), y(0), z(0), w(0) {}
        TVector4(T x, T y) : x(x), y(y), z(0), w(0) {}
        TVector4(T x, T y, T z) : x(x), y(y), z(z), w(0) {}
		TVector4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

        explicit TVector4(TVector2<T> vec2);
		TVector4(TVector2<T> xy, TVector2<T> zw);
        explicit TVector4(TVector3<T> vec3);


        TVector4(TVector3<T> vec3, T w) : x(vec3.x), y(vec3.y), z(vec3.z), w(w) {}

        union
        {
            struct
            {
                T x, y, z, w;
            };
			struct
            {
				T left, top, right, bottom;
			};
			struct
            {
				T topLeft, topRight, bottomRight, bottomLeft;
			};
			struct
            {
				TVector2<T> min, max;
			};
			struct
            {
				TVector2<T> pos, size;
			};

            T xyzw[4];
        };

		inline size_t GetHash() const
		{
			return GetCombinedHashes({ SceneryEditorX::GetHash(x), SceneryEditorX::GetHash(y), SceneryEditorX::GetHash(z), SceneryEditorX::GetHash(w) });
		}

        [[nodiscard]] Vec4 ToVec4() const
        {
            return Vec4(x, y, z, w);
        }

        [[nodiscard]] TVector4<int32_t> ToVec4i() const
        {
            return {(int32_t)x, (int32_t)y, (int32_t)z, (int32_t)z};
        }

        inline T& operator[](int index)
        {
            return xyzw[index];
        }

        inline const T& operator[](int index) const
        {
            return xyzw[index];
        }

        inline TVector4 operator+(const TVector4& rhs) const
        {
            return TVector4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
        }

        inline TVector4 operator-(const TVector4& rhs) const
        {
            return TVector4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
        }

        inline TVector4 operator+() const
        {
            return *this;
        }

        inline TVector4 operator-() const
        {
            return TVector4(-x, -y, -z, -w);
        }

        inline TVector4 operator+=(const TVector4& rhs)
        {
            *this = *this + rhs;
            return *this;
        }

        inline TVector4 operator-=(const TVector4& rhs)
        {
            *this = *this - rhs;
            return *this;
        }

        inline bool operator==(const TVector4& rhs) const
        {
            return Math::ApproxEquals(x, rhs.x) && Math::ApproxEquals(y, rhs.y) && Math::ApproxEquals(z, rhs.z) &&  Math::ApproxEquals(w, rhs.w);
        }

        inline bool operator!=(const TVector4& rhs) const
        {
            return !operator==(rhs);
        }

        inline TVector4 operator*(int32_t value) const
        {
            return TVector4(value * x, value * y, value * z, value * w);
        }

        inline TVector4 operator*(uint32_t value) const
        {
            return TVector4(value * x, value * y, value * z, value * w);
        }

        inline TVector4 operator*(float value) const
        {
            return TVector4(value * x, value * y, value * z, value * w);
        }

        inline TVector4 operator*(const TVector4& value) const
        {
            return TVector4(value.x * x, value.y * y, value.z * z, value.w * w);
        }

        inline TVector4 operator*=(int32_t value)
        {
            *this = *this * value;
            return *this;
        }

        inline TVector4 operator*=(uint32_t value)
        {
            *this = *this * value;
            return *this;
        }

        inline TVector4 operator*=(float value)
        {
            *this = *this * value;
            return *this;
        }

        inline TVector4 operator*=(const TVector4& value)
        {
            *this = *this * value;
            return *this;
        }

        inline TVector4 operator/(float value) const
        {
            return TVector4(x / value, y / value, z / value, w / value);
        }

        inline TVector4 operator/=(float value)
        {
            *this = *this / value;
            return *this;
        }

        inline float GetSqrMagnitude() const
        {
            return x * x + y * y + z * z + w * w;
        }

        inline float GetMagnitude() const
        {
            return sqrt(GetSqrMagnitude());
        }

        inline TVector4 GetNormalized() const
        {
            return *this / GetMagnitude();
        }

        [[nodiscard]] float GetMax() const
		{
            return Math::Max({x, y, z, w});
		}

        inline static float Dot(TVector4 a, TVector4 b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        inline float Dot(TVector4 b) const
        {
            return x * b.x + y * b.y + z * b.z + w * b.w;
        }

        inline static TVector4 Cross(TVector4 a, TVector4 b)
        {
            return TVector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x, a.w * b.w);
        }

        inline TVector4 Cross(TVector4 b) const
        {
            return TVector4(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x, w * b.w);
        }

        static float SqrDistance(TVector4 a, TVector4 b)
		{
            return (b - a).GetSqrMagnitude();
		}

        static float Distance(TVector4 a, TVector4 b)
		{
            return (b - a).GetMagnitude();
		}

        /// Signed angle in radians between 2 vectors
        inline static float SignedAngle(TVector4 a, TVector4 b)
        {
            return Math::ACos(Dot(a, b) / (a.GetMagnitude() * b.GetMagnitude())); // TODO
        }

        /// Signed angle in radians between 2 vectors
        inline float SignedAngle(TVector4 b) const
        {
            return SignedAngle(*this, b);
        }

        inline static TVector4<float> Lerp(TVector4 from, TVector4 to, float t)
        {
            return TVector4<float>(Math::Lerp(from.x, to.x, t), Math::Lerp(from.y, to.y, t), Math::Lerp(from.z, to.z, t), Math::Lerp(from.w, to.w, t));
        }

        inline static TVector4<float> LerpUnclamped(TVector4 from, TVector4 to, float t)
        {
            return TVector4<float>(Math::LerpUnclamped(from.x, to.x, t), Math::LerpUnclamped(from.y, to.y, t), Math::LerpUnclamped(from.z, to.z, t), Math::LerpUnclamped(from.w, to.w, t));
        }

        inline std::string ToString() const
        {
            return std::string ("({}, {}, {}, {})", x, y, z, w);
        }
    };
    */


    /**
     * @brief Represents a rectangle defined by its minimum and maximum points or by its left, top, right, and bottom edges.
     *
     * This class provides methods to create rectangles from various representations, check if a point is contained within the rectangle,
     * check for overlaps with another rectangle, and perform various geometric operations such as scaling, translating, and encapsulating points or other rectangles.
     * It also provides methods to compute the bounding box of a rotated rectangle and to rotate points around a center.
     *
     * @note The rectangle can be defined either by its minimum and maximum points (min, max) or by its edges (left, top, right, bottom).
     */
    /*
    class Rect
    {
    public:

        /**
         * @brief Default constructor that initializes the rectangle to an empty state.
         #1#
        Rect() : left(0), top(0), right(0), bottom(0) {}

        /**
         * @brief Creates a rectangle from a Vec4 representation.
         * @param vec 
         #1#
        explicit Rect(const Vec4& vec) : left(vec.left), top(vec.top), right(vec.right), bottom(vec.bottom) {}

        /**
         * @brief Creates a rectangle from its minimum and maximum points.
         * @param min 
         * @param max 
         #1#
        Rect(const Vec2& min, const Vec2& max) : min(min), max(max) {}

        /**
         * @brief Creates a rectangle from its left, top, right, and bottom edges.
         * @param left 
         * @param top 
         * @param right 
         * @param bottom 
         #1#
        Rect(float left, float top, float right, float bottom) : left(left), top(top), right(right), bottom(bottom) {}

        /**
         * @brief Creates a rectangle from a minimum point and a size.
         *
         * This function constructs a rectangle using the specified minimum point and size.
         *
         * @param min 
         * @param size 
         * @return Returns a Rect object representing the rectangle defined by the minimum point and size.
         #1#
        static Rect FromSize(const Vec2& min, const Vec2& size)
        {
            return Rect(min, min + size);
        }

        /**
         * @brief Creates a rectangle from a position and size.
         *
         * This function constructs a rectangle using the specified position (x, y) and size (w, h).
         * It calculates the minimum and maximum points of the rectangle based on the position and size,
         * resulting in a rectangle that starts at (x, y) and extends to (x + w, y + h).
         *
         * @param x 
         * @param y 
         * @param w 
         * @param h 
         * @return Returns a Rect object representing the rectangle defined by the position and size.
         #1#
        static Rect FromSize(float x, float y, const float w, const float h)
        {
            return {x, y, x + w, y + h};
        }

        /**
         * @brief Checks if a point is contained within this rectangle.
         *
         * This function checks if the given point's x and y coordinates are within the bounds defined by the rectangle's minimum and maximum points.
         * It returns true if the point is inside the rectangle, including its edges, and false otherwise.
         *
         * @param point The point to check for containment within the rectangle.
         * @return Returns true if the point is contained within the rectangle, false otherwise.
         #1#
        [[nodiscard]] bool Contains(const Vec2 point) const
        {
            return point.x >= min.x && point.y >= min.y && point.x <= max.x && point.y <= max.y;
        }

        /**
         * @brief Checks if this rectangle overlaps with another rectangle.
         *
         * This function checks if the current rectangle overlaps with another rectangle by comparing their edges.
         *
         * @param other The rectangle to check for overlap with this rectangle.
         * @return Returns true if the rectangles overlap, false otherwise.
         #1#
        [[nodiscard]] bool Overlaps(const Rect& other) const
        {
            return !(right < other.left || left > other.right || bottom < other.top || top > other.bottom);
        }

        union
        {
            struct
            {
                float left;
                float top;
                float right;
                float bottom;
            };

            struct
            {
                Vec2 min;
                Vec2 max;
            };
        };

        /**
         * @brief Calculates the size of the rectangle.
         *
         * This function computes the size of the rectangle by subtracting the minimum point from the maximum point.
         *
         * @return Returns a Vec2 object representing the width and height of the rectangle.
         #1#
        inline Vec2 GetSize() const
        {
            return max - min;
        }

		/**
		 * @brief Calculates the area of the rectangle.
		 *
		 * This function computes the area of the rectangle by multiplying its width and height.
		 *
		 * @return Returns the area of the rectangle as a float.
		 #1#
        [[nodiscard]] float GetArea() const
        {
            return GetSize().width * GetSize().height;
        }

        /**
         * @brief Calculates the area of the rectangle as an integer.
         *
         * This function computes the area of the rectangle by multiplying its width and height,
         * and then rounding the result to the nearest integer.
         *
         * @return Returns the area of the rectangle as an integer.
         #1#
        [[nodiscard]] int32_t GetAreaInt() const
        {
            return Math::RoundToInt(GetSize().width * GetSize().height);
        }

        /**
         * @brief Converts the rectangle to a Vec4 representation.
         *
         * This function converts the rectangle's left, top, right, and bottom edges into a Vec4 object.
         * @return Returns a Vec4 object containing the rectangle's edges.
         #1#
        inline Vec4 ToVec4() const
        {
            return {left, top, right, bottom};
        }

        /**
         * @brief Scales the rectangle by a given vector.
         *
         * This function takes a vector that represents the scale factors for the x and y dimensions,
         * and creates a new rectangle that is scaled from the center of the current rectangle.
         *
         * @param scale A vector representing the scale factors for the x and y dimensions.
         * @return Returns a new rectangle that is scaled by the given vector.
         #1#
        [[nodiscard]] Rect Scale(const Vec2 scale) const
        {
            const Vec2 size = GetSize();
            const Vec2 center = (min + max) * 0.5f;
            return Rect(center - size / 2.0f * scale, center + size / 2.0f * scale);
        }

        /**
         * @brief Scales the rectangle by a uniform scale factor.
         *
         * This function takes a single scale factor and creates a new rectangle that is scaled uniformly from the center of the current rectangle.
         * It calculates the new size by multiplying the current size by the scale factor,
         * and then computes the new minimum and maximum points based on the center of the rectangle.
         *
         * @param scale 
         * @return Returns a new rectangle that is scaled uniformly by the given factor.
         #1#
        [[nodiscard]] Rect Scale(const float scale) const
        {
            return Scale(Vec2(scale, scale));
        }

        /**
         * @brief Translates the rectangle by a given vector.
         *
         * This function takes a translation vector and creates a new rectangle that is translated from the current rectangle's minimum point.
         * It calculates the new minimum point by adding the translation vector to the current minimum point,
         * and keeps the size of the rectangle unchanged.
         *
         * @param translation 
         * @return Returns a new rectangle that is translated by the given vector.
         #1#
        [[nodiscard]] Rect Translate(const Vec2 translation) const
        {
            const Vec2 size = GetSize();
            return FromSize(min + translation, size);
        }

        /**
         * @brief Encapsulates this rectangle with a point.
         *
         * This function takes a point and expands the current rectangle to include the point.
         * It calculates the minimum and maximum x and y coordinates from the current rectangle and the point to create a new rectangle that encompasses both.
         *
         * @param point
         * @return Returns a new rectangle that encapsulates both this rectangle and the point.
         #1#
        [[nodiscard]] Rect Encapsulate(const Vec2 &point) const
        {
            auto xmin = Math::Min({ min.x, max.x, point.x });
            auto xmax = Math::Max({ min.x, max.x, point.x });
            auto ymin = Math::Min({ min.y, max.y, point.y });
            auto ymax = Math::Max({ min.y, max.y, point.y });
            return {xmin, ymin, xmax, ymax};
        }

        /**
         * @brief Encapsulates this rectangle with another rectangle.
         *
         * This function takes another rectangle and expands the current rectangle to include both its own area and the area of the other rectangle.
         * It calculates the minimum and maximum x and y coordinates from both rectangles to create a new rectangle that encompasses both.
         *
         * @param other 
         * @return Returns a new rectangle that encapsulates both this rectangle and the other rectangle.
         #1#
        [[nodiscard]] Rect Encapsulate(const Rect &other) const
        {
			auto xmin = Math::Min({ min.x, max.x, other.min.x, other.max.x });
            auto xmax = Math::Max({ min.x, max.x, other.min.x, other.max.x });
            auto ymin = Math::Min({ min.y, max.y, other.min.y, other.max.y });
            auto ymax = Math::Max({ min.y, max.y, other.min.y, other.max.y });
            return {xmin, ymin, xmax, ymax};
        }

        /**
         * @brief Rotates a point around a center by a specified angle.
         *
         * This function takes a point (x, y) and rotates it around a center point (centerX, centerY) by a given angle in degrees.
         * It calculates the new position of the point after rotation using basic trigonometric functions.
         *
         * @param x
         * @param y  
         * @param centerX
         * @param centerY 
         * @param angle 
         * @param newX 
         * @param newY 
         #1#
        static void RotatePoint(const float x, const float y, const float centerX, const float centerY, const double angle, float& newX, float& newY)
        {
            const float radians = angle * (M_PI / 180.0); ///< Convert degrees to radians
            const float cosine = std::cos(radians);
            const float sine = std::sin(radians);

            newX = centerX + (x - centerX) * cosine - (y - centerY) * sine;
            newY = centerY + (x - centerX) * sine + (y - centerY) * cosine;
        }

        /**
         * @brief Computes the bounding box of a rotated rectangle.
         *
         * This function calculates the bounding box of a rectangle after it has been rotated by a specified angle around its center.
         * It takes into account the four corners of the rectangle and computes the new positions after rotation,
         * then determines the minimum and maximum x and y coordinates to create the bounding box.
         *
         * @param rectangle The original rectangle to be rotated.
         * @param angle The angle in degrees by which to rotate the rectangle.
         * @return Returns a new rectangle representing the bounding box of the rotated rectangle.
         #1#
        static Rect ComputeBoundingBox(const Rect& rectangle, const float angle)
        {
            const float x1 = rectangle.min.x;
            const float y1 = rectangle.min.y;
            const float x2 = rectangle.max.x;
            const float y2 = rectangle.min.y;
            const float x3 = rectangle.max.x;
            const float y3 = rectangle.max.y;
            const float x4 = rectangle.min.x;
            const float y4 = rectangle.max.y;

            const float centerX = (rectangle.min.x + rectangle.max.x) / 2.0f;
            const float centerY = (rectangle.min.y + rectangle.max.y) / 2.0f;

            float newX1, newY1, newX2, newY2, newX3, newY3, newX4, newY4;

            RotatePoint(x1, y1, centerX, centerY, angle, newX1, newY1);
            RotatePoint(x2, y2, centerX, centerY, angle, newX2, newY2);
            RotatePoint(x3, y3, centerX, centerY, angle, newX3, newY3);
            RotatePoint(x4, y4, centerX, centerY, angle, newX4, newY4);

            const float minX = std::min({ newX1, newX2, newX3, newX4 });
            const float maxX = std::max({ newX1, newX2, newX3, newX4 });
            const float minY = std::min({ newY1, newY2, newY3, newY4 });
            const float maxY = std::max({ newY1, newY2, newY3, newY4 });

            const Rect boundingBox = { minX, minY, maxX - minX, maxY - minY };
            return boundingBox;
        }

        inline bool operator==(const Rect& rhs) const
        {
            return min == rhs.min && max == rhs.max;
        }

        inline bool operator!=(const Rect& rhs) const
        {
            return !operator==(rhs);
        }

        inline Rect operator+(const Rect& rhs) const
        {
            return {left + rhs.left, top + rhs.top, right + rhs.right, bottom + rhs.bottom};
        }

        inline Rect operator-(const Rect& rhs) const
        {
            return {left - rhs.left, top - rhs.top, right - rhs.right, bottom - rhs.bottom};
        }

        inline Rect& operator+=(const Rect& rhs)
        {
            left += rhs.left; top += rhs.top; right += rhs.right; bottom += rhs.bottom;
            return *this;
        }

        inline Rect& operator-=(const Rect& rhs)
        {
            left -= rhs.left; top -= rhs.top; right -= rhs.right; bottom -= rhs.bottom;
            return *this;
        }

        inline size_t GetHash() const
        {
            size_t hash = GetHash(left);
            CombineHash(hash, top);
            CombineHash(hash, right);
            CombineHash(hash, bottom);
            return hash;
        }

        inline std::string ToString() const
        {
            return fmt::format("({}, {}, {}, {})", left, top, right, bottom);
        }
        
    };
    */

    /*
    template<typename T>
    TVector2<T>::TVector2(TVector3<T> vec3) : x(vec3.x), y(vec3.y) {}

    template<typename T>
    TVector2<T>::TVector2(TVector4<T> vec4) : x(vec4.x), y(vec4.y) {}

    template<typename T>
    TVector3<T>::TVector3(TVector4<T> vec4) : x(vec4.x), y(vec4.y), z(vec4.z)
    {
        xyz[3] = 0;
    }

    template<typename T>
    TVector3<T>::TVector3(TVector2<T> vec2) : x(vec2.x), y(vec2.y), z(0)
    {
        xyz[3] = 0;
    }

    template<typename T>
    TVector4<T>::TVector4(TVector2<T> vec2) : x(vec2.x), y(vec2.y), z(0), w(0) {}

	template<typename T>
	TVector4<T>::TVector4(TVector2<T> xy, TVector2<T> zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}

    template<typename T>
    TVector4<T>::TVector4(TVector3<T> vec3) : x(vec3.x), y(vec3.y), z(vec3.z), w(0) {}

    template<typename T>
    inline TVector2<T> operator*(int32_t lhs, TVector2<T> rhs)
    {
        return rhs * lhs;
    }

    template<typename T>
    inline TVector3<T> operator*(int32_t lhs, TVector3<T> rhs)
    {
        return rhs * lhs;
    }
    
    template<typename T>
    inline TVector4<T> operator*(int32_t lhs, TVector4<T> rhs)
    {
        return rhs * lhs;
    }

    template<typename T>
    inline TVector2<T> operator*(float lhs, TVector2<T> rhs)
    {
        return rhs * lhs;
    }

    template<typename T>
    inline TVector3<T> operator*(float lhs, TVector3<T> rhs)
    {
        return rhs * lhs;
    }

    template<typename T>
    inline TVector4<T> operator*(float lhs, TVector4<T> rhs)
    {
        return rhs * lhs;
    }
    */

}

/** fmt user-defined Formatter for SceneryEditorX::Vec2 */
/*
template <>
struct fmt::formatter<SceneryEditorX::Utils::Vec2>
{
    /**
     * @brief Parses format specifications of the form ['f' | 'e'].
     *
     * This function is called when the formatter is used in a format string.
     * It allows for parsing any format specifications that may be present.
     *
     * @param ctx The format parse context containing the range to parse.
     * @return Returns an iterator past the end of the parsed range.
     #1#
    static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
    {
        ///< Return an iterator past the end of the parsed range:
        return ctx.end();
    }

    /**
     * @brief Formats the point p using the parsed format specification (presentation) stored in this formatter.
     *
     * @param vec2 The Vec2 object to format.
     * @param ctx The format context containing the output iterator.
     * @return Returns an output iterator to write to.
     #1#
    template <typename FormatContext>
    auto format(const SceneryEditorX::Utils::Vec2 &vec2, FormatContext &ctx) const -> decltype(ctx.out())
    {
        ///< ctx.out() is an output iterator to write to.
        return fmt::format_to(ctx.out(), "{}", vec2.ToString());
    }
};
*/

/** fmt user-defined Formatter for SceneryEditorX::Vec3 */
/*
template <>
struct fmt::formatter<SceneryEditorX::Utils::Vec3>
{
    /**
     * @brief Parses format specifications of the form ['f' | 'e'].
     *
     * This function is called when the formatter is used in a format string.
     * It allows for parsing any format specifications that may be present.
     *
     * @param ctx The format parse context containing the range to parse.
     * @return Returns an iterator past the end of the parsed range.
     #1#
    static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
    {
        ///< Return an iterator past the end of the parsed range:
        return ctx.end();
    }

    /**
     * @brief Formats the point p using the parsed format specification (presentation) stored in this formatter.
     *
     * @param vec3 The Vec3 object to format.
     * @param ctx The format context containing the output iterator.
     * @return Returns an output iterator to write to.
     #1#
    template <typename FormatContext>
    auto format(const SceneryEditorX::Utils::Vec3 &vec3, FormatContext &ctx) const -> decltype(ctx.out())
    {
        ///< ctx.out() is an output iterator to write to.
        return fmt::format_to(ctx.out(), "{}", vec3.ToString());
    }
};
*/

/** fmt user-defined Formatter for SceneryEditorX::Vec4 */
/*
template <>
struct fmt::formatter<SceneryEditorX::Utils::Vec4>
{
    /**
     * @brief Parses format specifications of the form ['f' | 'e'].
     *
     * This function is called when the formatter is used in a format string.
     * It allows for parsing any format specifications that may be present.
     *
     * @param ctx The format parse context containing the range to parse.
     * @return Returns an iterator past the end of the parsed range.
     #1#
    static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
    {
        ///< Return an iterator past the end of the parsed range:
        return ctx.end();
    }

    /**
     * @brief Formats the point p using the parsed format specification (presentation) stored in this formatter.
     *
     * @param vec4 The Vec4 object to format.
     * @param ctx The format context containing the output iterator.
     * @return Returns an output iterator to write to.
     #1#
    template <typename FormatContext>
    auto format(const SceneryEditorX::Utils::Vec4 &vec4, FormatContext &ctx) const -> decltype(ctx.out())
    {
        ///< ctx.out() is an output iterator to write to.
        return fmt::format_to(ctx.out(), "{}", vec4.ToString());
    }
};
*/

/** fmt user-defined Formatter for SceneryEditorX::Utils::Vec2i */
/*
template <>
struct fmt::formatter<SceneryEditorX::Utils::Vec2i>
{
    /**
     * @brief Parses format specifications of the form ['f' | 'e'].
     *
     * This function is called when the formatter is used in a format string.
     * It allows for parsing any format specifications that may be present.
     *
     * @param ctx The format parse context containing the range to parse.
     * @return Returns an iterator past the end of the parsed range.
     #1#
    static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
    {
        ///< Return an iterator past the end of the parsed range:
        return ctx.end();
    }

    /**
     * @brief Formats the point p using the parsed format specification (presentation) stored in this formatter.
     *
     * @param vec2 The Vec2i object to format.
     * @param ctx The format context containing the output iterator.
     * @return Returns an output iterator to write to.
     #1#
    template <typename FormatContext>
    auto format(const SceneryEditorX::Utils::Vec2i &vec2, FormatContext &ctx) const -> decltype(ctx.out())
    {
        ///< ctx.out() is an output iterator to write to.
        return fmt::format_to(ctx.out(), "{}", vec2.ToString());
    }
};
*/

/// fmt user-defined Formatter for SceneryEditorX::Utils::Vec3i
/*
template <>
struct fmt::formatter<SceneryEditorX::Utils::Vec3i>
{
    /**
     * @brief Parses format specifications of the form ['f' | 'e'].
     *
     * This function is called when the formatter is used in a format string.
     * It allows for parsing any format specifications that may be present.
     *
     * @param ctx The format parse context containing the range to parse.
     * @return Returns an iterator past the end of the parsed range.
    #1#
    static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
    {
        ///< Return an iterator past the end of the parsed range:
        return ctx.end();
    }

    /**
     * @brief Formats the point p using the parsed format specification (presentation) stored in this formatter.
     *
     * @param vec3 The Vec3i object to format.
     * @param ctx The format context containing the output iterator.
     * @return Returns an output iterator to write to.
    #1#
    template <typename FormatContext>
    auto format(const SceneryEditorX::Utils::Vec3i &vec3, FormatContext &ctx) const -> decltype(ctx.out())
    {
        ///< ctx.out() is an output iterator to write to.
        return fmt::format_to(ctx.out(), "{}", vec3.ToString());
    }
};
*/

/** fmt user-defined Formatter for SceneryEditorX::Utils::Vec4i */
/*
template <>
struct fmt::formatter<SceneryEditorX::Utils::Vec4i>
{
    /**
     * @brief Parses format specifications of the form ['f' | 'e'].
     *
     * This function is called when the formatter is used in a format string.
     * It allows for parsing any format specifications that may be present.
     *
     * @param ctx The format parse context containing the range to parse.
     * @return Returns an iterator past the end of the parsed range.
     #1#
    static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
    {
        ///< Return an iterator past the end of the parsed range:
        return ctx.end();
    }

    /**
     * @brief Formats the point p using the parsed format specification (presentation) stored in this formatter.
     * 
     * @param vec4 The Vec4i object to format.
     * @param ctx The format context containing the output iterator.
     * @return Returns an output iterator to write to.
     #1#
    template <typename FormatContext>
    auto format(const SceneryEditorX::Utils::Vec4i &vec4, FormatContext &ctx) const -> decltype(ctx.out())
    {
        ///< ctx.out() is an output iterator to write to.
        return fmt::format_to(ctx.out(), "{}", vec4.ToString());
    }
};
*/

/*
template <>
struct fmt::formatter<SceneryEditorX::Utils::Rect>
{
    /**
     * @brief Parses format specifications of the form ['f' | 'e'].
     *
     * This function is called when the formatter is used in a format string.
     * It allows for parsing any format specifications that may be present.
     *
     * @param ctx The format parse context containing the range to parse.
     * @return Returns an iterator past the end of the parsed range.
     #1#
    static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin()) 
    {
        ///< Return an iterator past the end of the parsed range:
        return ctx.end();
    }

    /**
     * @brief Formats the rectangle using the parsed format specification (presentation) stored in this formatter.
     *
     * @param rect The Rect object to format.
     * @param ctx The format context containing the output iterator.
     * @return Returns an output iterator to write to.
     #1#
    template <typename FormatContext>
    auto format(const SceneryEditorX::Utils::Rect &rect, FormatContext &ctx) const -> decltype(ctx.out())
    {
        ///< ctx.out() is an output iterator to write to.
        return fmt::format_to(ctx.out(), "{}", rect.ToString());
    }

};
*/

/// -------------------------------------------------------

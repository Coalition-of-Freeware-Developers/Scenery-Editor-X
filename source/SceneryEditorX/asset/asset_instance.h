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
 * asset_instance.h
 * -------------------------------------------------------
 * Created: 20/03/2026
 * -------------------------------------------------------
 */
#pragma once

#include <xMath/includes/math_utils.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	#pragma pack(push, 1)

	/**
	 * @struct Instance
	 * @brief Struct representing a single instance of a renderable, containing its transform and optional per-instance data.
	 */
	struct Instance
	{
		uint16_t positionX;   // 2 bytes
		uint16_t positionY;   // 2 bytes
		uint16_t positionZ;   // 2 bytes
		uint16_t normal_Oct;  // 2 bytes
		uint8_t yaw_Packed;   // 1 byte
		uint8_t scale_Packed; // 1 byte
							  // total: 10 bytes
		/**
		 * @brief Get the transformation matrix of the instance.
		 * @return The transformation matrix.
		 */
		[[nodiscard]] Matrix GetMatrix() const
		{
			// compose position
			Vec3 position(HalfToFloat(positionX), HalfToFloat(positionY), HalfToFloat(positionZ));

			// compose rotation
			Vec3 normal = DecodeOctahedral(normal_Oct);
		 Vec3 up(0.0f, 1.0f, 0.0f);
			float up_dot_normal  = xMath::Dot(up, normal);
			Quat quat_align;
			if (std::abs(up_dot_normal) >= 0.999999f)
			{
			  quat_align = up_dot_normal > 0.0f ? Quat::Identity() : Quat(1.0f, 0.0f, 0.0f, 0.0f);
			}
			else
			{
				float s                  = std::sqrt(2.0f + 2.0f * up_dot_normal);
				Vec3 cross_prod			 = xMath::Cross(up, normal) / s;
				quat_align               = Quat(cross_prod.x, cross_prod.y, cross_prod.z, s * 0.5f);
			}
		 float yaw = (static_cast<float>(yaw_Packed) / 255.0f) * xMath::TWO_PI;
			Quat quatYaw(0.0f, std::sin(-yaw * 0.5f), 0.0f, std::cos(-yaw * 0.5f));
			Quat quat = quat_align * quatYaw;

			// compose scale
			float t = static_cast<float>(scale_Packed) / 255.0f;
			float scale_float = std::exp(std::lerp(std::log(0.01f), std::log(100.0f), t));

			// compose matrix
		   return Matrix(position, quat, Vec3(scale_float));
		}

		/**
		 * @brief Set the transformation matrix of the instance.
		 * @param matrix The transformation matrix to set.
		 */
		void SetMatrix(const Matrix& matrix)
		{
			// pack position
			Vec3 position = matrix.GetTranslation();
			positionX             = FloatToHalf(position.x);
			positionY             = FloatToHalf(position.y);
			positionZ             = FloatToHalf(position.z);

			// pack normal
			Quat quat = matrix.GetRotation();
		 Vec3 normal  = quat * Vec3(0.0f, 1.0f, 0.0f);
			normal_Oct            = EncodeOctahedral(normal);

			// pack yaw
		  Vec3 up(0.0f, 1.0f, 0.0f);
			float up_dot_normal = xMath::Dot(up, normal);
			Quat quat_align;
			if (std::abs(up_dot_normal) >= 0.999999f)
			{
			  quat_align = up_dot_normal > 0.0f ? Quat::Identity() : Quat(1.0f, 0.0f, 0.0f, 0.0f);
			}
			else
			{
				float s                  = std::sqrt(2.0f + 2.0f * up_dot_normal);
				Vec3 cross_prod			 = xMath::Cross(up, normal) / s;
				quat_align               = Quat(cross_prod.x, cross_prod.y, cross_prod.z, s * 0.5f);
			}
			Quat quat_yaw  = quat_align.Conjugate() * quat;
			float half_angle           = std::atan2(-quat_yaw.y, quat_yaw.w);
			float yaw                  = half_angle * 2.0f;
			if (yaw < 0.0f) yaw       += xMath::TWO_PI;
			yaw_Packed                 = static_cast<uint8_t>((yaw / xMath::TWO_PI) * 255.0f);
			// pack scale
			float scaleAvg = (matrix.GetScale().x + matrix.GetScale().y + matrix.GetScale().z) / 3.0f;
			scaleAvg       = std::max(0.01f, std::min(100.0f, scaleAvg));
			float t         = (std::log(scaleAvg) - std::log(0.01f)) / (std::log(100.0f) - std::log(0.01f));
			scale_Packed    = static_cast<uint8_t>(t * 255.0f);
		}

		/**
		 * @brief Get an identity instance with default values.
		 * @return An identity instance.
		 */
		static Instance GetIdentity()
		{
			Instance instance;
			instance.positionX   = 0;
			instance.positionY   = 0;
			instance.positionZ   = 0;
			instance.normal_Oct   = 0;
			instance.yaw_Packed   = 0;
			instance.scale_Packed = 0;

			return instance;
		}

		/**
		 * @brief Encode a direction vector into an octahedral representation.
		 * @param dir The direction vector to encode.
		 * @return The encoded octahedral representation.
		 */
		static uint16_t EncodeOctahedral(const xMath::Vec3& dir)
		{
			xMath::Vec3 oct = dir / (std::abs(dir.x) + std::abs(dir.y) + std::abs(dir.z));
			if (oct.z < 0.0f)
			{
				float temp_x = oct.x;
				oct.x = (1.0f - std::abs(oct.y)) * (temp_x >= 0.0f ? 1.0f : -1.0f);
				oct.y = (1.0f - std::abs(temp_x)) * (oct.y >= 0.0f ? 1.0f : -1.0f);
			}
			uint8_t ox = static_cast<uint8_t>(std::round((oct.x * 0.5f + 0.5f) * 255.0f));
			uint8_t oy = static_cast<uint8_t>(std::round((oct.y * 0.5f + 0.5f) * 255.0f));
			return (static_cast<uint16_t>(ox) << 8) | oy;
		}

		/**
		 * @brief Decode an octahedral representation into a direction vector.
		 * @param packed The encoded octahedral representation.
		 * @return The decoded direction vector.
		 */
		static xMath::Vec3 DecodeOctahedral(uint16_t packed)
		{
			float x = (static_cast<float>(packed >> 8) / 255.0f) * 2.0f - 1.0f;
			float y = (static_cast<float>(packed & 0xFF) / 255.0f) * 2.0f - 1.0f;
			float z = 1.0f - std::abs(x) - std::abs(y);
			if (z < 0.0f)
			{
				float temp_x = x;
				x = (1.0f - std::abs(y)) * (x >= 0.0f ? 1.0f : -1.0f);
				y = (1.0f - std::abs(temp_x)) * (y >= 0.0f ? 1.0f : -1.0f);
			}
			xMath::Vec3 dir(x, y, z);
		 return xMath::Normalize(dir);
		}

		/**
		 * @brief Convert a float to IEEE 754 half-precision.
		 * @param value The float value to convert.
		 * @return The half-precision representation.
		 */
		static uint16_t FloatToHalf(float value)
		{
			union { float f; uint32_t i; } u = { value };
			uint32_t sign = (u.i >> 16) & 0x8000;
			int32_t exponent = ((u.i >> 23) & 0xFF) - 127;
			uint32_t mantissa = u.i & 0x7FFFFF;
			if (exponent <= -15) return sign;
			if (exponent > 15) return sign | 0x7C00;
			if (exponent <= -14)
			{
				mantissa |= 0x800000;
				int shift = -14 - exponent;
				mantissa >>= shift;
				return sign | mantissa;
			}
			exponent += 15;
			mantissa >>= 13;
			return sign | (exponent << 10) | mantissa;
		}

		/**
		 * @brief Convert IEEE 754 half-precision to float.
		 * @param value The half-precision value to convert.
		 * @return The float representation.
		 */
		static float HalfToFloat(uint16_t value)
		{
			// extract components
			uint32_t sign = (value & 0x8000) << 16;
			uint32_t exp  = (value >> 10) & 0x1F;
			uint32_t mant = value & 0x3FF;
		
			// handle inf/nan as 0
			if (exp == 0x1F)
			{
				return 0.0f;
			}
		
			// zero
			if (exp == 0 && mant == 0)
			{
				return std::bit_cast<float>(sign);
			}
		
			// denormalized
			if (exp == 0)
			{
				// normalize mantissa
				int shifts = std::countl_zero(mant) - 21; // 32 - 11 effective bits
				mant <<= shifts;
				exp = 1 - shifts;
			}
		
			// half 15 to float 127
			exp += 112; // 127 - 15 = 112
		
			// assemble float bits
			return std::bit_cast<float>(sign | (exp << 23) | (mant << 13));
		}
	};
	#pragma pack(pop)

}

// -------------------------------------------------------

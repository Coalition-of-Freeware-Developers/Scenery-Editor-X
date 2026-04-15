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
 * instance_buffer.h
 * -------------------------------------------------------
 * Created: 14/04/2026
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX
{

#pragma pack(push, 1) // tightly pack the instance data to minimize memory usage and bandwidth

	/**
	 * @struct Instance
	 * @brief Represents a single instance of a 3D object with position, rotation, and scale.
	 *
	 * Compact per-instance transform record intended as an instance-buffer element for instanced rendering.
	 * Instance packs position (16-bit half), normal (octahedral uint16), yaw and scale into 10 bytes and provides SetMatrix/GetMatrix 
	 * to convert to/from a full xMath::Matrix for CPU/GPU use.
	 */
	struct Instance
	{
		uint16_t position_X;  // 2 bytes
		uint16_t position_Y;  // 2 bytes
		uint16_t position_Z;  // 2 bytes
		uint16_t normal_Oct;  // 2 bytes
		uint8_t yaw_Packed;   // 1 byte
		uint8_t scale_Packed; // 1 byte
							  // total: 10 bytes

		/**
		 * @brief Decodes the instance data into a transformation matrix.
		 * @return The transformation matrix representing the instance's position, rotation, and scale.
		 */
		xMath::Matrix GetMatrix() const
		{
			// compose position
			xMath::Vec3 position(HalfToFloat(position_X), HalfToFloat(position_Y), HalfToFloat(position_Z));

			// compose rotation
			xMath::Vec3 normal = DecodeOctahedral(normal_Oct);
			xMath::Vec3 up     = xMath::Vec3::UP;
			float upDotNormal  = Dot(up,normal);
			xMath::Quat quatAlign;
			if (std::abs(upDotNormal) >= 0.999999f)
			{
				quatAlign = upDotNormal > 0.0f ? xMath::Quat::Identity : xMath::Quat(1.0f, 0.0f, 0.0f, 0.0f);
			}
			else
			{
				float s                 = std::sqrt(2.0f + 2.0f * upDotNormal);
				xMath::Vec3 crossProd	= Cross(up,normal) / s;
				quatAlign               = xMath::Quat(crossProd.x, crossProd.y, crossProd.z, s * 0.5f);
			}

			float yaw = (static_cast<float>(yaw_Packed) / 255.0f) * xMath::TWO_PI;
			xMath::Quat quatYaw(0.0f, std::sin(-yaw * 0.5f), 0.0f, std::cos(-yaw * 0.5f));
			xMath::Quat quat = quatAlign * quatYaw;

			// compose scale
			float t = static_cast<float>(scale_Packed) / 255.0f;
			float scaleFloat = std::exp(std::lerp(std::log(0.01f), std::log(100.0f), t));

			// compose matrix
			return xMath::Matrix::CreateScale(scaleFloat) * xMath::Matrix::CreateRotation(quat) * xMath::Matrix::CreateTranslation(position);
		}

		/**
		 * @brief Packs a transformation matrix into the instance data format.
		 * @param matrix The transformation matrix to be packed.
		 */
		void SetMatrix(const xMath::Matrix& matrix)
		{
			// pack position
			xMath::Vec3 position	= matrix.GetTranslation();
			position_X				= FloatToHalf(position.x);
			position_Y				= FloatToHalf(position.y);
			position_Z				= FloatToHalf(position.z);

			// pack normal
			xMath::Quat quat	= matrix.GetRotation();
			xMath::Vec3 normal  = quat * xMath::Vec3::UP;
			normal_Oct          = EncodeOctahedral(normal);

			// pack yaw
			xMath::Vec3 up    = xMath::Vec3::UP;
			float upDotNormal = Dot(up,normal);
			xMath::Quat quatAlign;
			if (std::abs(upDotNormal) >= 0.999999f)
			{
				quatAlign = upDotNormal > 0.0f ? xMath::Quat::Identity : xMath::Quat(1.0f, 0.0f, 0.0f, 0.0f);
			}
			else
			{
				float s                 = std::sqrt(2.0f + 2.0f * upDotNormal);
				xMath::Vec3 crossProd	= Cross(up,normal) / s;
				quatAlign				= xMath::Quat(crossProd.x, crossProd.y, crossProd.z, s * 0.5f);
			}

			xMath::Quat quatYaw		= quatAlign.Conjugate() * quat;
			float halfAngle         = std::atan2(-quatYaw.y, quatYaw.w);
			float yaw               = halfAngle * 2.0f;
			if (yaw < 0.0f) yaw    += xMath::TWO_PI;
			yaw_Packed              = static_cast<uint8_t>((yaw / xMath::TWO_PI) * 255.0f);

			// pack scale
			float scaleAvg	= (matrix.GetScale().x + matrix.GetScale().y + matrix.GetScale().z) / 3.0f;
			scaleAvg		= std::max(0.01f, std::min(100.0f, scaleAvg));
			float t			= (std::log(scaleAvg) - std::log(0.01f)) / (std::log(100.0f) - std::log(0.01f));
			scale_Packed	= static_cast<uint8_t>(t * 255.0f);
		}

		/**
		 * @brief Returns an instance with identity transformation (position at origin, no rotation, scale of 1).
		 * @return An instance representing the identity transformation.
		 */
		static Instance GetIdentity()
		{
			Instance instance;
			instance.position_X   = 0;
			instance.position_Y   = 0;
			instance.position_Z   = 0;
			instance.normal_Oct   = 0;
			instance.yaw_Packed   = 0;
			instance.scale_Packed = 0;

			return instance;
		}

		/**
		 * @brief Encodes a 3D direction vector into a 16-bit octahedral representation.
		 * @param dir The 3D direction vector to be encoded.
		 * @return The 16-bit octahedral representation of the direction vector.
		 */
		static uint16_t EncodeOctahedral(const xMath::Vec3& dir)
		{
			xMath::Vec3 oct = dir / (std::abs(dir.x) + std::abs(dir.y) + std::abs(dir.z));
			if (oct.z < 0.0f)
			{
				float tempX = oct.x;
				oct.x = (1.0f - std::abs(oct.y)) * (tempX >= 0.0f ? 1.0f : -1.0f);
				oct.y = (1.0f - std::abs(tempX)) * (oct.y >= 0.0f ? 1.0f : -1.0f);
			}
			uint8_t ox = static_cast<uint8_t>(std::round((oct.x * 0.5f + 0.5f) * 255.0f));
			uint8_t oy = static_cast<uint8_t>(std::round((oct.y * 0.5f + 0.5f) * 255.0f));
			return (static_cast<uint16_t>(ox) << 8) | oy;
		}

		/**
		 * @brief Decodes a 16-bit octahedral representation into a 3D direction vector.
		 * @param packed The 16-bit octahedral representation to be decoded.
		 * @return The decoded 3D direction vector.
		 */
		static xMath::Vec3 DecodeOctahedral(uint16_t packed)
		{
			float x = (static_cast<float>(packed >> 8) / 255.0f) * 2.0f - 1.0f;
			float y = (static_cast<float>(packed & 0xFF) / 255.0f) * 2.0f - 1.0f;
			float z = 1.0f - std::abs(x) - std::abs(y);
			if (z < 0.0f)
			{
				float tempX = x;
				x = (1.0f - std::abs(y)) * (x >= 0.0f ? 1.0f : -1.0f);
				y = (1.0f - std::abs(tempX)) * (y >= 0.0f ? 1.0f : -1.0f);
			}
			xMath::Vec3 dir(x, y, z);
			Normalize(dir);
			return dir;
		}

		/**
		 * @brief Converts a 32-bit floating-point value to IEEE 754 16-bit half-precision floating-point value.
		 * @param value The 32-bit floating-point value to be converted.
		 * @return The 16-bit half-precision floating-point representation of the input value.
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
		 * @brief Converts an IEEE 754 16-bit half-precision floating-point value to a 32-bit floating-point value.
		 * @param value The 16-bit half-precision floating-point value to be converted.
		 * @return The 32-bit floating-point representation of the input value.
		 */
		static float HalfToFloat(uint16_t value)
		{
			// extract components
			uint32_t sign = (value & 0x8000) << 16;
			uint32_t exp  = (value >> 10) & 0x1F;
			uint32_t mant = value & 0x3FF;
		
			// handle inf/nan as 0
			if (exp == 0x1F)
				return 0.0f;

			// zero
			if (exp == 0 && mant == 0)
				return std::bit_cast<float>(sign);

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

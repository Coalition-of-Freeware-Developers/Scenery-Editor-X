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
 * xp_thumbnail.cpp
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#include "xp_thumbnail.h"
#include "xp_apt_geometry.h"
#include "xp_assets.h"
#include "xp_dsf.h"
#include "xp_text.h"
#include <SceneryEditorX/filesystem/vfs.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <optional>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace fs = std::filesystem;

	namespace
	{
		constexpr float kPi = 3.14159265358979323846f;
		constexpr float kDegToRad = kPi / 180.0f;

		// -------------------------------------------------------
		// DDS

		uint32_t Read32(const std::span<const uint8_t> bytes, const size_t offset)
		{
			return static_cast<uint32_t>(bytes[offset]) | static_cast<uint32_t>(bytes[offset + 1]) << 8 |
				static_cast<uint32_t>(bytes[offset + 2]) << 16 | static_cast<uint32_t>(bytes[offset + 3]) << 24;
		}

		constexpr uint32_t FourCC(const char a, const char b, const char c, const char d)
		{
			return static_cast<uint32_t>(static_cast<uint8_t>(a)) | static_cast<uint32_t>(static_cast<uint8_t>(b)) << 8 |
				static_cast<uint32_t>(static_cast<uint8_t>(c)) << 16 | static_cast<uint32_t>(static_cast<uint8_t>(d)) << 24;
		}

		enum class DdsFormat : uint8_t { Bc1, Bc2, Bc3, Bc4, Bc5, Masked, Unsupported };

		void Expand565(const uint16_t color, uint8_t out[4])
		{
			const uint32_t r = (color >> 11) & 31, g = (color >> 5) & 63, b = color & 31;
			out[0] = static_cast<uint8_t>((r << 3) | (r >> 2));
			out[1] = static_cast<uint8_t>((g << 2) | (g >> 4));
			out[2] = static_cast<uint8_t>((b << 3) | (b >> 2));
			out[3] = 255;
		}

		/* @brief A BC1 color block; BC2 / BC3 blocks always use four colors (bc1 false). */
		void DecodeColorBlock(const uint8_t *block, const bool bc1, uint8_t texels[16][4])
		{
			const uint16_t c0 = static_cast<uint16_t>(block[0] | block[1] << 8);
			const uint16_t c1 = static_cast<uint16_t>(block[2] | block[3] << 8);
			uint8_t palette[4][4];
			Expand565(c0, palette[0]);
			Expand565(c1, palette[1]);
			for (int c = 0; c < 3; ++c)
			{
				if (!bc1 || c0 > c1)
				{
					palette[2][c] = static_cast<uint8_t>((2 * palette[0][c] + palette[1][c] + 1) / 3);
					palette[3][c] = static_cast<uint8_t>((palette[0][c] + 2 * palette[1][c] + 1) / 3);
				}
				else
				{
					palette[2][c] = static_cast<uint8_t>((palette[0][c] + palette[1][c]) / 2);
					palette[3][c] = 0;
				}
			}
			palette[2][3] = 255;
			palette[3][3] = !bc1 || c0 > c1 ? 255 : 0;

			const uint32_t bits = static_cast<uint32_t>(block[4]) | static_cast<uint32_t>(block[5]) << 8 |
				static_cast<uint32_t>(block[6]) << 16 | static_cast<uint32_t>(block[7]) << 24;
			for (int i = 0; i < 16; ++i)
				std::memcpy(texels[i], palette[(bits >> (2 * i)) & 3], 4);
		}

		/* @brief A BC3 alpha / BC4 channel block: two endpoints and 3-bit indices. */
		void DecodeValueBlock(const uint8_t *block, uint8_t values[16])
		{
			const int a0 = block[0], a1 = block[1];
			uint8_t palette[8];
			palette[0] = static_cast<uint8_t>(a0);
			palette[1] = static_cast<uint8_t>(a1);
			if (a0 > a1)
			{
				for (int k = 2; k < 8; ++k)
					palette[k] = static_cast<uint8_t>(((8 - k) * a0 + (k - 1) * a1 + 3) / 7);
			}
			else
			{
				for (int k = 2; k < 6; ++k)
					palette[k] = static_cast<uint8_t>(((6 - k) * a0 + (k - 1) * a1 + 2) / 5);
				palette[6] = 0;
				palette[7] = 255;
			}
			uint64_t bits = 0;
			for (int i = 0; i < 6; ++i)
				bits |= static_cast<uint64_t>(block[2 + i]) << (8 * i);
			for (int i = 0; i < 16; ++i)
				values[i] = palette[(bits >> (3 * i)) & 7];
		}

		/* @brief A BC2 alpha block: 4 bits per texel. */
		void DecodeExplicitAlpha(const uint8_t *block, uint8_t values[16])
		{
			for (int i = 0; i < 16; ++i)
				values[i] = static_cast<uint8_t>(((block[i / 2] >> ((i & 1) * 4)) & 15) * 17);
		}

		struct MaskChannel
		{
			uint32_t mask = 0;
			int shift = 0;
			uint32_t max = 0;

			explicit MaskChannel(const uint32_t channelMask = 0) : mask(channelMask)
			{
				if (mask)
				{
					shift = std::countr_zero(mask);
					max = mask >> shift;
				}
			}

			[[nodiscard]] uint8_t Get(const uint32_t pixel, const uint8_t fallback) const
			{
				if (!mask || !max)
					return fallback;
				return static_cast<uint8_t>((static_cast<uint64_t>((pixel & mask) >> shift) * 255 + max / 2) / max);
			}
		};

		// -------------------------------------------------------
		// Images

		/* @brief Box-resamples to w x h (alpha-weighted color), keeping the row order. */
		RgbaImage Resample(const RgbaImage &source, const uint32_t width, const uint32_t height)
		{
			RgbaImage out;
			out.width = std::max(1u, width);
			out.height = std::max(1u, height);
			out.pixels.assign(static_cast<size_t>(out.width) * out.height * 4, 0);
			const double sx = static_cast<double>(source.width) / out.width;
			const double sy = static_cast<double>(source.height) / out.height;
			for (uint32_t y = 0; y < out.height; ++y)
			{
				const double y0 = y * sy, y1 = (y + 1) * sy;
				for (uint32_t x = 0; x < out.width; ++x)
				{
					const double x0 = x * sx, x1 = (x + 1) * sx;
					double sum[4] = { 0.0, 0.0, 0.0, 0.0 };
					double weightSum = 0.0;
					for (uint32_t j = static_cast<uint32_t>(y0); j < std::min<uint32_t>(source.height, static_cast<uint32_t>(std::ceil(y1))); ++j)
					{
						const double wy = std::min<double>(j + 1, y1) - std::max<double>(j, y0);
						for (uint32_t i = static_cast<uint32_t>(x0); i < std::min<uint32_t>(source.width, static_cast<uint32_t>(std::ceil(x1))); ++i)
						{
							const double w = wy * (std::min<double>(i + 1, x1) - std::max<double>(i, x0));
							if (w <= 0.0)
								continue;
							const uint8_t *p = &source.pixels[(static_cast<size_t>(j) * source.width + i) * 4];
							const double a = p[3] / 255.0;
							sum[0] += p[0] * a * w;
							sum[1] += p[1] * a * w;
							sum[2] += p[2] * a * w;
							sum[3] += a * w;
							weightSum += w;
						}
					}
					uint8_t *o = &out.pixels[(static_cast<size_t>(y) * out.width + x) * 4];
					if (weightSum > 0.0 && sum[3] > 0.0)
					{
						for (int c = 0; c < 3; ++c)
							o[c] = static_cast<uint8_t>(std::clamp(sum[c] / sum[3] + 0.5, 0.0, 255.0));
						o[3] = static_cast<uint8_t>(std::clamp(sum[3] / weightSum * 255.0 + 0.5, 0.0, 255.0));
					}
				}
			}
			return out;
		}

		// -------------------------------------------------------
		// Color

		struct ColorTables
		{
			std::array<float, 256> toLinear{};
			std::array<uint8_t, 4096> toSrgb{};

			ColorTables()
			{
				for (int i = 0; i < 256; ++i)
				{
					const float c = static_cast<float>(i) / 255.0f;
					toLinear[static_cast<size_t>(i)] = c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
				}
				for (int i = 0; i < 4096; ++i)
				{
					const float c = static_cast<float>(i) / 4095.0f;
					const float s = c <= 0.0031308f ? c * 12.92f : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
					toSrgb[static_cast<size_t>(i)] = static_cast<uint8_t>(std::clamp(s * 255.0f + 0.5f, 0.0f, 255.0f));
				}
			}
		};

		const ColorTables &GetColorTables()
		{
			static const ColorTables tables;
			return tables;
		}

		uint8_t ToSrgb8(const float linear)
		{
			return GetColorTables().toSrgb[static_cast<size_t>(std::clamp(linear, 0.0f, 1.0f) * 4095.0f + 0.5f)];
		}

		// -------------------------------------------------------
		// Rendering

		struct Vec3
		{
			float x = 0.0f, y = 0.0f, z = 0.0f;
		};

		Vec3 operator-(const Vec3 &a, const Vec3 &b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
		float Dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
		Vec3 Cross(const Vec3 &a, const Vec3 &b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
		Vec3 Normalize(const Vec3 &v)
		{
			const float length = std::sqrt(Dot(v, v));
			return length > 1e-12f ? Vec3{ v.x / length, v.y / length, v.z / length } : Vec3{ 0.0f, 1.0f, 0.0f };
		}

		/* @brief A texture as linear, premultiplied texels with its mip chain (row 0 at t = 0). */
		struct SampledTexture
		{
			struct Mip
			{
				uint32_t width = 0;
				uint32_t height = 0;
				std::vector<std::array<float, 4>> texels;
			};
			std::vector<Mip> mips;

			explicit SampledTexture(const RgbaImage &image)
			{
				if (!image.IsValid())
					return;
				const ColorTables &tables = GetColorTables();
				Mip base;
				base.width = image.width;
				base.height = image.height;
				base.texels.resize(static_cast<size_t>(image.width) * image.height);
				for (size_t i = 0; i < base.texels.size(); ++i)
				{
					const uint8_t *p = &image.pixels[i * 4];
					const float a = p[3] / 255.0f;
					base.texels[i] = { tables.toLinear[p[0]] * a, tables.toLinear[p[1]] * a, tables.toLinear[p[2]] * a, a };
				}
				mips.push_back(std::move(base));
				while (mips.back().width > 1 || mips.back().height > 1)
				{
					const Mip &previous = mips.back();
					Mip next;
					next.width = std::max(1u, previous.width / 2);
					next.height = std::max(1u, previous.height / 2);
					next.texels.resize(static_cast<size_t>(next.width) * next.height);
					for (uint32_t y = 0; y < next.height; ++y)
					{
						for (uint32_t x = 0; x < next.width; ++x)
						{
							std::array<float, 4> sum{};
							for (uint32_t j = 0; j < 2; ++j)
							{
								for (uint32_t i = 0; i < 2; ++i)
								{
									const uint32_t sx = std::min(previous.width - 1, x * 2 + i);
									const uint32_t sy = std::min(previous.height - 1, y * 2 + j);
									const std::array<float, 4> &t = previous.texels[static_cast<size_t>(sy) * previous.width + sx];
									for (int c = 0; c < 4; ++c)
										sum[static_cast<size_t>(c)] += t[static_cast<size_t>(c)] * 0.25f;
								}
							}
							next.texels[static_cast<size_t>(y) * next.width + x] = sum;
						}
					}
					mips.push_back(std::move(next));
				}
			}

			/* @brief Bilinear, wrapping. */
			[[nodiscard]] std::array<float, 4> Sample(const size_t level, const float u, const float v) const
			{
				const Mip &mip = mips[std::min(level, mips.size() - 1)];
				const float fx = u * static_cast<float>(mip.width) - 0.5f;
				const float fy = v * static_cast<float>(mip.height) - 0.5f;
				const float x0f = std::floor(fx), y0f = std::floor(fy);
				const float tx = fx - x0f, ty = fy - y0f;
				const auto wrap = [](const int64_t value, const uint32_t size) { const int64_t m = value % static_cast<int64_t>(size); return static_cast<uint32_t>(m < 0 ? m + size : m); };
				const int64_t x0 = static_cast<int64_t>(x0f), y0 = static_cast<int64_t>(y0f);
				const uint32_t xa = wrap(x0, mip.width), xb = wrap(x0 + 1, mip.width);
				const uint32_t ya = wrap(y0, mip.height), yb = wrap(y0 + 1, mip.height);
				const std::array<float, 4> &a = mip.texels[static_cast<size_t>(ya) * mip.width + xa];
				const std::array<float, 4> &b = mip.texels[static_cast<size_t>(ya) * mip.width + xb];
				const std::array<float, 4> &c = mip.texels[static_cast<size_t>(yb) * mip.width + xa];
				const std::array<float, 4> &d = mip.texels[static_cast<size_t>(yb) * mip.width + xb];
				std::array<float, 4> result{};
				for (size_t k = 0; k < 4; ++k)
					result[k] = (a[k] * (1.0f - tx) + b[k] * tx) * (1.0f - ty) + (c[k] * (1.0f - tx) + d[k] * tx) * ty;
				return result;
			}
		};

		/* @brief A vertex after projection: screen position, 1 / depth, and attributes divided by depth. */
		struct ScreenVertex
		{
			float x = 0.0f, y = 0.0f;
			float invDepth = 0.0f;
			float u = 0.0f, v = 0.0f;
			float nx = 0.0f, ny = 0.0f, nz = 0.0f;
		};

		struct Target
		{
			uint32_t size = 0;
			std::vector<std::array<float, 4>> color;	// Linear, premultiplied
			std::vector<float> depth;					// 1 / depth: larger is nearer, 0 empty
		};

		struct Shading
		{
			Vec3 toCamera;
			Vec3 toSun;
			float sun = 0.85f;
		};

		/* @brief A vertex in view space: right, up, distance in front of the camera, and its attributes. */
		struct ViewVertex
		{
			float x = 0.0f, y = 0.0f, depth = 0.0f;
			float u = 0.0f, v = 0.0f;
			float nx = 0.0f, ny = 0.0f, nz = 0.0f;
		};

		ViewVertex Lerp(const ViewVertex &a, const ViewVertex &b, const float t)
		{
			const auto mix = [t](const float p, const float q) { return p + (q - p) * t; };
			return { mix(a.x, b.x), mix(a.y, b.y), mix(a.depth, b.depth), mix(a.u, b.u), mix(a.v, b.v), mix(a.nx, b.nx), mix(a.ny, b.ny), mix(a.nz, b.nz) };
		}

		/* @brief Pixel coverage of a straight-alpha color over a linear premultiplied pixel. */
		void BlendOver(std::array<float, 4> &pixel, const std::array<float, 4> &color)
		{
			const float alpha = std::clamp(color[3], 0.0f, 1.0f);
			for (size_t k = 0; k < 3; ++k)
				pixel[k] = color[k] * alpha + pixel[k] * (1.0f - alpha);
			pixel[3] = alpha + pixel[3] * (1.0f - alpha);
		}

		/* @brief Linear premultiplied pixels to an 8-bit straight-alpha image (same size, row 0 first). */
		void ResolvePixels(const std::vector<std::array<float, 4>> &pixels, const uint32_t size, RgbaImage &out)
		{
			out.width = size;
			out.height = size;
			out.pixels.assign(static_cast<size_t>(size) * size * 4, 0);
			for (size_t i = 0; i < pixels.size() && i < static_cast<size_t>(size) * size; ++i)
			{
				const std::array<float, 4> &p = pixels[i];
				if (p[3] <= 1e-4f)
					continue;
				uint8_t *o = &out.pixels[i * 4];
				for (size_t k = 0; k < 3; ++k)
					o[k] = ToSrgb8(p[k] / p[3]);
				o[3] = static_cast<uint8_t>(std::clamp(p[3] * 255.0f + 0.5f, 0.0f, 255.0f));
			}
		}

		void DrawTriangle(Target &target, const ScreenVertex &a, const ScreenVertex &b, const ScreenVertex &c, const ThumbnailMesh &mesh,
			const SampledTexture *texture, const std::array<float, 4> &tint, const Shading &shading)
		{
			const float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
			if (std::abs(area) < 1e-9f)
				return;
			const int minX = std::max(0, static_cast<int>(std::floor(std::min({ a.x, b.x, c.x }))));
			const int maxX = std::min(static_cast<int>(target.size) - 1, static_cast<int>(std::ceil(std::max({ a.x, b.x, c.x }))));
			const int minY = std::max(0, static_cast<int>(std::floor(std::min({ a.y, b.y, c.y }))));
			const int maxY = std::min(static_cast<int>(target.size) - 1, static_cast<int>(std::ceil(std::max({ a.y, b.y, c.y }))));
			if (minX > maxX || minY > maxY)
				return;

			// One mip for the triangle: texels it covers per pixel it covers
			size_t level = 0;
			if (texture && !texture->mips.empty())
			{
				const float ua = a.u / a.invDepth, va = a.v / a.invDepth;
				const float ub = b.u / b.invDepth, vb = b.v / b.invDepth;
				const float uc = c.u / c.invDepth, vc = c.v / c.invDepth;
				const float uvArea = std::abs((ub - ua) * (vc - va) - (vb - va) * (uc - ua)) *
					static_cast<float>(texture->mips[0].width) * static_cast<float>(texture->mips[0].height);
				const float ratio = uvArea / std::abs(area);
				if (ratio > 1.0f)
					level = std::min(texture->mips.size() - 1, static_cast<size_t>(std::lround(0.5f * std::log2(ratio))));
			}

			const float invArea = 1.0f / area;
			for (int y = minY; y <= maxY; ++y)
			{
				const float py = static_cast<float>(y) + 0.5f;
				for (int x = minX; x <= maxX; ++x)
				{
					const float px = static_cast<float>(x) + 0.5f;
					float w0 = ((c.x - b.x) * (py - b.y) - (c.y - b.y) * (px - b.x)) * invArea;
					float w1 = ((a.x - c.x) * (py - c.y) - (a.y - c.y) * (px - c.x)) * invArea;
					float w2 = ((b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x)) * invArea;
					if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)
						continue;
					const float invDepth = w0 * a.invDepth + w1 * b.invDepth + w2 * c.invDepth;
					if (invDepth <= 0.0f)
						continue;
					const size_t index = static_cast<size_t>(y) * target.size + static_cast<size_t>(x);
					if (!mesh.draped && invDepth <= target.depth[index])
						continue;

					const float depth = 1.0f / invDepth;
					std::array<float, 4> albedo = { tint[0], tint[1], tint[2], tint[3] };	// Straight alpha
					if (texture && !texture->mips.empty())
					{
						const float u = (w0 * a.u + w1 * b.u + w2 * c.u) * depth;
						const float v = (w0 * a.v + w1 * b.v + w2 * c.v) * depth;
						const std::array<float, 4> texel = texture->Sample(level, u, v);
						const float alpha = texel[3];
						if (alpha > 1e-5f)
						{
							for (size_t k = 0; k < 3; ++k)
								albedo[k] *= texel[k] / alpha;
						}
						albedo[3] *= alpha;
					}
					if (!mesh.draped && albedo[3] < mesh.cutoff)
						continue;
					if (albedo[3] <= 0.0f)
						continue;

					// Sky light brighter from above plus the sun; faces lit from whichever side the camera sees
					float light = 1.0f;
					if (mesh.lit)
					{
						Vec3 normal = Normalize({ (w0 * a.nx + w1 * b.nx + w2 * c.nx) * depth, (w0 * a.ny + w1 * b.ny + w2 * c.ny) * depth,
							(w0 * a.nz + w1 * b.nz + w2 * c.nz) * depth });
						if (Dot(normal, shading.toCamera) < 0.0f)
							normal = { -normal.x, -normal.y, -normal.z };
						light = 0.32f + 0.18f * (0.5f + 0.5f * normal.y) + shading.sun * std::max(0.0f, Dot(normal, shading.toSun));
					}

					std::array<float, 4> &pixel = target.color[index];
					if (mesh.draped)
					{
						// Painter's order: later draped meshes go over earlier ones
						BlendOver(pixel, { albedo[0] * light, albedo[1] * light, albedo[2] * light, albedo[3] });
						if (albedo[3] > 0.5f)
							target.depth[index] = std::max(target.depth[index], invDepth);
					}
					else
					{
						for (size_t k = 0; k < 3; ++k)
							pixel[k] = albedo[k] * light;
						pixel[3] = 1.0f;
						target.depth[index] = invDepth;
					}
				}
			}
		}

		void TransformObjectPoint(const float in[3], const float heading, const float position[3], float out[3])
		{
			// Rotation about +y by -heading: an object's -z (its front) turns to the heading's direction
			const float radians = heading * kDegToRad;
			const float c = std::cos(radians), s = std::sin(radians);
			out[0] = in[0] * c - in[2] * s + position[0];
			out[1] = in[1] + position[1];
			out[2] = in[0] * s + in[2] * c + position[2];
		}

		std::string Extension(const std::string_view path)
		{
			const size_t dot = path.find_last_of('.');
			const size_t slash = path.find_last_of("/\\");
			if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash))
				return {};
			return ToLower(path.substr(dot));
		}

		/* @brief A square of ground under the asset (WorldEditor's lawn): draped, unlit, left out of the framing. */
		ThumbnailMesh GroundPlane(const double half, const std::array<float, 4> &color)
		{
			ThumbnailMesh mesh;
			const float h = static_cast<float>(half);
			const float corners[4][2] = { { -h, h }, { h, h }, { h, -h }, { -h, -h } };
			for (const auto &corner : corners)
			{
				ThumbnailVertex vertex;
				vertex.position[0] = corner[0];
				vertex.position[1] = -0.01f;
				vertex.position[2] = corner[1];
				mesh.vertices.push_back(vertex);
			}
			mesh.indices = { 0, 1, 2, 0, 2, 3 };
			std::copy(color.begin(), color.end(), mesh.color);
			mesh.draped = true;
			mesh.lit = false;
			mesh.frame = false;
			return mesh;
		}

		uint16_t Parameter(const double value)
		{
			return static_cast<uint16_t>(std::clamp(std::lround(value), 0l, 65535l));
		}
	}

	// -------------------------------------------------------

	bool DecodeDds(const std::span<const uint8_t> bytes, const uint32_t minSize, RgbaImage &out, std::string *error)
	{
		const auto fail = [&](const char *message)
		{
			if (error)
				*error = message;
			return false;
		};
		if (bytes.size() < 128 || std::memcmp(bytes.data(), "DDS ", 4) != 0)
			return fail("Not a DDS file");

		const uint32_t height = Read32(bytes, 12);
		const uint32_t width = Read32(bytes, 16);
		const uint32_t mipCount = std::max(1u, Read32(bytes, 28));
		const uint32_t pixelFlags = Read32(bytes, 80);
		const uint32_t fourCC = Read32(bytes, 84);
		uint32_t bitCount = Read32(bytes, 88);
		uint32_t masks[4] = { Read32(bytes, 92), Read32(bytes, 96), Read32(bytes, 100), Read32(bytes, 104) };
		if (width == 0 || height == 0 || width > 65536 || height > 65536)
			return fail("Bad DDS size");

		constexpr uint32_t DDPF_ALPHAPIXELS = 0x1, DDPF_ALPHA = 0x2, DDPF_FOURCC = 0x4, DDPF_RGB = 0x40, DDPF_LUMINANCE = 0x20000;
		size_t offset = 128;
		DdsFormat format = DdsFormat::Unsupported;
		bool hasAlpha = (pixelFlags & (DDPF_ALPHAPIXELS | DDPF_ALPHA)) != 0;
		bool luminance = (pixelFlags & DDPF_LUMINANCE) != 0;
		if (pixelFlags & DDPF_FOURCC)
		{
			if (fourCC == FourCC('D', 'X', 'T', '1'))
				format = DdsFormat::Bc1;
			else if (fourCC == FourCC('D', 'X', 'T', '2') || fourCC == FourCC('D', 'X', 'T', '3'))
				format = DdsFormat::Bc2;
			else if (fourCC == FourCC('D', 'X', 'T', '4') || fourCC == FourCC('D', 'X', 'T', '5'))
				format = DdsFormat::Bc3;
			else if (fourCC == FourCC('A', 'T', 'I', '1') || fourCC == FourCC('B', 'C', '4', 'U'))
				format = DdsFormat::Bc4;
			else if (fourCC == FourCC('A', 'T', 'I', '2') || fourCC == FourCC('B', 'C', '5', 'U'))
				format = DdsFormat::Bc5;
			else if (fourCC == FourCC('D', 'X', '1', '0'))
			{
				if (bytes.size() < 148)
					return fail("Truncated DDS header");
				offset = 148;
				switch (Read32(bytes, 128))
				{
				case 71: case 72: format = DdsFormat::Bc1; break;
				case 74: case 75: format = DdsFormat::Bc2; break;
				case 77: case 78: format = DdsFormat::Bc3; break;
				case 80: format = DdsFormat::Bc4; break;
				case 83: format = DdsFormat::Bc5; break;
				case 28: case 29:	// R8G8B8A8
					format = DdsFormat::Masked;
					bitCount = 32;
					masks[0] = 0x000000FF; masks[1] = 0x0000FF00; masks[2] = 0x00FF0000; masks[3] = 0xFF000000;
					hasAlpha = true;
					luminance = false;
					break;
				case 87: case 91:	// B8G8R8A8
				case 88: case 93:	// B8G8R8X8
				{
					const uint32_t dxgi = Read32(bytes, 128);
					format = DdsFormat::Masked;
					bitCount = 32;
					masks[0] = 0x00FF0000; masks[1] = 0x0000FF00; masks[2] = 0x000000FF; masks[3] = 0xFF000000;
					hasAlpha = dxgi == 87 || dxgi == 91;
					luminance = false;
					break;
				}
				default: break;
				}
			}
		}
		else if ((pixelFlags & (DDPF_RGB | DDPF_LUMINANCE | DDPF_ALPHA)) && bitCount >= 8 && bitCount <= 32 && bitCount % 8 == 0)
			format = DdsFormat::Masked;
		if (format == DdsFormat::Unsupported)
			return fail("DDS format not decoded (BC7 and floating-point formats have no thumbnail)");

		const bool compressed = format != DdsFormat::Masked;
		const uint32_t blockBytes = format == DdsFormat::Bc1 || format == DdsFormat::Bc4 ? 8 : 16;
		const uint32_t pixelBytes = bitCount / 8;
		const auto levelBytes = [&](const uint32_t w, const uint32_t h) -> size_t
		{
			return compressed ? static_cast<size_t>((w + 3) / 4) * ((h + 3) / 4) * blockBytes : static_cast<size_t>(w) * h * pixelBytes;
		};

		// The smallest level still at least minSize on its longer side
		uint32_t levelWidth = width, levelHeight = height;
		size_t levelOffset = offset;
		{
			uint32_t w = width, h = height;
			size_t at = offset;
			for (uint32_t level = 0; level < mipCount; ++level)
			{
				if (level > 0 && std::max(w, h) < minSize)
					break;
				if (at + levelBytes(w, h) > bytes.size())
					break;
				levelWidth = w;
				levelHeight = h;
				levelOffset = at;
				at += levelBytes(w, h);
				w = std::max(1u, w / 2);
				h = std::max(1u, h / 2);
			}
		}
		if (levelOffset + levelBytes(levelWidth, levelHeight) > bytes.size())
			return fail("Truncated DDS file");

		out.width = levelWidth;
		out.height = levelHeight;
		out.pixels.assign(static_cast<size_t>(levelWidth) * levelHeight * 4, 255);
		const uint8_t *data = bytes.data() + levelOffset;

		if (!compressed)
		{
			const MaskChannel r(masks[0]), g(masks[1]), b(masks[2]), a(hasAlpha ? masks[3] : 0u);
			const bool alphaOnly = (pixelFlags & DDPF_ALPHA) && !(pixelFlags & (DDPF_RGB | DDPF_LUMINANCE));
			for (size_t i = 0; i < static_cast<size_t>(levelWidth) * levelHeight; ++i)
			{
				uint32_t pixel = 0;
				for (uint32_t k = 0; k < pixelBytes; ++k)
					pixel |= static_cast<uint32_t>(data[i * pixelBytes + k]) << (8 * k);
				uint8_t *o = &out.pixels[i * 4];
				if (alphaOnly)
				{
					o[0] = o[1] = o[2] = 255;
					o[3] = a.Get(pixel, 255);
					continue;
				}
				o[0] = r.Get(pixel, 0);
				o[1] = luminance ? o[0] : g.Get(pixel, 0);
				o[2] = luminance ? o[0] : b.Get(pixel, 0);
				o[3] = a.Get(pixel, 255);
			}
			return true;
		}

		const uint32_t blocksX = (levelWidth + 3) / 4, blocksY = (levelHeight + 3) / 4;
		for (uint32_t by = 0; by < blocksY; ++by)
		{
			for (uint32_t bx = 0; bx < blocksX; ++bx)
			{
				const uint8_t *block = data + (static_cast<size_t>(by) * blocksX + bx) * blockBytes;
				uint8_t texels[16][4];
				uint8_t first[16], second[16];
				switch (format)
				{
				case DdsFormat::Bc1:
					DecodeColorBlock(block, true, texels);
					break;
				case DdsFormat::Bc2:
					DecodeColorBlock(block + 8, false, texels);
					DecodeExplicitAlpha(block, first);
					for (int i = 0; i < 16; ++i)
						texels[i][3] = first[i];
					break;
				case DdsFormat::Bc3:
					DecodeColorBlock(block + 8, false, texels);
					DecodeValueBlock(block, first);
					for (int i = 0; i < 16; ++i)
						texels[i][3] = first[i];
					break;
				case DdsFormat::Bc4:
					DecodeValueBlock(block, first);
					for (int i = 0; i < 16; ++i)
					{
						texels[i][0] = texels[i][1] = texels[i][2] = first[i];
						texels[i][3] = 255;
					}
					break;
				default:	// Bc5
					DecodeValueBlock(block, first);
					DecodeValueBlock(block + 8, second);
					for (int i = 0; i < 16; ++i)
					{
						texels[i][0] = first[i];
						texels[i][1] = second[i];
						texels[i][2] = 0;
						texels[i][3] = 255;
					}
					break;
				}
				for (uint32_t j = 0; j < 4; ++j)
				{
					const uint32_t y = by * 4 + j;
					if (y >= levelHeight)
						break;
					for (uint32_t i = 0; i < 4; ++i)
					{
						const uint32_t x = bx * 4 + i;
						if (x >= levelWidth)
							break;
						std::memcpy(&out.pixels[(static_cast<size_t>(y) * levelWidth + x) * 4], texels[j * 4 + i], 4);
					}
				}
			}
		}
		return true;
	}

	void FlipRows(RgbaImage &image)
	{
		if (!image.IsValid())
			return;
		const size_t pitch = static_cast<size_t>(image.width) * 4;
		std::vector<uint8_t> row(pitch);
		for (uint32_t y = 0; y < image.height / 2; ++y)
		{
			uint8_t *top = &image.pixels[y * pitch];
			uint8_t *bottom = &image.pixels[(image.height - 1 - y) * pitch];
			std::memcpy(row.data(), top, pitch);
			std::memcpy(top, bottom, pitch);
			std::memcpy(bottom, row.data(), pitch);
		}
	}

	void ShrinkImage(RgbaImage &image, const uint32_t maxSize)
	{
		while (image.IsValid() && std::max(image.width, image.height) > std::max(1u, maxSize))
			image = Resample(image, std::max(1u, image.width / 2), std::max(1u, image.height / 2));
	}

	bool FitThumbnailImage(const RgbaImage &image, const uint32_t size, RgbaImage &out, const float aspect)
	{
		if (!image.IsValid() || size == 0)
			return false;
		RgbaImage source = image;
		ShrinkImage(source, size * 2);
		const double shown = aspect > 0.0f ? aspect : static_cast<double>(source.width) / source.height;
		const uint32_t width = shown >= 1.0 ? size : std::clamp<uint32_t>(static_cast<uint32_t>(std::lround(size * shown)), 1u, size);
		const uint32_t height = shown >= 1.0 ? std::clamp<uint32_t>(static_cast<uint32_t>(std::lround(size / shown)), 1u, size) : size;
		const RgbaImage fitted = Resample(source, width, height);

		out.width = size;
		out.height = size;
		out.pixels.assign(static_cast<size_t>(size) * size * 4, 0);
		const uint32_t left = (size - width) / 2, top = (size - height) / 2;
		for (uint32_t y = 0; y < height; ++y)
		{
			// Row 0 of the image is its bottom
			const uint8_t *row = &fitted.pixels[static_cast<size_t>(height - 1 - y) * width * 4];
			std::memcpy(&out.pixels[(static_cast<size_t>(top + y) * size + left) * 4], row, static_cast<size_t>(width) * 4);
		}
		return true;
	}

	bool RenderLineThumbnail(const LineAsset &asset, const RgbaImage &texture, const uint32_t size, RgbaImage &out)
	{
		if (!texture.IsValid() || asset.segments.empty() || size == 0 || asset.scaleT <= 0.0f)
			return false;
		const SampledTexture sampled(texture);
		const ColorTables &tables = GetColorTables();
		std::vector<std::array<float, 4>> pixels(static_cast<size_t>(size) * size, { 0.0f, 0.0f, 0.0f, 0.0f });

		// Dark paint (WorldEditor: luminance under 50 of 255) goes on a light background
		double luminance = 0.0;
		size_t counted = 0;
		for (const LineAsset::Segment &segment : asset.segments)
		{
			const uint32_t x0 = static_cast<uint32_t>(std::clamp(segment.s1, 0.0f, 1.0f) * static_cast<float>(texture.width - 1));
			const uint32_t x1 = static_cast<uint32_t>(std::clamp(segment.s2, 0.0f, 1.0f) * static_cast<float>(texture.width - 1));
			for (uint32_t y = 0; y < texture.height; ++y)
			{
				for (uint32_t x = std::min(x0, x1); x <= std::max(x0, x1); ++x)
				{
					const uint8_t *p = &texture.pixels[(static_cast<size_t>(y) * texture.width + x) * 4];
					if (p[3] <= 30)
						continue;
					luminance += 0.2 * p[0] + 0.5 * p[1] + 0.3 * p[2];
					++counted;
				}
			}
		}
		if (counted > 0 && luminance / static_cast<double>(counted) < 50.0)
		{
			for (uint32_t y = 0; y < size; ++y)
			{
				const float shade = tables.toLinear[static_cast<size_t>(235 - 40 * y / std::max(1u, size - 1))];
				for (uint32_t x = 0; x < size; ++x)
					pixels[static_cast<size_t>(y) * size + x] = { shade, shade, shade, 1.0f };
			}
		}

		// One repeat from the bottom (t = 0) to the top; widths relative to the repeat's length
		const float fullSize = static_cast<float>(size);
		const float center = fullSize * 0.5f;
		const float metersAcross = asset.scaleS / asset.scaleT;
		for (const LineAsset::Segment &segment : asset.segments)
		{
			const float left = center + (segment.s1 - segment.sm) * metersAcross * fullSize;
			const float right = center + (segment.s2 - segment.sm) * metersAcross * fullSize;
			if (right - left <= 1e-4f)
				continue;
			const int first = std::max(0, static_cast<int>(std::floor(left)));
			const int last = std::min(static_cast<int>(size) - 1, static_cast<int>(std::ceil(right)) - 1);
			for (int x = first; x <= last; ++x)
			{
				// Partial coverage at the strip's edges
				const float coverage = std::clamp(std::min(right, static_cast<float>(x) + 1.0f) - std::max(left, static_cast<float>(x)), 0.0f, 1.0f);
				const float s = segment.s1 + (segment.s2 - segment.s1) * ((static_cast<float>(x) + 0.5f - left) / (right - left));
				for (uint32_t y = 0; y < size; ++y)
				{
					const float t = (fullSize - (static_cast<float>(y) + 0.5f)) / fullSize;
					const std::array<float, 4> texel = sampled.Sample(0, s, t);
					if (texel[3] <= 1e-5f)
						continue;
					BlendOver(pixels[static_cast<size_t>(y) * size + static_cast<size_t>(x)],
						{ texel[0] / texel[3], texel[1] / texel[3], texel[2] / texel[3], texel[3] * coverage });
				}
			}
		}
		ResolvePixels(pixels, size, out);
		return true;
	}

	bool RenderThumbnail(const ThumbnailScene &scene, const ThumbnailOptions &options, RgbaImage &out)
	{
		const uint32_t size = std::max(1u, options.size);
		const uint32_t samples = std::clamp(options.supersample, 1u, 4u);

		// Bounds of what is framed
		Vec3 low{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
		Vec3 high{ -low.x, -low.y, -low.z };
		size_t triangles = 0;
		for (const ThumbnailMesh &mesh : scene.meshes)
		{
			if (mesh.indices.size() < 3)
				continue;
			triangles += mesh.indices.size() / 3;
			if (!mesh.frame)
				continue;
			for (const uint32_t index : mesh.indices)
			{
				if (index >= mesh.vertices.size())
					continue;
				const float *p = mesh.vertices[index].position;
				low = { std::min(low.x, p[0]), std::min(low.y, p[1]), std::min(low.z, p[2]) };
				high = { std::max(high.x, p[0]), std::max(high.y, p[1]), std::max(high.z, p[2]) };
			}
		}
		if (triangles == 0 || low.x > high.x)
			return false;
		const Vec3 center{ (low.x + high.x) * 0.5f, (low.y + high.y) * 0.5f, (low.z + high.z) * 0.5f };

		// Camera: from the south turned toward the east and above (steeper for draped art); light from the viewer's upper left
		const bool ground = options.framing == ThumbnailFraming::Ground;
		const float azimuth = (ground ? 15.0f : 30.0f) * kDegToRad;		// From south toward east
		const float elevation = (ground ? 62.0f : 20.0f) * kDegToRad;
		const Vec3 toCamera{ std::cos(elevation) * std::sin(azimuth), std::sin(elevation), std::cos(elevation) * std::cos(azimuth) };
		const Vec3 right = Normalize({ toCamera.z, 0.0f, -toCamera.x });
		const Vec3 up = Cross(right, { -toCamera.x, -toCamera.y, -toCamera.z });
		const float tanHalf = std::tan(12.5f * kDegToRad);

		// Distance that keeps every framed vertex in a 25 degree view, then the screen bounds to fill
		const auto isFramed = [](const ThumbnailMesh &mesh) { return mesh.frame && mesh.indices.size() >= 3; };
		float distance = 0.0f;
		float radius = 0.0f;
		for (const ThumbnailMesh &mesh : scene.meshes)
		{
			if (!isFramed(mesh))
				continue;
			for (const ThumbnailVertex &vertex : mesh.vertices)
			{
				const Vec3 q = Vec3{ vertex.position[0], vertex.position[1], vertex.position[2] } - center;
				const float extent = std::max(std::abs(Dot(q, right)), std::abs(Dot(q, up)));
				distance = std::max(distance, Dot(q, toCamera) + extent / tanHalf);
				radius = std::max(radius, std::sqrt(Dot(q, q)));
			}
		}
		distance += 0.05f * radius + 1e-3f;
		float minX = std::numeric_limits<float>::max(), minY = minX, maxX = -minX, maxY = -minX;
		for (const ThumbnailMesh &mesh : scene.meshes)
		{
			if (!isFramed(mesh))
				continue;
			for (const ThumbnailVertex &vertex : mesh.vertices)
			{
				const Vec3 q = Vec3{ vertex.position[0], vertex.position[1], vertex.position[2] } - center;
				const float depth = distance - Dot(q, toCamera);
				const float sx = Dot(q, right) / depth, sy = Dot(q, up) / depth;
				minX = std::min(minX, sx);
				maxX = std::max(maxX, sx);
				minY = std::min(minY, sy);
				maxY = std::max(maxY, sy);
			}
		}
		const float span = std::max(maxX - minX, maxY - minY);
		if (!(span > 1e-9f))
			return false;

		Target target;
		target.size = size * samples;
		target.color.assign(static_cast<size_t>(target.size) * target.size, { 0.0f, 0.0f, 0.0f, 0.0f });
		target.depth.assign(static_cast<size_t>(target.size) * target.size, 0.0f);
		const float pixels = static_cast<float>(target.size);
		const float scale = (1.0f - 2.0f * std::clamp(options.margin, 0.0f, 0.4f)) * pixels / span;
		const float offsetX = pixels * 0.5f - scale * (minX + maxX) * 0.5f;
		const float offsetY = pixels * 0.5f + scale * (minY + maxY) * 0.5f;
		const float nearDepth = std::max(1e-3f, 0.02f * distance);	// Ground planes reaching behind the camera are cut here

		Shading shading;
		shading.toCamera = toCamera;
		shading.toSun = Normalize({ toCamera.x + up.x - right.x, toCamera.y + up.y - right.y, toCamera.z + up.z - right.z });

		std::vector<SampledTexture> textures;
		textures.reserve(scene.textures.size());
		for (const RgbaImage &image : scene.textures)
			textures.emplace_back(image);

		const ColorTables &tables = GetColorTables();
		const auto project = [&](const ViewVertex &v)
		{
			const float invDepth = 1.0f / v.depth;
			ScreenVertex s;
			s.x = offsetX + scale * v.x * invDepth;
			s.y = offsetY - scale * v.y * invDepth;
			s.invDepth = invDepth;
			s.u = v.u * invDepth;
			s.v = v.v * invDepth;
			s.nx = v.nx * invDepth;
			s.ny = v.ny * invDepth;
			s.nz = v.nz * invDepth;
			return s;
		};
		std::vector<ViewVertex> view;
		const auto drawMesh = [&](const ThumbnailMesh &mesh)
		{
			view.resize(mesh.vertices.size());
			for (size_t i = 0; i < mesh.vertices.size(); ++i)
			{
				const ThumbnailVertex &vertex = mesh.vertices[i];
				const Vec3 q = Vec3{ vertex.position[0], vertex.position[1], vertex.position[2] } - center;
				view[i] = { Dot(q, right), Dot(q, up), distance - Dot(q, toCamera), vertex.uv[0], vertex.uv[1], vertex.normal[0], vertex.normal[1], vertex.normal[2] };
			}
			const SampledTexture *texture = mesh.texture >= 0 && static_cast<size_t>(mesh.texture) < textures.size() ? &textures[static_cast<size_t>(mesh.texture)] : nullptr;
			const auto channel = [&](const float value) { return tables.toLinear[static_cast<size_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f)]; };
			const std::array<float, 4> tint = { channel(mesh.color[0]), channel(mesh.color[1]), channel(mesh.color[2]), std::clamp(mesh.color[3], 0.0f, 1.0f) };
			for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
			{
				const uint32_t a = mesh.indices[i], b = mesh.indices[i + 1], c = mesh.indices[i + 2];
				if (a >= view.size() || b >= view.size() || c >= view.size())
					continue;
				const ViewVertex corners[3] = { view[a], view[b], view[c] };
				if (corners[0].depth >= nearDepth && corners[1].depth >= nearDepth && corners[2].depth >= nearDepth)
				{
					DrawTriangle(target, project(corners[0]), project(corners[1]), project(corners[2]), mesh, texture, tint, shading);
					continue;
				}

				// Cut at the near plane: what is left is a triangle or a quad
				ViewVertex clipped[4];
				int count = 0;
				for (int k = 0; k < 3; ++k)
				{
					const ViewVertex &p = corners[k];
					const ViewVertex &q = corners[(k + 1) % 3];
					const bool pIn = p.depth >= nearDepth, qIn = q.depth >= nearDepth;
					if (pIn)
						clipped[count++] = p;
					if (pIn != qIn && count < 4)
						clipped[count++] = Lerp(p, q, (nearDepth - p.depth) / (q.depth - p.depth));
				}
				if (count >= 3)
					DrawTriangle(target, project(clipped[0]), project(clipped[1]), project(clipped[2]), mesh, texture, tint, shading);
				if (count == 4)
					DrawTriangle(target, project(clipped[0]), project(clipped[2]), project(clipped[3]), mesh, texture, tint, shading);
			}
		};
		for (const ThumbnailMesh &mesh : scene.meshes)
		{
			if (mesh.draped)
				drawMesh(mesh);
		}
		for (const ThumbnailMesh &mesh : scene.meshes)
		{
			if (!mesh.draped)
				drawMesh(mesh);
		}

		// Average the samples of each pixel; row 0 is the top
		std::vector<std::array<float, 4>> averaged(static_cast<size_t>(size) * size, { 0.0f, 0.0f, 0.0f, 0.0f });
		const float weight = 1.0f / static_cast<float>(samples * samples);
		bool drawn = false;
		for (uint32_t y = 0; y < size; ++y)
		{
			for (uint32_t x = 0; x < size; ++x)
			{
				std::array<float, 4> &sum = averaged[static_cast<size_t>(y) * size + x];
				for (uint32_t j = 0; j < samples; ++j)
				{
					for (uint32_t i = 0; i < samples; ++i)
					{
						const std::array<float, 4> &sample = target.color[static_cast<size_t>(y * samples + j) * target.size + x * samples + i];
						for (size_t k = 0; k < 4; ++k)
							sum[k] += sample[k] * weight;
					}
				}
				drawn = drawn || sum[3] > 1e-4f;
			}
		}
		ResolvePixels(averaged, size, out);
		return drawn;
	}

	bool IsThumbnailImageFile(const std::string_view path)
	{
		const std::string extension = Extension(path);
		return extension == ".dds" || extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".tga" || extension == ".bmp";
	}

	bool CanMakeThumbnail(const std::string_view path)
	{
		const std::string extension = Extension(path);
		return IsThumbnailImageFile(path) || extension == ".obj" || extension == ".fac" || extension == ".pol" || extension == ".lin" ||
			extension == ".str" || extension == ".for" || extension == ".agp";
	}

	fs::path ResolveThumbnailTexture(const fs::path &baseFolder, const std::string_view reference)
	{
		if (Trim(reference).empty())
			return {};
		const fs::path named = (baseFolder / ToNativeRelativePath(reference)).lexically_normal();
		fs::path dds = named;
		dds.replace_extension(".dds");
		if (const std::optional<fs::path> found = VFS::FindCaseInsensitive(dds); found && VFS::IsFile(*found))
			return *found;
		if (const std::optional<fs::path> found = VFS::FindCaseInsensitive(named); found && VFS::IsFile(*found))
			return *found;
		return {};
	}

	// -------------------------------------------------------

	ThumbnailBuilder::ThumbnailBuilder(AssetResolver resolver, ThumbnailTextureLoader textures) : m_Resolver(std::move(resolver)), m_Textures(std::move(textures))
	{
	}

	ThumbnailBuilder::~ThumbnailBuilder() = default;

	void ThumbnailBuilder::ClearCache()
	{
		m_Overlay.reset();
		m_Objects.clear();
		m_TextureCache.clear();
	}

	int ThumbnailBuilder::LoadTexture(const fs::path &file, ThumbnailScene &scene)
	{
		if (file.empty() || !m_Textures)
			return -1;
		if (const auto it = m_SceneTextures.find(file); it != m_SceneTextures.end())
			return it->second;

		std::shared_ptr<const RgbaImage> image;
		if (const auto it = m_TextureCache.find(file); it != m_TextureCache.end())
			image = it->second;
		else
		{
			auto loaded = std::make_shared<RgbaImage>();
			if (!m_Textures(file, m_TextureSize, *loaded) || !loaded->IsValid())
				loaded.reset();
			else
				ShrinkImage(*loaded, m_TextureSize);
			image = loaded;
			m_TextureCache[file] = image;
		}
		const int index = image ? static_cast<int>(scene.textures.size()) : -1;
		if (image)
			scene.textures.push_back(*image);
		m_SceneTextures[file] = index;
		return index;
	}

	const Obj8Data *ThumbnailBuilder::GetObject(const fs::path &file)
	{
		const auto it = m_Objects.find(file);
		if (it != m_Objects.end())
			return it->second.get();
		auto object = std::make_unique<Obj8Data>();
		if (!LoadObj8(file, *object))
			object.reset();
		return m_Objects.emplace(file, std::move(object)).first->second.get();
	}

	void ThumbnailBuilder::AddObject(const Obj8Data &object, const fs::path &objectFile, const float position[3], const float heading, ThumbnailScene &scene)
	{
		const fs::path folder = objectFile.parent_path();
		const int texture = LoadTexture(ResolveThumbnailTexture(folder, object.texture), scene);
		const int drapedTexture = object.textureDraped.empty() ? texture : LoadTexture(ResolveThumbnailTexture(folder, object.textureDraped), scene);

		// At rest: every dataref reads 0
		const DatarefReader rest = [](const DatarefRef &) { return 0.0f; };
		std::vector<RigidTransform> nodes(object.nodes.size());
		std::vector<bool> evaluated(object.nodes.size(), false);
		const auto nodeTransform = [&](const auto &self, const int node) -> RigidTransform
		{
			if (node < 0 || static_cast<size_t>(node) >= object.nodes.size())
				return {};
			if (!evaluated[static_cast<size_t>(node)])
			{
				const RigidTransform local = EvaluateAnimNode(object.nodes[static_cast<size_t>(node)], rest);
				const int parent = object.nodes[static_cast<size_t>(node)].parent;
				nodes[static_cast<size_t>(node)] = parent >= 0 && parent != node ? self(self, parent).Compose(local) : local;
				evaluated[static_cast<size_t>(node)] = true;
			}
			return nodes[static_cast<size_t>(node)];
		};

		// The nearest LOD (every LOD starting at the nearest distance, for additive LODs)
		float nearest = std::numeric_limits<float>::max();
		for (const Obj8Lod &lod : object.lods)
			nearest = std::min(nearest, lod.nearDistance);

		std::vector<uint32_t> remap(object.vertices.size(), ~0u);
		std::vector<uint32_t> used;
		for (const Obj8Batch &batch : object.batches)
		{
			if (!batch.state.drawEnabled)
				continue;
			if (batch.lod >= 0 && static_cast<size_t>(batch.lod) < object.lods.size() && object.lods[static_cast<size_t>(batch.lod)].nearDistance > nearest)
				continue;
			if (!batch.visibility.empty() && !EvaluateVisibility(object, batch.visibility, rest))
				continue;

			const RigidTransform transform = nodeTransform(nodeTransform, batch.node);
			ThumbnailMesh mesh;
			mesh.draped = batch.state.draped;
			mesh.texture = mesh.draped ? drapedTexture : texture;
			mesh.color[0] = mesh.color[1] = mesh.color[2] = 1.0f;
			mesh.cutoff = batch.state.blend ? 0.5f : batch.state.alphaCutoff;
			for (const Obj8Batch::Range &range : batch.ranges)
			{
				for (uint32_t i = range.offset; i < range.offset + range.count && i < object.indices.size(); ++i)
				{
					const uint32_t source = object.indices[i];
					if (source >= object.vertices.size())
						continue;
					if (remap[source] == ~0u)
					{
						const Obj8Vertex &v = object.vertices[source];
						ThumbnailVertex vertex;
						float moved[3], normal[3];
						transform.TransformPoint(v.position, moved);
						transform.TransformPoint(v.normal, normal);
						for (int k = 0; k < 3; ++k)
							normal[k] -= transform.translation[k];
						TransformObjectPoint(moved, heading, position, vertex.position);
						const float zero[3] = { 0.0f, 0.0f, 0.0f };
						TransformObjectPoint(normal, heading, zero, vertex.normal);
						vertex.uv[0] = v.uv[0];
						vertex.uv[1] = v.uv[1];
						remap[source] = static_cast<uint32_t>(mesh.vertices.size());
						mesh.vertices.push_back(vertex);
						used.push_back(source);
					}
					mesh.indices.push_back(remap[source]);
				}
			}
			for (const uint32_t source : used)
				remap[source] = ~0u;
			used.clear();
			if (mesh.indices.size() >= 3)
				scene.meshes.push_back(std::move(mesh));
		}
	}

	void ThumbnailBuilder::AddOverlay(const OverlayBuild &build, ThumbnailScene &scene)
	{
		for (const OverlayMesh &source : build.meshes)
		{
			if (source.indices.size() < 3)
				continue;
			ThumbnailMesh mesh;
			mesh.vertices.reserve(source.vertices.size());
			for (const OverlayVertex &v : source.vertices)
			{
				ThumbnailVertex vertex;
				vertex.position[0] = v.x;
				vertex.position[1] = v.y;
				vertex.position[2] = v.z;
				vertex.normal[0] = v.nx;
				vertex.normal[1] = v.ny;
				vertex.normal[2] = v.nz;
				vertex.uv[0] = v.u;
				vertex.uv[1] = v.v;
				mesh.vertices.push_back(vertex);
			}
			mesh.indices = source.indices;
			if (!source.texture.empty())
				mesh.texture = LoadTexture(ResolveThumbnailTexture(source.texture.parent_path(), VFS::ToUtf8(source.texture.filename())), scene);
			std::copy(std::begin(source.color), std::end(source.color), mesh.color);
			mesh.cutoff = source.cutout ? source.cutoff : 0.1f;
			mesh.draped = source.kind != OverlayKind::Facade && source.kind != OverlayKind::Forest && source.kind != OverlayKind::Road;
			scene.meshes.push_back(std::move(mesh));
		}
		for (const OverlayObject &placed : build.objects)
		{
			if (placed.file.empty())
				continue;
			if (const Obj8Data *object = GetObject(placed.file))
			{
				const float position[3] = { static_cast<float>(placed.x), static_cast<float>(placed.y), static_cast<float>(placed.z) };
				AddObject(*object, placed.file, position, placed.heading, scene);
			}
		}
	}

	void ThumbnailBuilder::AddForest(const ForestAsset &forest, const fs::path &forestFile, ThumbnailScene &scene)
	{
		if (forest.trees.empty())
			return;
		ThumbnailMesh mesh;
		mesh.texture = LoadTexture(ResolveThumbnailTexture(forestFile.parent_path(), forest.texture), scene);
		mesh.color[0] = mesh.color[1] = mesh.color[2] = 1.0f;
		mesh.cutoff = 0.5f;

		// WorldEditor's preview: each layer fills a grid of trees, kinds by their percentages (the first kind fills the rest)
		std::map<int, std::vector<const ForestAsset::Tree *>> layers;
		for (const ForestAsset::Tree &tree : forest.trees)
			layers[tree.layer].push_back(&tree);
		const int perRow = layers.begin()->second.size() < 4 ? 3 : 6;
		const int cells = perRow * perRow;
		uint32_t seed = 12345u;
		const auto random = [&seed]
		{
			seed = seed * 1664525u + 1013904223u;
			return static_cast<float>((seed >> 8) & 0xFFFFFF) / 16777216.0f;
		};
		const float scaleX = std::max(1.0f, forest.scaleX), scaleY = std::max(1.0f, forest.scaleY);

		for (const auto &[layer, trees] : layers)
		{
			std::vector<size_t> kinds(static_cast<size_t>(cells), 0);
			float total = 0.0f;
			for (const ForestAsset::Tree *tree : trees)
				total += tree->percent;
			for (size_t kind = trees.size() - 1; kind > 0 && total > 0.0f; --kind)
			{
				const int count = static_cast<int>(std::lround(trees[kind]->percent / total * static_cast<float>(cells)));
				for (int placed = 0; placed < count; ++placed)
				{
					for (int attempt = 0; attempt < 20; ++attempt)
					{
						const size_t where = std::min(static_cast<size_t>(cells) - 1, static_cast<size_t>(random() * static_cast<float>(cells)));
						if (kinds[where] == 0)
						{
							kinds[where] = kind;
							break;
						}
					}
				}
			}

			for (int cell = 0; cell < cells; ++cell)
			{
				const ForestAsset::Tree &tree = *trees[kinds[static_cast<size_t>(cell)]];
				const float height = tree.minHeight + (tree.maxHeight - tree.minHeight) * random();
				const float x = static_cast<float>(cell % perRow) * forest.spacingX + forest.randomX * (2.0f * random() - 1.0f);
				const float z = static_cast<float>(cell / perRow) * forest.spacingY + forest.randomY * (2.0f * random() - 1.0f);
				const float turn = random();
				const float width = std::abs(height / tree.height * tree.width);
				const float offset = tree.offset / tree.width;		// Where the trunk is across the quad
				for (int quad = 0; quad < tree.quads; ++quad)
				{
					const float angle = kPi * (turn + static_cast<float>(quad) / static_cast<float>(tree.quads));
					const float dx = width * std::sin(angle), dz = width * std::cos(angle);
					const float s0 = tree.s / scaleX, s1 = (tree.s + tree.width) / scaleX;
					const float t0 = tree.t / scaleY, t1 = (tree.t + tree.height) / scaleY;
					const uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
					const float corners[4][5] = {
						{ x - dx * offset, 0.0f, z - dz * offset, s0, t0 },
						{ x + dx * (1.0f - offset), 0.0f, z + dz * (1.0f - offset), s1, t0 },
						{ x + dx * (1.0f - offset), height, z + dz * (1.0f - offset), s1, t1 },
						{ x - dx * offset, height, z - dz * offset, s0, t1 },
					};
					for (const auto &corner : corners)
					{
						ThumbnailVertex vertex;
						vertex.position[0] = corner[0];
						vertex.position[1] = corner[1];
						vertex.position[2] = corner[2];
						vertex.uv[0] = corner[3];
						vertex.uv[1] = corner[4];
						mesh.vertices.push_back(vertex);	// Normal up, as WorldEditor lights its tree quads
					}
					mesh.indices.insert(mesh.indices.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
				}
			}
		}
		if (mesh.indices.size() >= 3)
			scene.meshes.push_back(std::move(mesh));
	}

	bool ThumbnailBuilder::BuildScene(const fs::path &file, ThumbnailScene &scene, ThumbnailFraming &framing, std::string *error)
	{
		scene = {};
		m_SceneTextures.clear();
		framing = ThumbnailFraming::Object;
		const auto fail = [&](const std::string &message)
		{
			if (error)
				*error = message;
			return false;
		};

		std::string resource = VFS::ToUtf8(file);
		std::replace(resource.begin(), resource.end(), '\\', '/');
		const std::string extension = Extension(resource);
		const float origin[3] = { 0.0f, 0.0f, 0.0f };
		if (extension == ".obj")
		{
			const Obj8Data *object = GetObject(file);
			if (!object)
				return fail("Not a readable OBJ8 file");
			AddObject(*object, file, origin, 0.0f, scene);
			return scene.meshes.empty() ? fail("The object draws no triangles in its nearest LOD") : true;
		}
		if (extension == ".pol" || extension == ".lin")
			return fail("Polygons and lines are drawn flat (Render)");

		if (!m_Overlay)
		{
			AssetResolver resolver = m_Resolver;
			m_Overlay = std::make_unique<OverlayBuilder>([resolver](const std::string_view path) -> fs::path
			{
				const fs::path direct = VFS::FromUtf8(path);
				if (direct.is_absolute())
				{
					const std::optional<fs::path> found = VFS::FindCaseInsensitive(direct);
					return found && VFS::IsFile(*found) ? *found : fs::path();
				}
				return resolver ? resolver(path) : fs::path();
			}, GeoProjection(0.0, 0.0));
		}

		OverlayBuild build;
		fs::path assetFile;
		const auto warning = [&](const char *fallback) { return build.warnings.empty() ? std::string(fallback) : build.warnings.front(); };

		if (extension == ".str")
		{
			// WorldEditor shows a string's first object
			const ObjectStringAsset *asset = m_Overlay->GetString(resource, &assetFile, build);
			if (!asset || asset->objects.empty())
				return fail(warning("Not a readable object string"));
			const std::string &name = asset->objects.front().object;
			fs::path objectFile;
			if (!name.starts_with("lib/"))
			{
				const fs::path local = (file.parent_path() / ToNativeRelativePath(name)).lexically_normal();
				if (const std::optional<fs::path> found = VFS::FindCaseInsensitive(local); found && VFS::IsFile(*found))
					objectFile = *found;
			}
			if (objectFile.empty() && m_Resolver)
				objectFile = m_Resolver(name);
			const Obj8Data *object = objectFile.empty() ? nullptr : GetObject(objectFile);
			if (!object)
				return fail("The string's first object wasn't found: " + name);
			AddObject(*object, objectFile, origin, 0.0f, scene);
			return scene.meshes.empty() ? fail("The string's first object draws nothing") : true;
		}
		if (extension == ".for")
		{
			const ForestAsset *asset = m_Overlay->GetForest(resource, &assetFile, build);
			if (!asset)
				return fail(warning("Not a readable forest"));
			AddForest(*asset, file, scene);
			return scene.meshes.empty() ? fail("The forest has no trees") : true;
		}

		const GeoProjection projection(0.0, 0.0);
		const auto toPoint = [&](const double x, const double z, const int wall)
		{
			DsfPoint point;
			double latitude = 0.0, longitude = 0.0;
			projection.ToGeo(Point2{ x, z }, latitude, longitude);
			point.v[0] = longitude;
			point.v[1] = latitude;
			point.v[2] = wall;
			point.planes = wall >= 0 ? 3 : 2;
			return point;
		};

		DsfFile dsf;
		std::vector<DsfPoint> footprint;
		ThumbnailMesh lawn;
		if (extension == ".fac")
		{
			const FacadeAsset *asset = m_Overlay->GetFacade(resource, &assetFile, build);
			if (!asset)
				return fail(warning("Not a readable facade"));

			// WorldEditor's preview: a 20 m square from the south-west corner, east first (the front faces the camera); its
			// sides show walls 0, 3, 2, 1 when the facade has that many; an open facade with four walls or more gets a
			// fifth corner, leaving a gap in the last side
			const size_t wallCount = asset->IsType2() ? (asset->floors.empty() ? 0 : asset->floors.front().walls.size()) : asset->walls.size();
			const bool wallTypes = wallCount > 1;
			const int choices[5] = { 0, 3, 2, 1, 0 };
			const auto wall = [&](const int side) { return !wallTypes ? -1 : static_cast<size_t>(choices[side]) < wallCount ? choices[side] : 0; };
			constexpr double side = 20.0;
			footprint = { toPoint(-side / 2, side / 2, wall(0)), toPoint(side / 2, side / 2, wall(1)), toPoint(side / 2, -side / 2, wall(2)),
				toPoint(-side / 2, -side / 2, wall(3)) };
			if (!asset->ring && wallCount >= 4)
				footprint.push_back(toPoint(-side / 2, -side / 2 + 0.8 * side, wall(4)));

			// 10 m high (type 2: a middle floor's height, so a floor is there to pick)
			float height = 10.0f;
			if (asset->IsType2() && !asset->floors.empty())
				height = asset->floors[asset->floors.size() / 2].height;
			DsfPolygonPlacement polygon;
			polygon.parameter = Parameter(std::clamp(height, 1.0f, 500.0f));
			polygon.planes = wallTypes ? 3 : 2;
			polygon.windings.push_back(std::move(footprint));
			dsf.polygonDefinitions.push_back(resource);
			dsf.polygons.push_back(std::move(polygon));

			// The lawn it stands on, reaching 40 m past the footprint (water under piers)
			const bool pier = ToLower(resource).find("piers") != std::string::npos;
			lawn = GroundPlane(side / 2 + 2.0 * side, pier ? std::array<float, 4>{ 0.0f, 0.2f, 0.4f, 0.7f } : std::array<float, 4>{ 0.2f, 0.4f, 0.2f, 0.7f });
		}
		else if (extension == ".agp")
		{
			dsf.objectDefinitions.push_back(resource);
			DsfObjectPlacement placement;
			placement.definition = 0;
			dsf.objects.push_back(placement);
		}
		else
			return fail("No thumbnail for this kind of file");

		OverlayBuilder::Options options;
		options.forestTreeLimit = 2000;
		options.maxObjects = 2000;
		m_Overlay->Build(dsf, options, build);
		AddOverlay(build, scene);
		if (scene.meshes.empty())
			return fail(warning("The asset draws nothing"));
		if (!lawn.indices.empty())
			scene.meshes.insert(scene.meshes.begin(), std::move(lawn));
		return true;
	}

	bool ThumbnailBuilder::Render(const fs::path &file, const ThumbnailOptions &options, RgbaImage &out, std::string *error)
	{
		const auto fail = [&](const std::string &message)
		{
			if (error)
				*error = message;
			return false;
		};
		const std::string name = VFS::ToUtf8(file);
		if (IsThumbnailImageFile(name))
		{
			RgbaImage image;
			if (!m_Textures || !m_Textures(file, options.size, image) || !image.IsValid())
				return fail("The image can't be read");
			return FitThumbnailImage(image, options.size, out);
		}

		// Polygons and lines are drawn flat, as WorldEditor previews them
		const std::string extension = Extension(name);
		if (extension == ".pol" || extension == ".lin")
		{
			std::string text;
			if (!VFS::ReadFile(file, text))
				return fail("The file can't be read");
			std::string parseError;
			RgbaImage texture;
			const auto loadTexture = [&](const std::string &reference)
			{
				const fs::path textureFile = ResolveThumbnailTexture(file.parent_path(), reference);
				return !textureFile.empty() && m_Textures && m_Textures(textureFile, extension == ".lin" ? m_TextureSize * 2 : options.size, texture) && texture.IsValid();
			};
			if (extension == ".pol")
			{
				PolygonAsset asset;
				if (!ParsePolygonAsset(text, asset, &parseError))
					return fail(parseError);
				if (!loadTexture(asset.texture))
					return fail("The polygon's texture can't be read: " + asset.texture);
				// One repeat in the aspect of its SCALE
				const float aspect = asset.scaleS > 0.0f && asset.scaleT > 0.0f ? asset.scaleS / asset.scaleT : 0.0f;
				return FitThumbnailImage(texture, options.size, out, aspect);
			}
			LineAsset asset;
			if (!ParseLineAsset(text, asset, &parseError))
				return fail(parseError);
			if (!loadTexture(asset.texture))
				return fail("The line's texture can't be read: " + asset.texture);
			return RenderLineThumbnail(asset, texture, options.size, out) ? true : fail("The line paints nothing");
		}

		ThumbnailScene scene;
		ThumbnailOptions framed = options;
		if (!BuildScene(file, scene, framed.framing, error))
			return false;
		return RenderThumbnail(scene, framed, out) ? true : fail("Nothing was drawn");
	}

}

// -------------------------------------------------------

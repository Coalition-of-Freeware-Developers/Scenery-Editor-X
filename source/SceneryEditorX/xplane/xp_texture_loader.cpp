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
 * xp_texture_loader.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "xp_texture_loader.h"
#include "xp_text.h"
#include <cstring>
#include <deque>
#include <mutex>
#include <stb_image.h>
#include <unordered_map>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/vulkan/device.h>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		constexpr const char *LOG_TAG = "XPlaneTextures";

		struct CacheEntry
		{
			Ref<ImageResource> image;
			TextureInfo info;
			uint64_t bytes = 0;
		};

		std::mutex s_Mutex;
		std::unordered_map<std::string, CacheEntry> s_Cache;
		std::deque<std::string> s_Names; // ImgResourceSpec keeps a raw name pointer, so names live for the session

		const char *PersistName(std::string name)
		{
			s_Names.push_back(std::move(name));
			return s_Names.back().c_str();
		}

		std::string CacheKey(const std::filesystem::path &path, const std::string_view suffix)
		{
			std::string key = std::filesystem::absolute(path).lexically_normal().string();
			for (char &c : key)
			{
				if (c >= 'A' && c <= 'Z')
					c = static_cast<char>(c - 'A' + 'a'); // Windows paths are case-insensitive
			}
			key += suffix;
			return key;
		}

		bool IsDds(const std::filesystem::path &path)
		{
			return ToLower(path.extension().string()) == ".dds";
		}

		bool ReadBinary(const std::filesystem::path &path, std::vector<uint8_t> &bytes)
		{
			std::ifstream file(path, std::ios::binary | std::ios::ate);
			if (!file)
				return false;

			const std::streamsize size = file.tellg();
			if (size <= 0)
				return false;

			bytes.resize(static_cast<size_t>(size));
			file.seekg(0, std::ios::beg);
			return static_cast<bool>(file.read(reinterpret_cast<char *>(bytes.data()), size));
		}

		bool IsBlockCompressionAvailable()
		{
			static const bool available = []
			{
				const VkPhysicalDevice physical = Device::GetPhysicalDevice();
				if (physical == VK_NULL_HANDLE)
					return false;

				VkFormatProperties properties{};
				vkGetPhysicalDeviceFormatProperties(physical, VK_FORMAT_BC3_UNORM_BLOCK, &properties);
				return (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;
			}();
			return available;
		}

		// -------------------------------------------------------
		// DDS
		// -------------------------------------------------------

		constexpr uint32_t FourCC(const char a, const char b, const char c, const char d)
		{
			return static_cast<uint32_t>(static_cast<uint8_t>(a)) | (static_cast<uint32_t>(static_cast<uint8_t>(b)) << 8) |
				   (static_cast<uint32_t>(static_cast<uint8_t>(c)) << 16) | (static_cast<uint32_t>(static_cast<uint8_t>(d)) << 24);
		}

		uint32_t ReadU32(const uint8_t *data)
		{
			uint32_t value = 0;
			std::memcpy(&value, data, sizeof(value));
			return value;
		}

		constexpr uint32_t DDPF_ALPHAPIXELS = 0x1;
		constexpr uint32_t DDPF_FOURCC = 0x4;
		constexpr uint32_t DDPF_RGB = 0x40;
		constexpr uint32_t DDSD_MIPMAPCOUNT = 0x20000;

		struct DdsImage
		{
			uint32_t width = 0;
			uint32_t height = 0;
			VkFormat format = VK_FORMAT_UNDEFINED;	// Block-compressed format, or R8G8B8A8 after conversion
			uint32_t blockBytes = 0;				// 0 for uncompressed RGBA8
			std::vector<MipBytes> mips;
			bool flipped = true;					// Rows reordered to X-Plane's bottom-left origin (false: BC7, left as stored)
		};

		/* @brief Bit position and width of a channel mask (uncompressed DDS). */
		void MaskShift(const uint32_t mask, uint32_t &shift, uint32_t &bits)
		{
			shift = 0;
			bits = 0;
			if (mask == 0)
				return;
			while (((mask >> shift) & 1u) == 0)
				++shift;
			while (shift + bits < 32 && ((mask >> (shift + bits)) & 1u) != 0)
				++bits;
		}

		uint8_t ExpandChannel(const uint32_t pixel, const uint32_t mask, const uint8_t fallback)
		{
			uint32_t shift = 0, bits = 0;
			MaskShift(mask, shift, bits);
			if (bits == 0)
				return fallback;
			const uint32_t value = (pixel & mask) >> shift;
			const uint32_t maxValue = (1u << bits) - 1u;
			return static_cast<uint8_t>((value * 255u + maxValue / 2) / maxValue);
		}

		/* @brief Reverses the first `rows` 2-bit index rows of a BC1-style colour block (4 index bytes, one per row). */
		void FlipColorIndices(uint8_t *indices, const uint32_t rows)
		{
			for (uint32_t r = 0; r < rows / 2; ++r)
				std::swap(indices[r], indices[rows - 1 - r]);
		}

		/* @brief Reverses the first `rows` 3-bit index rows of a BC3/BC4 alpha block (6 index bytes, 12 bits per row). */
		void FlipAlphaIndices(uint8_t *indices, const uint32_t rows)
		{
			uint64_t bits = 0;
			for (int i = 0; i < 6; ++i)
				bits |= static_cast<uint64_t>(indices[i]) << (8 * i);
			uint64_t row[4];
			for (int r = 0; r < 4; ++r)
				row[r] = (bits >> (12 * r)) & 0xFFFu;
			for (uint32_t r = 0; r < rows / 2; ++r)
				std::swap(row[r], row[rows - 1 - r]);
			bits = 0;
			for (int r = 0; r < 4; ++r)
				bits |= row[r] << (12 * r);
			for (int i = 0; i < 6; ++i)
				indices[i] = static_cast<uint8_t>((bits >> (8 * i)) & 0xFFu);
		}

		/**
		 * @brief Flips one block-compressed mip vertically without decoding it: block rows swap end for end and each
		 * block's pixel rows reverse (only the rows the mip uses, for mips under 4 pixels tall). Like WorldEditor's
		 * BCx_y_flip. BC7 can't be flipped this way.
		 * @return False for formats it can't flip.
		 */
		bool FlipBlocks(std::vector<std::byte> &data, const uint32_t width, const uint32_t height, const VkFormat format, const uint32_t blockBytes)
		{
			if (format == VK_FORMAT_BC7_UNORM_BLOCK)
				return false;
			auto *bytes = reinterpret_cast<uint8_t *>(data.data());
			const uint32_t blocksX = std::max(1u, (width + 3) / 4);
			const uint32_t blocksY = std::max(1u, (height + 3) / 4);
			const size_t lineBytes = static_cast<size_t>(blocksX) * blockBytes;
			if (data.size() < lineBytes * blocksY)
				return false;

			std::vector<uint8_t> line(lineBytes);
			for (uint32_t y = 0; y < blocksY / 2; ++y)
			{
				uint8_t *a = bytes + y * lineBytes;
				uint8_t *b = bytes + (blocksY - 1 - y) * lineBytes;
				std::memcpy(line.data(), a, lineBytes);
				std::memcpy(a, b, lineBytes);
				std::memcpy(b, line.data(), lineBytes);
			}

			const uint32_t rows = std::min(4u, height);
			for (size_t i = 0; i < static_cast<size_t>(blocksX) * blocksY; ++i)
			{
				uint8_t *block = bytes + i * blockBytes;
				switch (format)
				{
					case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
						FlipColorIndices(block + 4, rows);
						break;
					case VK_FORMAT_BC2_UNORM_BLOCK:
						// 4-bit explicit alpha: two bytes per row
						for (uint32_t r = 0; r < rows / 2; ++r)
						{
							std::swap(block[r * 2], block[(rows - 1 - r) * 2]);
							std::swap(block[r * 2 + 1], block[(rows - 1 - r) * 2 + 1]);
						}
						FlipColorIndices(block + 12, rows);
						break;
					case VK_FORMAT_BC3_UNORM_BLOCK:
						FlipAlphaIndices(block + 2, rows);
						FlipColorIndices(block + 12, rows);
						break;
					case VK_FORMAT_BC4_UNORM_BLOCK:
						FlipAlphaIndices(block + 2, rows);
						break;
					case VK_FORMAT_BC5_UNORM_BLOCK:
						FlipAlphaIndices(block + 2, rows);
						FlipAlphaIndices(block + 10, rows);
						break;
					default:
						return false;
				}
			}
			return true;
		}

		/* @brief Flips an RGBA8 mip vertically. */
		void FlipRows(std::vector<std::byte> &data, const uint32_t width, const uint32_t height)
		{
			const size_t rowBytes = static_cast<size_t>(width) * 4;
			if (data.size() < rowBytes * height)
				return;
			std::vector<std::byte> row(rowBytes);
			for (uint32_t y = 0; y < height / 2; ++y)
			{
				std::byte *a = data.data() + y * rowBytes;
				std::byte *b = data.data() + (height - 1 - y) * rowBytes;
				std::memcpy(row.data(), a, rowBytes);
				std::memcpy(a, b, rowBytes);
				std::memcpy(b, row.data(), rowBytes);
			}
		}

		bool ParseDds(const std::vector<uint8_t> &file, DdsImage &out, std::string &error)
		{
			if (file.size() < 128 || ReadU32(file.data()) != FourCC('D', 'D', 'S', ' '))
			{
				error = "Not a DDS file";
				return false;
			}

			const uint8_t *header = file.data() + 4;
			const uint32_t flags = ReadU32(header + 4);
			out.height = ReadU32(header + 8);
			out.width = ReadU32(header + 12);
			const uint32_t headerMips = (flags & DDSD_MIPMAPCOUNT) ? std::max(1u, ReadU32(header + 24)) : 1u;
			const uint8_t *pf = header + 72;
			const uint32_t pfFlags = ReadU32(pf + 4);
			const uint32_t fourCC = ReadU32(pf + 8);
			const uint32_t rgbBits = ReadU32(pf + 12);
			const uint32_t masks[4] = { ReadU32(pf + 16), ReadU32(pf + 20), ReadU32(pf + 24), ReadU32(pf + 28) };

			if (out.width == 0 || out.height == 0 || out.width > 16384 || out.height > 16384)
			{
				error = "DDS has invalid dimensions";
				return false;
			}

			size_t offset = 128;
			bool convertRgb = false;
			bool bgra = false;

			if (pfFlags & DDPF_FOURCC)
			{
				switch (fourCC)
				{
					case FourCC('D', 'X', 'T', '1'): out.format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK; out.blockBytes = 8; break;
					case FourCC('D', 'X', 'T', '2'):
					case FourCC('D', 'X', 'T', '3'): out.format = VK_FORMAT_BC2_UNORM_BLOCK; out.blockBytes = 16; break;
					case FourCC('D', 'X', 'T', '4'):
					case FourCC('D', 'X', 'T', '5'): out.format = VK_FORMAT_BC3_UNORM_BLOCK; out.blockBytes = 16; break;
					case FourCC('A', 'T', 'I', '1'):
					case FourCC('B', 'C', '4', 'U'): out.format = VK_FORMAT_BC4_UNORM_BLOCK; out.blockBytes = 8; break;
					case FourCC('A', 'T', 'I', '2'):
					case FourCC('B', 'C', '5', 'U'): out.format = VK_FORMAT_BC5_UNORM_BLOCK; out.blockBytes = 16; break;
					case FourCC('D', 'X', '1', '0'):
					{
						if (file.size() < 148)
						{
							error = "Truncated DX10 DDS header";
							return false;
						}

						// Albedo is linearized in the shader, so sRGB variants use the UNORM format.
						switch (ReadU32(file.data() + 128))
						{
							case 71: case 72: out.format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK; out.blockBytes = 8; break;
							case 74: case 75: out.format = VK_FORMAT_BC2_UNORM_BLOCK; out.blockBytes = 16; break;
							case 77: case 78: out.format = VK_FORMAT_BC3_UNORM_BLOCK; out.blockBytes = 16; break;
							case 80:          out.format = VK_FORMAT_BC4_UNORM_BLOCK; out.blockBytes = 8; break;
							case 83:          out.format = VK_FORMAT_BC5_UNORM_BLOCK; out.blockBytes = 16; break;
							case 98: case 99: out.format = VK_FORMAT_BC7_UNORM_BLOCK; out.blockBytes = 16; break;
							case 28: case 29: out.format = VK_FORMAT_R8G8B8A8_UNORM; break;
							case 87: case 91: out.format = VK_FORMAT_R8G8B8A8_UNORM; bgra = true; break;
							default:
								error = "Unsupported DXGI format " + std::to_string(ReadU32(file.data() + 128));
								return false;
						}
						offset = 148;
						break;
					}
					default:
						error = "Unsupported DDS FourCC";
						return false;
				}
			}
			else if ((pfFlags & DDPF_RGB) && (rgbBits == 32 || rgbBits == 24))
			{
				out.format = VK_FORMAT_R8G8B8A8_UNORM;
				convertRgb = true;
			}
			else
			{
				error = "Unsupported DDS pixel format";
				return false;
			}

			for (uint32_t mip = 0; mip < headerMips; ++mip)
			{
				const uint32_t w = std::max(1u, out.width >> mip);
				const uint32_t h = std::max(1u, out.height >> mip);

				if (out.blockBytes > 0)
				{
					const size_t size = static_cast<size_t>(std::max(1u, (w + 3) / 4)) * std::max(1u, (h + 3) / 4) * out.blockBytes;
					if (offset + size > file.size())
						break;
					MipBytes bytes;
					bytes.bytes.resize(size);
					std::memcpy(bytes.bytes.data(), file.data() + offset, size);
					out.mips.push_back(std::move(bytes));
					offset += size;
					continue;
				}

				const uint32_t srcBytesPerPixel = convertRgb ? rgbBits / 8 : 4;
				const size_t srcSize = static_cast<size_t>(w) * h * srcBytesPerPixel;
				if (offset + srcSize > file.size())
					break;

				MipBytes bytes;
				bytes.bytes.resize(static_cast<size_t>(w) * h * 4);
				auto *dst = reinterpret_cast<uint8_t *>(bytes.bytes.data());
				const uint8_t *src = file.data() + offset;
				for (size_t i = 0; i < static_cast<size_t>(w) * h; ++i, src += srcBytesPerPixel, dst += 4)
				{
					if (convertRgb)
					{
						uint32_t pixel = 0;
						std::memcpy(&pixel, src, srcBytesPerPixel);
						dst[0] = ExpandChannel(pixel, masks[0], 0);
						dst[1] = ExpandChannel(pixel, masks[1], 0);
						dst[2] = ExpandChannel(pixel, masks[2], 0);
						dst[3] = (pfFlags & DDPF_ALPHAPIXELS) ? ExpandChannel(pixel, masks[3], 255) : 255;
					}
					else
					{
						dst[0] = src[bgra ? 2 : 0];
						dst[1] = src[1];
						dst[2] = src[bgra ? 0 : 2];
						dst[3] = src[3];
					}
				}
				out.mips.push_back(std::move(bytes));
				offset += srcSize;
			}

			if (out.mips.empty())
			{
				error = "DDS has no complete mip level";
				return false;
			}

			// DDS stores the top row first; X-Plane textures have their origin at the bottom left (t = 0 is the bottom
			// row), so X-Plane and WorldEditor flip every DDS on load. Do the same, losslessly, so it matches the PNGs.
			for (size_t mip = 0; mip < out.mips.size(); ++mip)
			{
				const uint32_t w = std::max(1u, out.width >> mip);
				const uint32_t h = std::max(1u, out.height >> mip);
				if (out.blockBytes > 0)
				{
					if (!FlipBlocks(out.mips[mip].bytes, w, h, out.format, out.blockBytes))
						out.flipped = false;
				}
				else
					FlipRows(out.mips[mip].bytes, w, h);
			}
			return true;
		}

		/* @brief True when a block-compressed top mip has texels with alpha below ~1. */
		bool BlocksHaveAlpha(const DdsImage &dds)
		{
			const std::vector<std::byte> &data = dds.mips.front().bytes;
			const auto *bytes = reinterpret_cast<const uint8_t *>(data.data());
			const size_t blocks = data.size() / std::max(1u, dds.blockBytes);

			switch (dds.format)
			{
				case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
					for (size_t i = 0; i < blocks; ++i)
					{
						const uint8_t *b = bytes + i * 8;
						const uint16_t c0 = static_cast<uint16_t>(b[0] | (b[1] << 8));
						const uint16_t c1 = static_cast<uint16_t>(b[2] | (b[3] << 8));
						if (c0 > c1)
							continue; // four-color block: opaque
						const uint32_t indices = ReadU32(b + 4);
						for (int t = 0; t < 16; ++t)
						{
							if (((indices >> (t * 2)) & 3u) == 3u)
								return true;
						}
					}
					return false;

				case VK_FORMAT_BC2_UNORM_BLOCK:
					for (size_t i = 0; i < blocks; ++i)
					{
						const uint8_t *b = bytes + i * 16;
						for (int k = 0; k < 8; ++k)
						{
							if (b[k] != 0xFF)
								return true;
						}
					}
					return false;

				case VK_FORMAT_BC3_UNORM_BLOCK:
				{
					for (size_t i = 0; i < blocks; ++i)
					{
						const uint8_t *b = bytes + i * 16;
						const uint32_t a0 = b[0], a1 = b[1];
						if (a0 > a1 && a1 >= 250)
							continue; // eight-value block interpolating between two opaque endpoints

						uint32_t table[8];
						table[0] = a0;
						table[1] = a1;
						if (a0 > a1)
						{
							for (uint32_t k = 1; k < 7; ++k)
								table[k + 1] = ((7 - k) * a0 + k * a1) / 7;
						}
						else
						{
							for (uint32_t k = 1; k < 5; ++k)
								table[k + 1] = ((5 - k) * a0 + k * a1) / 5;
							table[6] = 0;
							table[7] = 255;
						}

						uint64_t indices = 0;
						for (int k = 0; k < 6; ++k)
							indices |= static_cast<uint64_t>(b[2 + k]) << (8 * k);
						for (int t = 0; t < 16; ++t)
						{
							if (table[(indices >> (3 * t)) & 7u] < 250)
								return true;
						}
					}
					return false;
				}

				default:
					return false;
			}
		}

		// -------------------------------------------------------
		// RGBA8 helpers
		// -------------------------------------------------------

		bool PixelsHaveAlpha(const uint8_t *pixels, const size_t count)
		{
			for (size_t i = 0; i < count; ++i)
			{
				if (pixels[i * 4 + 3] < 250)
					return true;
			}
			return false;
		}

		/* @brief Box-filtered mip chain for an RGBA8 image (non-power-of-two sizes clamp at the edge). */
		std::vector<MipBytes> BuildMipChain(const uint8_t *pixels, const uint32_t width, const uint32_t height)
		{
			std::vector<MipBytes> mips;
			MipBytes top;
			top.bytes.resize(static_cast<size_t>(width) * height * 4);
			std::memcpy(top.bytes.data(), pixels, top.bytes.size());
			mips.push_back(std::move(top));

			uint32_t w = width, h = height;
			while (w > 1 || h > 1)
			{
				const uint32_t nw = std::max(1u, w / 2);
				const uint32_t nh = std::max(1u, h / 2);
				const auto *src = reinterpret_cast<const uint8_t *>(mips.back().bytes.data());

				MipBytes next;
				next.bytes.resize(static_cast<size_t>(nw) * nh * 4);
				auto *dst = reinterpret_cast<uint8_t *>(next.bytes.data());
				for (uint32_t y = 0; y < nh; ++y)
				{
					const uint32_t y0 = std::min(y * 2, h - 1), y1 = std::min(y * 2 + 1, h - 1);
					for (uint32_t x = 0; x < nw; ++x)
					{
						const uint32_t x0 = std::min(x * 2, w - 1), x1 = std::min(x * 2 + 1, w - 1);
						for (int c = 0; c < 4; ++c)
						{
							const uint32_t sum = src[(static_cast<size_t>(y0) * w + x0) * 4 + c] + src[(static_cast<size_t>(y0) * w + x1) * 4 + c] +
												 src[(static_cast<size_t>(y1) * w + x0) * 4 + c] + src[(static_cast<size_t>(y1) * w + x1) * 4 + c];
							dst[(static_cast<size_t>(y) * nw + x) * 4 + c] = static_cast<uint8_t>((sum + 2) / 4);
						}
					}
				}
				mips.push_back(std::move(next));
				w = nw;
				h = nh;
			}
			return mips;
		}

		Ref<ImageResource> CreateImage(const std::string &name, const uint32_t width, const uint32_t height, const VkFormat format,
									   std::vector<MipBytes> mips, const bool transparent)
		{
			const uint32_t mipCount = static_cast<uint32_t>(mips.size());
			std::vector<Slice> slices;
			slices.push_back(Slice{ std::move(mips) });

			uint32_t flags = ShaderViews;
			if (transparent)
				flags |= Transparent;

			Ref<ImageResource> image = CreateRef<ImageResource>(ImgResourceSpec{
				.type = ImageType::Type2D,
				.width = width,
				.height = height,
				.depth = 1,
				.mipCount = mipCount,
				.format = format,
				.flags = flags,
				.name = PersistName(name) }, std::move(slices));

			if (image && !ImageResource::IsCompressedFormat(format))
			{
				image->SetBitsPerChannel(8);
				image->SetChannelCount(4);
			}
			return image;
		}

		uint64_t EstimateBytes(const std::vector<MipBytes> &mips)
		{
			uint64_t total = 0;
			for (const MipBytes &mip : mips)
				total += mip.bytes.size();
			return total;
		}
	}

	// -------------------------------------------------------

	std::filesystem::path TextureLoader::ResolveTexturePath(const std::filesystem::path &baseFolder, const std::string_view reference)
	{
		if (Trim(reference).empty())
			return {};

		const std::filesystem::path named = (baseFolder / ToNativeRelativePath(reference)).lexically_normal();

		// Each candidate as the disk spells it (a pack made on Windows names "Hangar.PNG" for "hangar.png")
		std::filesystem::path dds = named;
		dds.replace_extension(".dds");
		if (const std::optional<std::filesystem::path> found = VFS::FindCaseInsensitive(dds))
			return *found;

		if (const std::optional<std::filesystem::path> found = VFS::FindCaseInsensitive(named))
			return *found;

		// Some packs reference .dds while shipping .png (and vice versa).
		std::filesystem::path png = named;
		png.replace_extension(".png");
		if (const std::optional<std::filesystem::path> found = VFS::FindCaseInsensitive(png))
			return *found;

		return {};
	}

	bool TextureLoader::DecodeRGBA(const std::filesystem::path &path, CpuImage &out, std::string *error)
	{
		out = CpuImage{};
		std::vector<uint8_t> file;
		if (!ReadBinary(path, file))
		{
			if (error)
				*error = "Could not read '" + path.string() + "'";
			return false;
		}

		if (IsDds(path))
		{
			DdsImage dds;
			std::string ddsError;
			if (!ParseDds(file, dds, ddsError) || dds.blockBytes != 0)
			{
				if (error)
					*error = ddsError.empty() ? "Block-compressed DDS can't be decoded on the CPU" : ddsError;
				return false;
			}

			out.width = dds.width;
			out.height = dds.height;
			out.pixels.resize(dds.mips.front().bytes.size());
			std::memcpy(out.pixels.data(), dds.mips.front().bytes.data(), out.pixels.size());
			return true; // DDS rows are already bottom-up
		}

		int width = 0, height = 0, channels = 0;
		stbi_uc *pixels = stbi_load_from_memory(file.data(), static_cast<int>(file.size()), &width, &height, &channels, 4);
		if (!pixels)
		{
			if (error)
				*error = std::string("Could not decode image: ") + (stbi_failure_reason() ? stbi_failure_reason() : "unknown error");
			return false;
		}

		out.width = static_cast<uint32_t>(width);
		out.height = static_cast<uint32_t>(height);
		out.pixels.resize(static_cast<size_t>(width) * height * 4);

		// Flip so row 0 is the bottom of the image (t = 0), matching X-Plane's DDS files.
		const size_t rowBytes = static_cast<size_t>(width) * 4;
		for (int y = 0; y < height; ++y)
			std::memcpy(out.pixels.data() + static_cast<size_t>(y) * rowBytes, pixels + static_cast<size_t>(height - 1 - y) * rowBytes, rowBytes);

		stbi_image_free(pixels);
		return true;
	}

	Ref<ImageResource> TextureLoader::Load(const std::filesystem::path &path, const TextureUsage usage, TextureInfo *info)
	{
		DecodedTexture texture = Decode(path, usage);
		return Upload(texture, info);
	}

	DecodedTexture TextureLoader::Decode(const std::filesystem::path &path, const TextureUsage usage)
	{
		DecodedTexture out;
		if (path.empty())
		{
			out.info.error = "No texture file";
			return out;
		}

		out.cacheKey = CacheKey(path, "");
		{
			std::scoped_lock lock(s_Mutex);
			if (const auto it = s_Cache.find(out.cacheKey); it != s_Cache.end())
			{
				out.info = it->second.info;
				out.cached = true;
				out.valid = it->second.image != nullptr;
				return out;
			}
		}

		TextureInfo &result = out.info;
		result.path = path;
		std::vector<MipBytes> mips;

		if (IsDds(path))
		{
			std::vector<uint8_t> file;
			DdsImage dds;
			if (!ReadBinary(path, file))
				result.error = "Could not read file";
			else if (!ParseDds(file, dds, result.error))
				; // error filled in
			else if (dds.blockBytes > 0 && !IsBlockCompressionAvailable())
				result.error = "The GPU can't sample block-compressed textures";
			else
			{
				if (!dds.flipped)
					SEDX_CORE_WARN_TAG(LOG_TAG, "'{}' is BC7, which can't be flipped to X-Plane's texture orientation: it draws upside down", path.string());

				result.fromDds = true;
				result.width = dds.width;
				result.height = dds.height;
				result.format = dds.format;
				result.hasAlpha = dds.blockBytes > 0 ? BlocksHaveAlpha(dds)
					: PixelsHaveAlpha(reinterpret_cast<const uint8_t *>(dds.mips.front().bytes.data()), static_cast<size_t>(dds.width) * dds.height);

				// Uncompressed DDS without a stored chain gets one built on the CPU.
				if (dds.blockBytes == 0 && dds.mips.size() == 1)
					mips = BuildMipChain(reinterpret_cast<const uint8_t *>(dds.mips.front().bytes.data()), dds.width, dds.height);
				else
					mips = std::move(dds.mips);
			}
		}
		else
		{
			CpuImage image;
			if (DecodeRGBA(path, image, &result.error))
			{
				result.width = image.width;
				result.height = image.height;
				result.format = VK_FORMAT_R8G8B8A8_UNORM;
				result.hasAlpha = PixelsHaveAlpha(image.pixels.data(), static_cast<size_t>(image.width) * image.height);
				mips = BuildMipChain(image.pixels.data(), image.width, image.height);
			}
		}

		if (mips.empty())
		{
			SEDX_CORE_WARN_TAG(LOG_TAG, "Failed to load texture '{}': {}", path.string(), result.error);
			return out;
		}

		result.mipCount = static_cast<uint32_t>(mips.size());
		out.mips = std::move(mips);
		out.name = "xp:" + path.filename().string();
		out.filePath = path;
		out.transparent = usage == TextureUsage::Albedo && result.hasAlpha;
		out.valid = true;
		return out;
	}

	Ref<ImageResource> TextureLoader::Upload(DecodedTexture &texture, TextureInfo *info)
	{
		if (info)
			*info = texture.info;
		if (texture.cacheKey.empty())
			return nullptr;

		{
			std::scoped_lock lock(s_Mutex);
			if (const auto it = s_Cache.find(texture.cacheKey); it != s_Cache.end())
			{
				if (info)
					*info = it->second.info;
				return it->second.image;
			}

			if (texture.mips.empty())
			{
				if (texture.cacheFailure)
					s_Cache[texture.cacheKey] = CacheEntry{};
				return nullptr;
			}
		}

		const TextureInfo &result = texture.info;
		const uint64_t bytes = EstimateBytes(texture.mips);
		Ref<ImageResource> image = CreateImage(texture.name, result.width, result.height, result.format, std::move(texture.mips), texture.transparent);
		texture.mips.clear();
		if (image && !texture.filePath.empty())
			image->SetResourceFilePath(texture.filePath.string());

		if (!texture.filePath.empty())
		{
			SEDX_CORE_TRACE_TAG(LOG_TAG, "Loaded '{}' ({}x{}, {} mips, {}{})", texture.filePath.string(), result.width, result.height, result.mipCount,
				result.fromDds ? "DDS" : "RGBA8", result.hasAlpha ? ", alpha" : "");
		}

		{
			std::scoped_lock lock(s_Mutex);
			s_Cache[texture.cacheKey] = CacheEntry{ image, result, bytes };
		}
		return image;
	}

	Ref<ImageResource> TextureLoader::LoadPackedMaterial(const std::filesystem::path &normalMapPath, const bool normalMetalness, const float specular)
	{
		DecodedTexture texture = DecodePackedMaterial(normalMapPath, normalMetalness, specular);
		return Upload(texture);
	}

	DecodedTexture TextureLoader::DecodePackedMaterial(const std::filesystem::path &normalMapPath, const bool normalMetalness, float specular)
	{
		DecodedTexture out;
		if (normalMapPath.empty())
			return out;

		specular = std::clamp(specular, 0.0f, 1.0f);
		out.cacheKey = CacheKey(normalMapPath, "|orm|" + std::to_string(normalMetalness) + "|" + std::to_string(static_cast<int>(specular * 1000.0f)));
		{
			std::scoped_lock lock(s_Mutex);
			if (const auto it = s_Cache.find(out.cacheKey); it != s_Cache.end())
			{
				out.info = it->second.info;
				out.cached = true;
				out.valid = it->second.image != nullptr;
				return out;
			}
		}

		CpuImage normal;
		std::string error;
		if (!DecodeRGBA(normalMapPath, normal, &error))
		{
			// Block-compressed normal maps keep the material's flat roughness / metalness.
			SEDX_CORE_TRACE_TAG(LOG_TAG, "No packed material for '{}': {}", normalMapPath.string(), error);
			out.cacheFailure = true;
			return out;
		}

		// R = occlusion (none), G = roughness = 1 - gloss * specular, B = metalness (NORMAL_METALNESS only)
		const size_t texels = static_cast<size_t>(normal.width) * normal.height;
		std::vector<uint8_t> packed(texels * 4);
		for (size_t i = 0; i < texels; ++i)
		{
			const float gloss = static_cast<float>(normal.pixels[i * 4 + 3]) / 255.0f;
			const float roughness = 1.0f - gloss * specular;
			packed[i * 4 + 0] = 255;
			packed[i * 4 + 1] = static_cast<uint8_t>(std::clamp(roughness, 0.0f, 1.0f) * 255.0f + 0.5f);
			packed[i * 4 + 2] = normalMetalness ? normal.pixels[i * 4 + 2] : 0;
			packed[i * 4 + 3] = 255;
		}

		out.mips = BuildMipChain(packed.data(), normal.width, normal.height);
		out.name = "xp:" + normalMapPath.stem().string() + "_orm";
		out.info.path = normalMapPath;
		out.info.width = normal.width;
		out.info.height = normal.height;
		out.info.format = VK_FORMAT_R8G8B8A8_UNORM;
		out.valid = true;
		return out;
	}

	void TextureLoader::ClearCache()
	{
		std::scoped_lock lock(s_Mutex);
		s_Cache.clear();
	}

	size_t TextureLoader::GetCacheCount()
	{
		std::scoped_lock lock(s_Mutex);
		return s_Cache.size();
	}

	uint64_t TextureLoader::GetCacheBytes()
	{
		std::scoped_lock lock(s_Mutex);
		uint64_t total = 0;
		for (const auto &[key, entry] : s_Cache)
			total += entry.bytes;
		return total;
	}

}

// -------------------------------------------------------

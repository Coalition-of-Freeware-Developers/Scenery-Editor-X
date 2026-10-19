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
 * texture_importer.cpp
 * -------------------------------------------------------
 * Created: 19/03/2026
 * -------------------------------------------------------
 */
#include "texture_importer.h"
#define FREEIMAGE_LIB
#include <FreeImage/FreeImage.h>
#include <FreeImage/Utilities.h>
#define TINYDDSLOADER_IMPLEMENTATION
#include "tinyddsloader.h"
#include "SceneryEditorX/logging/asserts.h"
#include "SceneryEditorX/renderer/vulkan/image_resource.h"

#include <math_utils.h>
#include <SceneryEditorX/filesystem/file_manager.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{

#pragma region Static Image Processing Functions

	static bool GetIsSRGB(FIBITMAP* bitmap)
		{
			if (FIICCPROFILE* icc_profile = FreeImage_GetICCProfile(bitmap))
			{
				if (!icc_profile->data || icc_profile->size < 132) // minimal size check for header + tag count
					return false;
		
				auto icc = static_cast<unsigned char*>(icc_profile->data);
		
				// check for "acsp" signature at bytes 36-39
				if (icc[36] != 'a' || icc[37] != 'c' || icc[38] != 's' || icc[39] != 'p')
					return false;
		
				unsigned char* icc_end = icc + icc_profile->size;
		
				// read tag_count (big endian)
				uint32_t tag_count = (icc[128] << 24) | (icc[129] << 16) | (icc[130] << 8) | icc[131];
		
				constexpr int header_size = 128;
				constexpr int tag_record_size = 12;
				char tag_data[256] = {};
		
				for (uint32_t i = 0; i < tag_count; ++i)
				{
					unsigned char* tag = icc + header_size + 4 + i * tag_record_size;
		
					if (tag + tag_record_size > icc_end)
						return false; // invalid ICC file, tag record out of bounds
		
					// check tag signature "desc"
					if (memcmp(tag, "desc", 4) == 0)
					{
						// read tag offset and size (big endian)
						uint32_t tag_ofs  = (tag[4] << 24) | (tag[5] << 16) | (tag[6] << 8) | tag[7];
						uint32_t tag_size = (tag[8] << 24) | (tag[9] << 16) | (tag[10] << 8) | tag[11];
		
						if (tag_ofs + tag_size > static_cast<uint32_t>(icc_profile->size) || tag_size < 12)
							return false; // invalid tag offset or size
		
						// copy the string data after the 12-byte header of the 'desc' tag, up to 255 chars safely
						uint32_t string_len = xMath::Min(255u, tag_size - 12);
						strncpy_s(tag_data, sizeof(tag_data), reinterpret_cast<char*>(icc + tag_ofs + 12), string_len);
						tag_data[string_len] = '\0'; // ensure null termination
		
						// check for sRGB profile names
						if (strcmp(tag_data, "sRGB IEC61966-2.1") == 0 ||
							strcmp(tag_data, "sRGB IEC61966-2-1") == 0 ||
							strcmp(tag_data, "sRGB IEC61966") == 0 ||
							strcmp(tag_data, "* wsRGB") == 0)
						{
							return true;
						}
		
						return false;
					}
				}
			}
		
			return false;
		}

	static uint32_t GetBitsPerChannel(FIBITMAP* bitmap)
		{
			SEDX_CORE_ASSERT(bitmap != nullptr);
		
			const FREE_IMAGE_TYPE type = FreeImage_GetImageType(bitmap);
			uint32_t size = 0;
		
			if (type == FIT_BITMAP)
			{
				size = sizeof(BYTE);
			}
			else if (type == FIT_UINT16 || type == FIT_RGB16 || type == FIT_RGBA16)
			{
				size = sizeof(WORD);
			}
			else if (type == FIT_FLOAT || type == FIT_RGBF || type == FIT_RGBAF)
			{
				size = sizeof(float);
			}

			SEDX_CORE_ASSERT(size != 0);

			return size * 8;
		}

	static uint32_t GetChannelCount(FIBITMAP* bitmap)
		{
			SEDX_CORE_ASSERT(bitmap != nullptr);

			const uint32_t bits_per_pixel   = FreeImage_GetBPP(bitmap);
			const uint32_t bits_per_channel = GetBitsPerChannel(bitmap);
			const uint32_t channel_count    = bits_per_pixel / bits_per_channel;

			SEDX_CORE_ASSERT(channel_count != 0);
		
			return channel_count;
		}

	static VkFormat GetRhiFormat(const uint32_t bits_per_channel, const uint32_t channel_count)
		{
			SEDX_CORE_ASSERT(bits_per_channel != 0);
			SEDX_CORE_ASSERT(channel_count != 0);

			VkFormat format = VkFormat::VK_FORMAT_MAX_ENUM;

			if (channel_count == 1)
			{
				if (bits_per_channel == 8)
				{
					format = VkFormat::VK_FORMAT_R8_UNORM;
				}
				else if (bits_per_channel == 16)
				{
					format = VkFormat::VK_FORMAT_R16_UNORM;
				}
			}
			else if (channel_count == 2)
			{
				if (bits_per_channel == 8)
				{
					format = VkFormat::VK_FORMAT_R8G8_UNORM;
				}
			}
			else if (channel_count == 3)
			{
				if (bits_per_channel == 32)
				{
					format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
				}
			}
			else if (channel_count == 4)
			{
				if (bits_per_channel == 8)
				{
					format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
				}
				else if (bits_per_channel == 16)
				{
					format = VkFormat::VK_FORMAT_R16G16B16A16_UNORM;
				}
				else if (bits_per_channel == 32)
				{
					format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
				}
			}

			SEDX_CORE_ASSERT(format != VkFormat::VK_FORMAT_MAX_ENUM);

			return format;
		}

	// converts a bitmap to 8 bits. If the bitmap was a high-color bitmap (16, 24 or 32-bit) or
		// if it was a monochrome or greyscale bitmap (1 or 4-bit), the end result will be a greyscale
		// bitmap, otherwise (1 or 4-bit palletized bitmaps) it will be a palletized bitmap
	static FIBITMAP* ConvertTo8Bits(FIBITMAP* bitmap)
		{
			SEDX_CORE_ASSERT(bitmap != nullptr);

			FIBITMAP* previous_bitmap = bitmap;
			bitmap = FreeImage_ConvertTo8Bits(previous_bitmap);
			FreeImage_Unload(previous_bitmap);

			SEDX_CORE_ASSERT(bitmap != nullptr);

			return bitmap;
		}

	static FIBITMAP* ConvertTo32Bits(FIBITMAP* bitmap)
		{
			SEDX_CORE_ASSERT(bitmap != nullptr);

			FIBITMAP* previous_bitmap = bitmap;
			bitmap = FreeImage_ConvertTo32Bits(previous_bitmap);
			FreeImage_Unload(previous_bitmap);

			SEDX_CORE_ASSERT(bitmap != nullptr);

			return bitmap;
		}

	static FIBITMAP* Rescale(FIBITMAP* bitmap, const uint32_t width, const uint32_t height)
		{
			SEDX_CORE_ASSERT(bitmap != nullptr);
			SEDX_CORE_ASSERT(width != 0);
			SEDX_CORE_ASSERT(height != 0);

			FIBITMAP* previous_bitmap = bitmap;
			bitmap = FreeImage_Rescale(previous_bitmap, width, height, FREE_IMAGE_FILTER::FILTER_LANCZOS3);

			if (!bitmap)
			{
				SEDX_CORE_ERROR("Failed");
				return previous_bitmap;
			}

			FreeImage_Unload(previous_bitmap);
			return bitmap;
		}

	static FIBITMAP* ApplyBitmapCorrections(FIBITMAP* bitmap)
		{
			SEDX_CORE_ASSERT(bitmap != nullptr);
		
			// convert to a standard bitmap. FIT_UINT16 and FIT_RGBA16 are processed without errors
			// but show up empty in the editor. For now, we convert everything to a standard bitmap
			const FREE_IMAGE_TYPE type = FreeImage_GetImageType(bitmap);
			if (type != FIT_BITMAP)
			{
				// FreeImage can't convert FIT_RGBF
				if (type != FIT_RGBF)
				{
					const auto previous_bitmap = bitmap;
					bitmap = FreeImage_ConvertToType(bitmap, FIT_BITMAP);
					FreeImage_Unload(previous_bitmap);
				}
			}

			// textures with few colors (typically less than 8 bits) and/or a palette color type, get converted to an R8G8B8A8
			// this is because get_channel_count() returns a single channel, and from there many issues start to occur
			if (FreeImage_GetColorsUsed(bitmap) <= 256 && FreeImage_GetColorType(bitmap) != FIC_RGB)
			{
				bitmap = ConvertTo32Bits(bitmap);
			}

			// textures with 3 channels and 8 bit per channel get converted to an R8G8B8A8 format
			// this is because there is no such RHI_FORMAT format
			if (GetChannelCount(bitmap) == 3 && GetBitsPerChannel(bitmap) == 8)
			{
				bitmap = ConvertTo32Bits(bitmap);
			}
		
			// most GPUs can't use a 32 bit RGB texture as a color attachment
			// vulkan tells you, your GPU doesn't support it
			// So to prevent that, we maintain the 32 bits and convert to an RGBA format
			if (GetChannelCount(bitmap) == 3 && GetBitsPerChannel(bitmap) == 32)
			{
				FIBITMAP* previous_bitmap = bitmap;
				bitmap = FreeImage_ConvertToRGBAF(bitmap);
				FreeImage_Unload(previous_bitmap);
			}
		
			// Convert BGR to RGB (if needed)
			if (FreeImage_GetBPP(bitmap) == 32)
			{
				if (FreeImage_GetRedMask(bitmap) == 0xff0000 && GetChannelCount(bitmap) >= 2)
				{
					if (!SwapRedBlue32(bitmap))
					{
						SEDX_CORE_ERROR("Failed to swap red with blue channel");
					}
				}
			}

			// FreeImage loads images upside down, so flip it
			FreeImage_FlipVertical(bitmap);
		
			return bitmap;
		}

	static void FreeImageErrorHandler(const FREE_IMAGE_FORMAT fif, const char* message)
		{
			const auto text   = (message != nullptr) ? message : "Unknown error";
			const auto format = (fif != FIF_UNKNOWN) ? FreeImage_GetFormatFromFIF(fif) : "Unknown";

			SEDX_CORE_ERROR("%s, Format: %s", text, format);
		};

	static bool HasTransparentPixels(FIBITMAP* bitmap)
		{
			// Check if the bitmap supports alpha channel
			FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(bitmap);
			int bpp = FreeImage_GetBPP(bitmap);
		
			if (image_type == FIT_BITMAP && (bpp == 32 || bpp == 24)) 
			{
				if (bpp == 32) // Direct check for alpha channel
				{
					for (unsigned y = 0; y < FreeImage_GetHeight(bitmap); ++y)
					{
						BYTE* bits = FreeImage_GetScanLine(bitmap, y);
						for (unsigned x = 0; x < FreeImage_GetWidth(bitmap); ++x)
						{
							BYTE alpha = bits[FI_RGBA_ALPHA];
							if (alpha != 255)
								return true;
		
							bits += 4; // move to the next pixel (4 bytes per pixel for 32-bit)
						}
					}
				}
				else if (bpp == 24) // For 24-bit, we need to check if there's a transparency mask
				{
					// If there's a transparency mask, we'll check it
					if (FreeImage_GetTransparencyCount(bitmap) > 0)
					{
						for (unsigned i = 0; i < FreeImage_GetTransparencyCount(bitmap); ++i)
						{
							if (FreeImage_GetTransparencyTable(bitmap)[i] != 255)
								return true;
						}
					}
					// If no transparency mask, 24-bit images are assumed to be fully opaque
				}
			}
			else if (image_type == FIT_RGBA16) // 16-bit per channel, including alpha
			{
				// For 64-bit or other formats with explicit alpha
				for (unsigned y = 0; y < FreeImage_GetHeight(bitmap); ++y)
				{
					WORD* bits = (WORD*)FreeImage_GetScanLine(bitmap, y);
					for (unsigned x = 0; x < FreeImage_GetWidth(bitmap); ++x)
					{
						WORD alpha = bits[3]; // Assuming RGBA order, alpha at index 3
						if (alpha != 0xFFFF) // 16-bit alpha, 0xFFFF is fully opaque
							return true;
		
						bits += 4; // move to next pixel (4 words for RGBA16)
					}
				}
			}
			// Other formats might not have transparency or are treated as fully opaque
		
			return false;
		}

	static float HalfToFloat(uint16_t half)
		{
			uint32_t mant = half & 0x3FFu;
			uint32_t exp  = (half >> 10) & 0x1Fu;
			uint32_t sign = (half >> 15) & 0x1u;

			uint32_t f;
			if (exp == 0)
			{
				f = (sign << 31) | (mant ? ((127 - 15) << 23) | (mant << 13) : 0); // denormals as zero for simplicity
			}
			else if (exp == 0x1F)
			{
				f = (sign << 31) | (0x7F800000) | (mant << 13); // inf/nan
			}
			else
			{
				f = (sign << 31) | ((exp + (127 - 15)) << 23) | (mant << 13);
			}

			return *reinterpret_cast<float*>(&f); // assumes ieee 754 and little-endian
		}

	// attempt to reverse the srgb gamma if data is already gamma-encoded
	static float SRGBToLinear(float srgb)
		{
			if (srgb <= 0.04045f)
				return srgb / 12.92f;
			return powf((srgb + 0.055f) / 1.055f, 2.4f);
		}

	// attempt to reverse any existing tonemapping (approximate inverse reinhard)
	static float InverseTonemap(float x)
		{
			// inverse of x/(x+1) is x/(1-x), clamped to avoid division issues
			x = xMath::Min(x, 0.999f);
			return x / (1.0f - x);
		}

	// aces tonemapping (same as shader implementation)
	static void AcesTonemap(float& r, float& g, float& b)
		{
			// srgb => xyz => d65_2_d60 => ap1 => rrt_sat
			float ir = r * 0.59719f + g * 0.35458f + b * 0.04823f;
			float ig = r * 0.07600f + g * 0.90834f + b * 0.01566f;
			float ib = r * 0.02840f + g * 0.13383f + b * 0.83777f;

			// rrt and odt fit
			float ar = ir * (ir + 0.0245786f) - 0.000090537f;
			float ag = ig * (ig + 0.0245786f) - 0.000090537f;
			float ab = ib * (ib + 0.0245786f) - 0.000090537f;
			float br = ir * (0.983729f * ir + 0.4329510f) + 0.238081f;
			float bg = ig * (0.983729f * ig + 0.4329510f) + 0.238081f;
			float bb = ib * (0.983729f * ib + 0.4329510f) + 0.238081f;
			ir = ar / br;
			ig = ag / bg;
			ib = ab / bb;

			// odt_sat => xyz => d60_2_d65 => srgb
			r =  ir *  1.60475f + ig * -0.53108f + ib * -0.07367f;
			g =  ir * -0.10208f + ig *  1.10813f + ib * -0.00605f;
			b =  ir * -0.00327f + ig * -0.07276f + ib *  1.07602f;

			// clamp
			r = xMath::Max(0.0f, xMath::Min(1.0f, r));
			g = xMath::Max(0.0f, xMath::Min(1.0f, g));
			b = xMath::Max(0.0f, xMath::Min(1.0f, b));
		}

	// linear to srgb gamma correction
	static float LinearToSRGB(float linear)
		{
			if (linear <= 0.0031308f)
				return linear * 12.92f;
			return 1.055f * powf(linear, 1.0f / 2.4f) - 0.055f;
		}

	// decode pq/st.2084 to linear (inverse of linear_to_hdr10 in output.hlsl)
	static void PqToLinear(float& r, float& g, float& b)
		{
			// inverse rec.2020 to rec.709 matrix
			static const float m[3][3] = {
				{  1.6605f, -0.5876f, -0.0728f },
				{ -0.1246f,  1.1329f, -0.0083f },
				{ -0.0182f, -0.1006f,  1.1187f }
			};

			// inverse pq (st.2084) constants
			const float m1 = 0.1593017578125f;
			const float m2 = 78.84375f;
			const float c1 = 0.8359375f;
			const float c2 = 18.8515625f;
			const float c3 = 18.6875f;

			// decode pq curve - output is normalized for aces input
			auto decode_pq = [&](float v) -> float {
				v = xMath::Max(v, 0.0f);
				float vp = powf(v, 1.0f / m2);
				float num = xMath::Max(vp - c1, 0.0f);
				float den = c2 - c3 * vp;
				if (den <= 0.0001f) return 0.0f;
				float linear = powf(num / den, 1.0f / m1);
				// scale for good aces input range
				return linear * 50.0f;
			};

			float lr = decode_pq(r);
			float lg = decode_pq(g);
			float lb = decode_pq(b);

			// convert rec.2020 back to rec.709/srgb
			r = m[0][0] * lr + m[0][1] * lg + m[0][2] * lb;
			g = m[1][0] * lr + m[1][1] * lg + m[1][2] * lb;
			b = m[2][0] * lr + m[2][1] * lg + m[2][2] * lb;

			r = xMath::Max(r, 0.0f);
			g = xMath::Max(g, 0.0f);
			b = xMath::Max(b, 0.0f);
		}

#pragma endregion

	void TextureImporter::Init()
	{
		FreeImage_Initialise();
		FreeImage_SetOutputMessage(FreeImageErrorHandler);
	}

	void TextureImporter::Shutdown()
	{
		FreeImage_DeInitialise();
	}

	void TextureImporter::Load(const std::string & filePath, const uint32_t sliceIndex, ImageResource* texture)
	{
		SEDX_CORE_ASSERT(texture != nullptr);

		if (!IO::FileSystem::Exists(filePath))
		{
			SEDX_CORE_ERROR("Path \"%s\" is invalid.", filePath.c_str());
			return;
		}

		// acquire image format
		FREE_IMAGE_FORMAT format = FIF_UNKNOWN;
		{
			format = FreeImage_GetFileType(filePath.c_str(), 0);

			// if the format is unknown, try to work it out from the file path
			if (format == FIF_UNKNOWN)
			{
				format = FreeImage_GetFIFFromFilename(filePath.c_str());
			}

			// if the format is still unknown, give up
			if (!FreeImage_FIFSupportsReading(format)) 
			{
				SEDX_CORE_ERROR("Unsupported format");
				return;
			}
		}

		uint32_t texture_flags = texture->GetFlags();

		// freeimage partially supports dds, they are certain configurations that it can't load
		// So in the case of a dds format in general, we don't rely on freeimage
		if (format == FIF_DDS)
		{
			// load
			tinyddsloader::DDSFile dds_file;
			auto result = dds_file.Load(filePath.c_str());
			if (result != tinyddsloader::Success)
			{
				SEDX_CORE_ERROR("Failed to load DSS file");
				return;
			}

			// get format
			auto format_dxgi = dds_file.GetFormat();
			VkFormat format = VkFormat::VK_FORMAT_MAX_ENUM;
			if (format_dxgi == tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm)
			{
				format = VkFormat::VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			}
			else if (format_dxgi == tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm)
			{
				format = VkFormat::VK_FORMAT_BC3_UNORM_BLOCK;
			}
			else if (format_dxgi == tinyddsloader::DDSFile::DXGIFormat::BC5_UNorm)
			{
				format = VkFormat::VK_FORMAT_BC5_UNORM_BLOCK;
			}
			else if (format_dxgi == tinyddsloader::DDSFile::DXGIFormat::BC7_UNorm)
			{
				format = VkFormat::VK_FORMAT_BC7_UNORM_BLOCK;
			}
			SEDX_CORE_ASSERT(format != VkFormat::VK_FORMAT_MAX_ENUM);

			// set properties
			texture->SetWidth(dds_file.GetWidth());
			texture->SetHeight(dds_file.GetHeight());
			texture->SetFormat(format);

			// set data
			for (uint32_t mip_index = 0; mip_index < dds_file.GetMipCount(); mip_index++)
			{
				texture->AllocateMip();
				MipBytes* mip = texture->GetMip(0, mip_index);

				const auto& data = dds_file.GetImageData(mip_index, 0);
				memcpy(mip->bytes.data(), data->m_mem, mip->bytes.size());
			}

			return;
		}

		// load
		FIBITMAP* bitmap = FreeImage_Load(format, filePath.c_str());
		if (!bitmap)
		{
			SEDX_CORE_ERROR("Failed to load \"%s\"", filePath.c_str());
			return;
		}
		
		// deduce certain properties
		// done before ApplyBitmapCorrections(), as after that, results for grayscale seem to be always false
		texture_flags |= (FreeImage_GetColorType(bitmap) == FREE_IMAGE_COLOR_TYPE::FIC_MINISBLACK) ? Greyscale : 0;
		texture_flags |= GetIsSRGB(bitmap) ? SRGB : 0;
		texture->SetFlags(texture_flags);

		// perform some corrections
		bitmap = ApplyBitmapCorrections(bitmap);
		if (!bitmap)
		{
			SEDX_CORE_ERROR("Failed to apply bitmap corrections");
			return;
		}

		// scale if needed
		const bool user_define_dimensions = (texture->GetWidth() != 0 && texture->GetHeight() != 0);
		const bool dimension_mismatch     = (FreeImage_GetWidth(bitmap) != texture->GetWidth() && FreeImage_GetHeight(bitmap) != texture->GetHeight());
		const bool scale                  = user_define_dimensions && dimension_mismatch;
		bitmap                            = scale ? Rescale(bitmap, texture->GetWidth(), texture->GetHeight()) : bitmap;

		// set properties
		texture->SetBitsPerChannel(GetBitsPerChannel(bitmap));
		texture->SetWidth(FreeImage_GetWidth(bitmap));
		texture->SetHeight(FreeImage_GetHeight(bitmap));
		texture->SetChannelCount(GetChannelCount(bitmap));
		texture->SetFormat(GetRhiFormat(texture->GetBitsPerChannel(), texture->GetChannelCount()));
		texture->SetFlag(Transparent, HasTransparentPixels(bitmap));

		// copy data over
		texture->AllocateMip();
		MipBytes* mip = texture->GetMip(0, 0);
		BYTE* bytes          = FreeImage_GetBits(bitmap);
		size_t bytes_size    = FreeImage_GetPitch(bitmap) * FreeImage_GetHeight(bitmap);
		mip->bytes.resize(bytes_size);
		memcpy(&mip->bytes[0], bytes, bytes_size);

		FreeImage_Unload(bitmap);
	}

	void TextureImporter::Save(const std::string & filePath, const uint32_t width, const uint32_t height, const uint32_t channelCount, const uint32_t bitsPerChannel, void* data)
	{
		// assume input is half float RGBA16F (from Vulkan)
		const uint16_t* src_half = static_cast<const uint16_t*>(data);
		uint32_t pixel_count     = width * height;

		// convert half -> float
		std::vector<float> converted(pixel_count * 4);
		for (uint32_t i = 0; i < pixel_count; i++)
		{
			converted[i * 4 + 0] = HalfToFloat(src_half[i * 4 + 0]);
			converted[i * 4 + 1] = HalfToFloat(src_half[i * 4 + 1]);
			converted[i * 4 + 2] = HalfToFloat(src_half[i * 4 + 2]);
			converted[i * 4 + 3] = HalfToFloat(src_half[i * 4 + 3]);
		}

		// allocate HDR bitmap (RGBAF = 128 bits per pixel float)
		FIBITMAP* bitmap = FreeImage_AllocateT(FIT_RGBAF, width, height, 128);
		if (!bitmap)
		{
			SEDX_CORE_ERROR("Failed to allocate FreeImage HDR bitmap");
			return;
		}

		// copy data row by row (FreeImage stores bottom-up by default)
		for (uint32_t y = 0; y < height; y++)
		{
			float* scanline = reinterpret_cast<float*>(FreeImage_GetScanLine(bitmap, height - 1 - y));
			memcpy(scanline, &converted[y * width * 4], width * 4 * sizeof(float));
		}

		// save as OpenEXR
		BOOL saved = FreeImage_Save(FIF_EXR, bitmap, filePath.c_str(), EXR_DEFAULT);
		FreeImage_Unload(bitmap);

		if (!saved)
		{
			SEDX_CORE_ERROR("Failed to save HDR EXR to %s", filePath.c_str());
		}
	}

	void TextureImporter::SaveSdr(const std::string & filePath, const uint32_t width, const uint32_t height, const uint32_t channelCount, const uint32_t bitsPerChannel, void* data, bool isHdr)
	{
		const uint16_t* src_half = static_cast<const uint16_t*>(data);

		// allocate 24-bit RGB bitmap (8 bits per channel)
		FIBITMAP* bitmap = FreeImage_Allocate(width, height, 24);
		if (!bitmap)
		{
			SEDX_CORE_ERROR("Failed to allocate FreeImage SDR bitmap");
			return;
		}

		for (uint32_t y = 0; y < height; y++)
		{
			BYTE* scanline = FreeImage_GetScanLine(bitmap, height - 1 - y);
			for (uint32_t x = 0; x < width; x++)
			{
				uint32_t src_idx = (y * width + x) * 4;

				// convert half to float
				float r = HalfToFloat(src_half[src_idx + 0]);
				float g = HalfToFloat(src_half[src_idx + 1]);
				float b = HalfToFloat(src_half[src_idx + 2]);

				if (isHdr)
				{
					// hdr mode: frame_output contains pq-encoded data
					// decode pq to linear (values can exceed 1.0 for hdr highlights)
					// apply aces to compress hdr range to sdr with proper toe/shoulder
					PqToLinear(r, g, b);
					AcesTonemap(r, g, b);
					r = LinearToSRGB(r);
					g = LinearToSRGB(g);
					b = LinearToSRGB(b);
				}
				// sdr mode: frame_output is already srgb, save directly

				// convert to 8-bit (FreeImage uses BGR order)
				uint32_t dst_idx = x * 3;
				scanline[dst_idx + 0] = static_cast<BYTE>(xMath::Min(xMath::Max(b, 0.0f), 1.0f) * 255.0f + 0.5f);
				scanline[dst_idx + 1] = static_cast<BYTE>(xMath::Min(xMath::Max(g, 0.0f), 1.0f) * 255.0f + 0.5f);
				scanline[dst_idx + 2] = static_cast<BYTE>(xMath::Min(xMath::Max(r, 0.0f), 1.0f) * 255.0f + 0.5f);
			}
		}

		// save as PNG (lossless, universal format)
		BOOL saved = FreeImage_Save(FIF_PNG, bitmap, filePath.c_str(), PNG_Z_BEST_COMPRESSION);
		FreeImage_Unload(bitmap);

		if (!saved)
		{
			SEDX_CORE_ERROR("Failed to save SDR PNG to %s", filePath.c_str());
		}
	}

#pragma region Memory::Buffer Loading

	Memory::Buffer TextureImporter::ToBufferFromFile(const std::filesystem::path &path, VkFormat &outFormat, uint32_t &outWidth, uint32_t &outHeight)
	{
		IO::FileStatus fileStatus = IO::FileSystem::TryOpenFileAndWait(path, 100);
		Memory::Buffer imageBuffer;
		std::string pathString = path.string();
		bool isSRGB = (outFormat == VkFormat::VK_FORMAT_R8G8B8_SRGB) || (outFormat == VkFormat::VK_FORMAT_R8G8B8A8_SRGB);

		int width, height, channels;
		void* tmp;
		size_t size = 0;

		if (stbi_is_hdr(pathString.c_str()))
		{
			tmp = stbi_loadf(pathString.c_str(), &width, &height, &channels, 4);
			if (tmp)
			{
				size = width * height * 4 * sizeof(float);
				outFormat = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
			}
		}
		else
		{
			//stbi_set_flip_vertically_on_load(1);
			tmp = stbi_load(pathString.c_str(), &width, &height, &channels, 4);
			if (tmp)
			{
				size = width * height * 4;
				outFormat = isSRGB ? VkFormat::VK_FORMAT_R8G8B8A8_SRGB : VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
			}
		}

		if (!tmp)
		{
			return {};
		}

		SEDX_CORE_ASSERT(size > 0);
		imageBuffer.data = new byte[size]; // avoid `malloc+delete[]` mismatch.
		imageBuffer.size = size;
		memcpy(imageBuffer.data, tmp, size);
		stbi_image_free(tmp);

		outWidth = width;
		outHeight = height;
		return imageBuffer;
	}

	Memory::Buffer TextureImporter::ToBufferFromMemory(Memory::Buffer buffer, VkFormat &outFormat, uint32_t &outWidth, uint32_t &outHeight)
	{
		Memory::Buffer imageBuffer;

		bool isSRGB = (outFormat == VkFormat::VK_FORMAT_R8G8B8_SRGB) || (outFormat == VkFormat::VK_FORMAT_R8G8B8A8_SRGB);

		int width, height, channels;
		void* tmp;
		size_t size;

		if (stbi_is_hdr_from_memory(static_cast<const stbi_uc *>(buffer.data), static_cast<int>(buffer.size)))
		{
			tmp = reinterpret_cast<byte *>(stbi_loadf_from_memory(static_cast<const stbi_uc *>(buffer.data),
																  static_cast<int>(buffer.size),
																  &width,
																  &height,
																  &channels,
																  STBI_rgb_alpha));
			size = width * height * 4 * sizeof(float);
			outFormat = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		}
		else
		{
			// stbi_set_flip_vertically_on_load(1);
			tmp = stbi_load_from_memory(static_cast<const stbi_uc *>(buffer.data), static_cast<int>(buffer.size), &width, &height, &channels, STBI_rgb_alpha);
			size = width * height * 4;
			outFormat = isSRGB? VkFormat::VK_FORMAT_R8G8B8A8_SRGB : VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
		}

		imageBuffer.data = new byte[size]; // avoid `malloc+delete[]` mismatch.
		imageBuffer.size = size;
		memcpy(imageBuffer.data, tmp, size);
		stbi_image_free(tmp);

		if (!imageBuffer.data)
			return {};

		outWidth = width;
		outHeight = height;
		return imageBuffer;
	}

#pragma endregion

} // namespace SceneryEditorX

// -------------------------------------------------------

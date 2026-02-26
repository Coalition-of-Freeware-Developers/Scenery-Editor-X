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
 * swapchain.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "swapchain.h"
#include "render_context.h"
#include "renderer.h"
#include "SceneryEditorX/core/window/window.h"
#include <utility>
#include <vector>
#include <SDL3/SDL_vulkan.h>
#include <tracy/Tracy.hpp>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

    static const char* FormatToString(const VkFormat format)
	{
		switch (format)
		{
			case VK_FORMAT_B8G8R8A8_SRGB:									return "B8G8R8A8_SRGB";
			case VK_FORMAT_D24_UNORM_S8_UINT:								return "D24_UNORM_S8_UINT";
            case VK_FORMAT_UNDEFINED:										return "UNDEFINED";
            case VK_FORMAT_R4G4_UNORM_PACK8:								return "R4G4_UNORM_PACK8";
            case VK_FORMAT_R4G4B4A4_UNORM_PACK16:							return "R4G4B4A4_UNORM_PACK16";
            case VK_FORMAT_B4G4R4A4_UNORM_PACK16:							return "B4G4R4A4_UNORM_PACK16";
            case VK_FORMAT_R5G6B5_UNORM_PACK16:								return "R5G6B5_UNORM_PACK16";
            case VK_FORMAT_B5G6R5_UNORM_PACK16:								return "B5G6R5_UNORM_PACK16";
            case VK_FORMAT_R5G5B5A1_UNORM_PACK16:							return "R5G5B5A1_UNORM_PACK16";
            case VK_FORMAT_B5G5R5A1_UNORM_PACK16:							return "B5G5R5A1_UNORM_PACK16";
            case VK_FORMAT_A1R5G5B5_UNORM_PACK16:							return "A1R5G5B5_UNORM_PACK16";
            case VK_FORMAT_R8_UNORM:										return "R8_UNORM";
            case VK_FORMAT_R8_SNORM:										return "R8_SNORM";
            case VK_FORMAT_R8_USCALED:										return "R8_USCALED";
            case VK_FORMAT_R8_SSCALED:										return "R8_SSCALED";
            case VK_FORMAT_R8_UINT:											return "R8_UINT";
            case VK_FORMAT_R8_SINT:											return "R8_SINT";
            case VK_FORMAT_R8_SRGB:											return "R8_SRGB";
            case VK_FORMAT_R8G8_UNORM:										return "R8G8_UNORM";
            case VK_FORMAT_R8G8_SNORM:										return "R8G8_SNORM";
            case VK_FORMAT_R8G8_USCALED:									return "R8G8_USCALED";
            case VK_FORMAT_R8G8_SSCALED:									return "R8G8_SSCALED";
            case VK_FORMAT_R8G8_UINT:										return "R8G8_UINT";
            case VK_FORMAT_R8G8_SINT:										return "R8G8_SINT";
            case VK_FORMAT_R8G8_SRGB:										return "R8G8_SRGB";
            case VK_FORMAT_R8G8B8_UNORM:									return "R8G8B8_UNORM";
            case VK_FORMAT_R8G8B8_SNORM:									return "R8G8B8_SNORM";
            case VK_FORMAT_R8G8B8_USCALED:									return "R8G8B8_USCALED";
            case VK_FORMAT_R8G8B8_SSCALED:									return "R8G8B8_SSCALED";
            case VK_FORMAT_R8G8B8_UINT:										return "R8G8B8_UINT";
            case VK_FORMAT_R8G8B8_SINT:										return "R8G8B8_SINT";
            case VK_FORMAT_R8G8B8_SRGB:										return "R8G8B8_SRGB";
            case VK_FORMAT_B8G8R8_UNORM:									return "B8G8R8_UNORM";
            case VK_FORMAT_B8G8R8_SNORM:									return "B8G8R8_SNORM";
            case VK_FORMAT_B8G8R8_USCALED:									return "B8G8R8_USCALED";
            case VK_FORMAT_B8G8R8_SSCALED:									return "B8G8R8_SSCALED";
            case VK_FORMAT_B8G8R8_UINT:										return "B8G8R8_UINT";
            case VK_FORMAT_B8G8R8_SINT:										return "B8G8R8_SINT";
            case VK_FORMAT_B8G8R8_SRGB:										return "B8G8R8_SRGB";
            case VK_FORMAT_R8G8B8A8_UNORM:									return "R8G8B8A8_UNORM";
            case VK_FORMAT_R8G8B8A8_SNORM:									return "R8G8B8A8_SNORM";
            case VK_FORMAT_R8G8B8A8_USCALED:								return "R8G8B8A8_USCALED";
            case VK_FORMAT_R8G8B8A8_SSCALED:								return "R8G8B8A8_SSCALED";
            case VK_FORMAT_R8G8B8A8_UINT:									return "R8G8B8A8_UINT";
            case VK_FORMAT_R8G8B8A8_SINT:									return "R8G8B8A8_SINT";
            case VK_FORMAT_R8G8B8A8_SRGB:									return "R8G8B8A8_SRGB";
            case VK_FORMAT_B8G8R8A8_UNORM:									return "B8G8R8A8_UNORM";
            case VK_FORMAT_B8G8R8A8_SNORM:									return "B8G8R8A8_SNORM";
            case VK_FORMAT_B8G8R8A8_USCALED:								return "B8G8R8A8_USCALED";
            case VK_FORMAT_B8G8R8A8_SSCALED:								return "B8G8R8A8_SSCALED";
            case VK_FORMAT_B8G8R8A8_UINT:									return "B8G8R8A8_UINT";
            case VK_FORMAT_B8G8R8A8_SINT:									return "B8G8R8A8_SINT";
            case VK_FORMAT_A8B8G8R8_UNORM_PACK32:							return "A8B8G8R8_UNORM_PACK32";
            case VK_FORMAT_A8B8G8R8_SNORM_PACK32:							return "A8B8G8R8_SNORM_PACK32";
            case VK_FORMAT_A8B8G8R8_USCALED_PACK32:							return "A8B8G8R8_USCALED_PACK32";
            case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:							return "A8B8G8R8_SSCALED_PACK32";
            case VK_FORMAT_A8B8G8R8_UINT_PACK32:							return "A8B8G8R8_UINT_PACK32";
            case VK_FORMAT_A8B8G8R8_SINT_PACK32:							return "A8B8G8R8_SINT_PACK32";
            case VK_FORMAT_A8B8G8R8_SRGB_PACK32:							return "A8B8G8R8_SRGB_PACK32";
            case VK_FORMAT_A2R10G10B10_UNORM_PACK32:						return "A2R10G10B10_UNORM_PACK32";
            case VK_FORMAT_A2R10G10B10_SNORM_PACK32:						return "A2R10G10B10_SNORM_PACK32";
            case VK_FORMAT_A2R10G10B10_USCALED_PACK32:						return "A2R10G10B10_USCALED_PACK32";
            case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:						return "A2R10G10B10_SSCALED_PACK32";
            case VK_FORMAT_A2R10G10B10_UINT_PACK32:							return "A2R10G10B10_UINT_PACK32";
            case VK_FORMAT_A2R10G10B10_SINT_PACK32:							return "A2R10G10B10_SINT_PACK32";
            case VK_FORMAT_A2B10G10R10_UNORM_PACK32:						return "A2B10G10R10_UNORM_PACK32";
            case VK_FORMAT_A2B10G10R10_SNORM_PACK32:						return "A2B10G10R10_SNORM_PACK32";
            case VK_FORMAT_A2B10G10R10_USCALED_PACK32:						return "A2B10G10R10_USCALED_PACK32";
            case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:						return "A2B10G10R10_SSCALED_PACK32";
            case VK_FORMAT_A2B10G10R10_UINT_PACK32:							return "A2B10G10R10_UINT_PACK32";
            case VK_FORMAT_A2B10G10R10_SINT_PACK32:							return "A2B10G10R10_SINT_PACK32";
            case VK_FORMAT_R16_UNORM:										return "R16_UNORM";
            case VK_FORMAT_R16_SNORM:										return "R16_SNORM";
            case VK_FORMAT_R16_USCALED:										return "R16_USCALED";
            case VK_FORMAT_R16_SSCALED:										return "R16_SSCALED";
            case VK_FORMAT_R16_UINT:										return "R16_UINT";
            case VK_FORMAT_R16_SINT:										return "R16_SINT";
            case VK_FORMAT_R16_SFLOAT:										return "R16_SFLOAT";
            case VK_FORMAT_R16G16_UNORM:									return "R16G16_UNORM";
            case VK_FORMAT_R16G16_SNORM:									return "R16G16_SNORM";
            case VK_FORMAT_R16G16_USCALED:									return "R16G16_USCALED";
            case VK_FORMAT_R16G16_SSCALED:									return "R16G16_SSCALED";
            case VK_FORMAT_R16G16_UINT:										return "R16G16_UINT";
            case VK_FORMAT_R16G16_SINT:										return "R16G16_SINT";
            case VK_FORMAT_R16G16_SFLOAT:									return "R16G16_SFLOAT";
            case VK_FORMAT_R16G16B16_UNORM:									return "R16G16B16_UNORM";
            case VK_FORMAT_R16G16B16_SNORM:									return "R16G16B16_SNORM";
            case VK_FORMAT_R16G16B16_USCALED:								return "R16G16B16_USCALED";
            case VK_FORMAT_R16G16B16_SSCALED:								return "R16G16B16_SSCALED";
            case VK_FORMAT_R16G16B16_UINT:									return "R16G16B16_UINT";
            case VK_FORMAT_R16G16B16_SINT:									return "R16G16B16_SINT";
            case VK_FORMAT_R16G16B16_SFLOAT:								return "R16G16B16_SFLOAT";
            case VK_FORMAT_R16G16B16A16_UNORM:								return "R16G16B16A16_UNORM";
            case VK_FORMAT_R16G16B16A16_SNORM:								return "R16G16B16A16_SNORM";
            case VK_FORMAT_R16G16B16A16_USCALED:							return "R16G16B16A16_USCALED";
            case VK_FORMAT_R16G16B16A16_SSCALED:							return "R16G16B16A16_SSCALED";
            case VK_FORMAT_R16G16B16A16_UINT:								return "R16G16B16A16_UINT";
            case VK_FORMAT_R16G16B16A16_SINT:								return "R16G16B16A16_SINT";
            case VK_FORMAT_R16G16B16A16_SFLOAT:								return "R16G16B16A16_SFLOAT";
            case VK_FORMAT_R32_UINT:										return "R32_UINT";
            case VK_FORMAT_R32_SINT:										return "R32_SINT";
            case VK_FORMAT_R32_SFLOAT:										return "R32_SFLOAT";
            case VK_FORMAT_R32G32_UINT:										return "R32G32_UINT";
            case VK_FORMAT_R32G32_SINT:										return "R32G32_SINT";
            case VK_FORMAT_R32G32_SFLOAT:									return "R32G32_SFLOAT";
            case VK_FORMAT_R32G32B32_UINT:									return "R32G32B32_UINT";
            case VK_FORMAT_R32G32B32_SINT:									return "R32G32B32_SINT";
            case VK_FORMAT_R32G32B32_SFLOAT:								return "R32G32B32_SFLOAT";
            case VK_FORMAT_R32G32B32A32_UINT:								return "R32G32B32A32_UINT";
            case VK_FORMAT_R32G32B32A32_SINT:								return "R32G32B32A32_SINT";
            case VK_FORMAT_R32G32B32A32_SFLOAT:								return "R32G32B32A32_SFLOAT";
            case VK_FORMAT_R64_UINT:										return "R64_UINT";
            case VK_FORMAT_R64_SINT:										return "R64_SINT";
            case VK_FORMAT_R64_SFLOAT:										return "R64_SFLOAT";
            case VK_FORMAT_R64G64_UINT:										return "R64G64_UINT";
            case VK_FORMAT_R64G64_SINT:										return "R64G64_SINT";
            case VK_FORMAT_R64G64_SFLOAT:									return "R64G64_SFLOAT";
            case VK_FORMAT_R64G64B64_UINT:									return "R64G64B64_UINT";
            case VK_FORMAT_R64G64B64_SINT:									return "R64G64B64_SINT";
            case VK_FORMAT_R64G64B64_SFLOAT:								return "R64G64B64_SFLOAT";
            case VK_FORMAT_R64G64B64A64_UINT:								return "R64G64B64A64_UINT";
            case VK_FORMAT_R64G64B64A64_SINT:								return "R64G64B64A64_SINT";
            case VK_FORMAT_R64G64B64A64_SFLOAT:								return "R64G64B64A64_SFLOAT";
            case VK_FORMAT_B10G11R11_UFLOAT_PACK32:							return "B10G11R11_UFLOAT_PACK32";
            case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:							return "E5B9G9R9_UFLOAT_PACK32";
            case VK_FORMAT_D16_UNORM:										return "D16_UNORM";
            case VK_FORMAT_X8_D24_UNORM_PACK32:								return "X8_D24_UNORM_PACK32";
            case VK_FORMAT_D32_SFLOAT:										return "D32_SFLOAT";
            case VK_FORMAT_S8_UINT:											return "S8_UINT";
            case VK_FORMAT_D16_UNORM_S8_UINT:								return "D16_UNORM_S8_UINT";
            case VK_FORMAT_D32_SFLOAT_S8_UINT:								return "D32_SFLOAT_S8_UINT";
            case VK_FORMAT_BC1_RGB_UNORM_BLOCK:								return "BC1_RGB_UNORM_BLOCK";
            case VK_FORMAT_BC1_RGB_SRGB_BLOCK:								return "BC1_RGB_SRGB_BLOCK";
            case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:							return "BC1_RGBA_UNORM_BLOCK";
            case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:								return "BC1_RGBA_SRGB_BLOCK";
            case VK_FORMAT_BC2_UNORM_BLOCK:									return "BC2_UNORM_BLOCK";
            case VK_FORMAT_BC2_SRGB_BLOCK:									return "BC2_SRGB_BLOCK";
            case VK_FORMAT_BC3_UNORM_BLOCK:									return "BC3_UNORM_BLOCK";
            case VK_FORMAT_BC3_SRGB_BLOCK:									return "BC3_SRGB_BLOCK";
            case VK_FORMAT_BC4_UNORM_BLOCK:									return "BC4_UNORM_BLOCK";
            case VK_FORMAT_BC4_SNORM_BLOCK:									return "BC4_SNORM_BLOCK";
            case VK_FORMAT_BC5_UNORM_BLOCK:									return "BC5_UNORM_BLOCK";
            case VK_FORMAT_BC5_SNORM_BLOCK:									return "BC5_SNORM_BLOCK";
            case VK_FORMAT_BC6H_UFLOAT_BLOCK:								return "BC6H_UFLOAT_BLOCK";
            case VK_FORMAT_BC6H_SFLOAT_BLOCK:								return "BC6H_SFLOAT_BLOCK";
            case VK_FORMAT_BC7_UNORM_BLOCK:									return "BC7_UNORM_BLOCK";
            case VK_FORMAT_BC7_SRGB_BLOCK:									return "BC7_SRGB_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:							return "ETC2_R8G8B8_UNORM_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:							return "ETC2_R8G8B8_SRGB_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:						return "ETC2_R8G8B8A1_UNORM_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:						return "ETC2_R8G8B8A1_SRGB_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:						return "ETC2_R8G8B8A8_UNORM_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:						return "ETC2_R8G8B8A8_SRGB_BLOCK";
            case VK_FORMAT_EAC_R11_UNORM_BLOCK:								return "EAC_R11_UNORM_BLOCK";
            case VK_FORMAT_EAC_R11_SNORM_BLOCK:								return "EAC_R11_SNORM_BLOCK";
            case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:							return "EAC_R11G11_UNORM_BLOCK";
            case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:							return "EAC_R11G11_SNORM_BLOCK";
            case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:							return "ASTC_4x4_UNORM_BLOCK";
            case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:								return "ASTC_4x4_SRGB_BLOCK";
            case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:							return "ASTC_5x4_UNORM_BLOCK";
            case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:								return "ASTC_5x4_SRGB_BLOCK";
            case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:							return "ASTC_5x5_UNORM_BLOCK";
            case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:								return "ASTC_5x5_SRGB_BLOCK";
            case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:							return "ASTC_6x5_UNORM_BLOCK";
            case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:								return "ASTC_6x5_SRGB_BLOCK";
            case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:							return "ASTC_6x6_UNORM_BLOCK";
            case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:								return "ASTC_6x6_SRGB_BLOCK";
            case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:							return "ASTC_8x5_UNORM_BLOCK";
            case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:								return "ASTC_8x5_SRGB_BLOCK";
            case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:							return "ASTC_8x6_UNORM_BLOCK";
            case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:								return "ASTC_8x6_SRGB_BLOCK";
            case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:							return "ASTC_8x8_UNORM_BLOCK";
            case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:								return "ASTC_8x8_SRGB_BLOCK";
            case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:							return "ASTC_10x5_UNORM_BLOCK";
            case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:							return "ASTC_10x5_SRGB_BLOCK";
            case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:							return "ASTC_10x6_UNORM_BLOCK";
            case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:							return "ASTC_10x6_SRGB_BLOCK";
            case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:							return "ASTC_10x8_UNORM_BLOCK";
            case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:							return "ASTC_10x8_SRGB_BLOCK";
            case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:							return "ASTC_10x10_UNORM_BLOCK";
            case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:							return "ASTC_10x10_SRGB_BLOCK";
            case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:							return "ASTC_12x10_UNORM_BLOCK";
            case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:							return "ASTC_12x10_SRGB_BLOCK";
            case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:							return "ASTC_12x12_UNORM_BLOCK";
            case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:							return "ASTC_12x12_SRGB_BLOCK";
            case VK_FORMAT_G8B8G8R8_422_UNORM:								return "G8B8G8R8_422_UNORM";
            case VK_FORMAT_B8G8R8G8_422_UNORM:								return "B8G8R8G8_422_UNORM";
            case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:						return "G8_B8_R8_3PLANE_420_UNORM";
            case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:						return "G8_B8R8_2PLANE_420_UNORM";
            case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:						return "G8_B8_R8_3PLANE_422_UNORM";
            case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:						return "G8_B8R8_2PLANE_422_UNORM";
            case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:						return "G8_B8_R8_3PLANE_444_UNORM";
            case VK_FORMAT_R10X6_UNORM_PACK16:								return "R10X6_UNORM_PACK16";
            case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:						return "R10X6G10X6_UNORM_2PACK16";
            case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:				return "R10X6G10X6B10X6A10X6_UNORM_4PACK16";
            case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:			return "G10X6B10X6G10X6R10X6_422_UNORM_4PACK16";
            case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:			return "B10X6G10X6R10X6G10X6_422_UNORM_4PACK16";
            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:		return "G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16";
            case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:		return "G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16";
            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:		return "G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16";
            case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:		return "G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16";
            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:		return "G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16";
            case VK_FORMAT_R12X4_UNORM_PACK16:								return "R12X4_UNORM_PACK16";
            case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:						return "R12X4G12X4_UNORM_2PACK16";
            case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:				return "R12X4G12X4B12X4A12X4_UNORM_4PACK16";
            case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:			return "G12X4B12X4G12X4R12X4_422_UNORM_4PACK16";
            case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:			return "B12X4G12X4R12X4G12X4_422_UNORM_4PACK16";
            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:		return "G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16";
            case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:		return "G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16";
            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:		return "G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16";
            case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:		return "G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16";
            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:		return "G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16";
            case VK_FORMAT_G16B16G16R16_422_UNORM:							return "G16B16G16R16_422_UNORM";
            case VK_FORMAT_B16G16R16G16_422_UNORM:							return "B16G16R16G16_422_UNORM";
            case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:					return "G16_B16_R16_3PLANE_420_UNORM";
            case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:						return "G16_B16R16_2PLANE_420_UNORM";
            case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:					return "G16_B16_R16_3PLANE_422_UNORM";
            case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:						return "G16_B16R16_2PLANE_422_UNORM";
            case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:					return "G16_B16_R16_3PLANE_444_UNORM";
            case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM:						return "G8_B8R8_2PLANE_444_UNORM";
            case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:		return "G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16";
            case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:		return "G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16";
            case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM:						return "G16_B16R16_2PLANE_444_UNORM";
            case VK_FORMAT_A4R4G4B4_UNORM_PACK16:							return "A4R4G4B4_UNORM_PACK16";
            case VK_FORMAT_A4B4G4R4_UNORM_PACK16:							return "A4B4G4R4_UNORM_PACK16";
            case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK:							return "ASTC_4x4_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK:							return "ASTC_5x4_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK:							return "ASTC_5x5_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK:							return "ASTC_6x5_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK:							return "ASTC_6x6_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK:							return "ASTC_8x5_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK:							return "ASTC_8x6_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK:							return "ASTC_8x8_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK:							return "ASTC_10x5_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK:							return "ASTC_10x6_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK:							return "ASTC_10x8_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK:							return "ASTC_10x10_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK:							return "ASTC_12x10_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK:							return "ASTC_12x12_SFLOAT_BLOCK";
            case VK_FORMAT_A1B5G5R5_UNORM_PACK16:							return "A1B5G5R5_UNORM_PACK16";
            case VK_FORMAT_A8_UNORM:										return "A8_UNORM";
            case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:						return "PVRTC1_2BPP_UNORM_BLOCK_IMG";
            case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:						return "PVRTC1_4BPP_UNORM_BLOCK_IMG";
            case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:						return "PVRTC2_2BPP_UNORM_BLOCK_IMG";
            case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:						return "PVRTC2_4BPP_UNORM_BLOCK_IMG";
            case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:						return "PVRTC1_2BPP_SRGB_BLOCK_IMG";
            case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:						return "PVRTC1_4BPP_SRGB_BLOCK_IMG";
            case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:						return "PVRTC2_2BPP_SRGB_BLOCK_IMG";
            case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:						return "PVRTC2_4BPP_SRGB_BLOCK_IMG";
            case VK_FORMAT_R8_BOOL_ARM:										return "R8_BOOL_ARM";
            case VK_FORMAT_R16G16_SFIXED5_NV:								return "R16G16_SFIXED5_NV";
            case VK_FORMAT_R10X6_UINT_PACK16_ARM:							return "R10X6_UINT_PACK16_ARM";
            case VK_FORMAT_R10X6G10X6_UINT_2PACK16_ARM:						return "R10X6G10X6_UINT_2PACK16_ARM";
            case VK_FORMAT_R10X6G10X6B10X6A10X6_UINT_4PACK16_ARM:			return "R10X6G10X6B10X6A10X6_UINT_4PACK16_ARM";
            case VK_FORMAT_R12X4_UINT_PACK16_ARM:							return "R12X4_UINT_PACK16_ARM";
            case VK_FORMAT_R12X4G12X4_UINT_2PACK16_ARM:						return "R12X4G12X4_UINT_2PACK16_ARM";
            case VK_FORMAT_R12X4G12X4B12X4A12X4_UINT_4PACK16_ARM:			return "R12X4G12X4B12X4A12X4_UINT_4PACK16_ARM";
            case VK_FORMAT_R14X2_UINT_PACK16_ARM:							return "R14X2_UINT_PACK16_ARM";
            case VK_FORMAT_R14X2G14X2_UINT_2PACK16_ARM:						return "R14X2G14X2_UINT_2PACK16_ARM";
            case VK_FORMAT_R14X2G14X2B14X2A14X2_UINT_4PACK16_ARM:			return "R14X2G14X2B14X2A14X2_UINT_4PACK16_ARM";
            case VK_FORMAT_R14X2_UNORM_PACK16_ARM:							return "R14X2_UNORM_PACK16_ARM";
            case VK_FORMAT_R14X2G14X2_UNORM_2PACK16_ARM:					return "R14X2G14X2_UNORM_2PACK16_ARM";
            case VK_FORMAT_R14X2G14X2B14X2A14X2_UNORM_4PACK16_ARM:			return "R14X2G14X2B14X2A14X2_UNORM_4PACK16_ARM";
            case VK_FORMAT_G14X2_B14X2R14X2_2PLANE_420_UNORM_3PACK16_ARM:	return "G14X2_B14X2R14X2_2PLANE_420_UNORM_3PACK16_ARM";
            case VK_FORMAT_G14X2_B14X2R14X2_2PLANE_422_UNORM_3PACK16_ARM:	return "G14X2_B14X2R14X2_2PLANE_422_UNORM_3PACK16_ARM";
            case VK_FORMAT_MAX_ENUM:
                break;
            default: return "Unknown Format";
		}
    }

    static const char *ColorSpaceToString(const VkColorSpaceKHR colorSpace)
    {
        switch (colorSpace)
        {
            case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:				return "SRGB_NONLINEAR_KHR";
            case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:		return "DISPLAY_P3_NONLINEAR_EXT";
            case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:		return "EXTENDED_SRGB_LINEAR_EXT";
            case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT:			return "DISPLAY_P3_LINEAR_EXT";
            case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT:			return "DCI_P3_NONLINEAR_EXT";
            case VK_COLOR_SPACE_BT709_LINEAR_EXT:				return "BT709_LINEAR_EXT";
            case VK_COLOR_SPACE_BT709_NONLINEAR_EXT:			return "BT709_NONLINEAR_EXT";
            case VK_COLOR_SPACE_BT2020_LINEAR_EXT:				return "BT2020_LINEAR_EXT";
            case VK_COLOR_SPACE_HDR10_ST2084_EXT:				return "HDR10_ST2084_EXT";
            case VK_COLOR_SPACE_DOLBYVISION_EXT:				return "DOLBY_VISION_EXT";
            case VK_COLOR_SPACE_HDR10_HLG_EXT:					return "HDR10_HLG_EXT";
            case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:			return "ADOBE_RGB_LINEAR_EXT";
            case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT:			return "ADOBE_RGB_NONLINEAR_EXT";
            case VK_COLOR_SPACE_PASS_THROUGH_EXT:				return "PASS_THROUGH_EXT";
            case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT:	return "EXTENDED_SRGB_NONLINEAR_EXT";
            case VK_COLOR_SPACE_DISPLAY_NATIVE_AMD:				return "DISPLAY_NATIVE_AMD";
            case VK_COLOR_SPACE_MAX_ENUM_KHR:  break;
            default:  return "Unknown Color Space";
        }

        return "Unknown Color Space";
    }

    //static VkSurfaceKHR s_Surface = VK_NULL_HANDLE;

    // -------------------------------------------------------

    Swapchain::Swapchain()
    {
       m_Device = RenderContext::Get()->GetDevice();

       VkSurfaceKHR surface = VK_NULL_HANDLE;

       SDL_Window *sdlWindow = Window::GetWindow();
       if (!sdlWindow)
       {
           SEDX_CORE_ERROR_TAG("Swapchain", "SDL window is null — cannot create Vulkan surface");
           return;
       }

       VkInstance instance = RenderContext::Get()->GetInstance();
       if (instance == VK_NULL_HANDLE)
       {
           SEDX_CORE_ERROR_TAG("Swapchain", "Vulkan instance is null — cannot create Vulkan surface");
           return;
       }

       // SDL3's SDL_Vulkan_CreateSurface returns bool, not VkResult
       if (!SDL_Vulkan_CreateSurface(sdlWindow, instance, nullptr, &surface))
       {
           SEDX_CORE_ERROR_TAG("Swapchain", "Failed to create Vulkan surface: {}", SDL_GetError());
           return;
       }

       m_Surface = surface;
	   SEDX_CORE_ASSERT(m_Surface != VK_NULL_HANDLE, "Vulkan surface creation failed");
       SEDX_CORE_TRACE_TAG("Swapchain", "Vulkan surface created successfully");
    }

    void Swapchain::Create(VkSurfaceKHR surface, uint32_t queueFamilyIndex, VmaAllocator allocator)
	{

		// Query surface formats and pick a reasonable default.
		uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device->GetPhysicalDevice(), surface, &formatCount, nullptr);
		if (formatCount == 0)
		{
			SEDX_CORE_ERROR_TAG("Swapchain", "No surface formats available");
			return;
		}

		std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device->GetPhysicalDevice(), surface, &formatCount, formats.data());
		VkSurfaceFormatKHR surfaceFormat = formats[0];
        SEDX_CORE_TRACE_TAG("Swapchain", "Available surface formats: {}", formatCount);
        SEDX_CORE_TRACE_TAG("Swapchain", "Preferred format: {}.{} ({} formats available)",
                            FormatToString(surfaceFormat.format), ColorSpaceToString(surfaceFormat.colorSpace), formatCount);

		for (auto &f : formats)
		{
			if (f.format == VK_FORMAT_B8G8R8A8_SRGB)
			{
			    surfaceFormat = f; break;
			}
		}
	
		VkSurfaceCapabilitiesKHR caps{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device->GetPhysicalDevice(), surface, &caps);
		SEDX_CORE_TRACE_TAG("Swapchain", "Surface capabilities: minImageCount={}, maxImageCount={}, currentExtent=({}, {})",
			caps.minImageCount, caps.maxImageCount, caps.currentExtent.width, caps.currentExtent.height);

		// Verify the selected queue family supports presentation to this surface.
		VkBool32 presentSupported = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_Device->GetPhysicalDevice(), queueFamilyIndex, surface, &presentSupported);
		if (!presentSupported)
		{
			SEDX_CORE_ERROR_TAG("Swapchain", "Selected queue family does not support presentation");
			return;
		}
	
		// (no debug prints)
		VkExtent2D extent = caps.currentExtent;
		if (std::cmp_equal(extent.width, -1))
		{
		    extent = {.width = 640, .height = 480};
		}
	
		uint32_t imageCount = caps.minImageCount + 1;
		if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
		{
		    imageCount = caps.maxImageCount;
		}
	
		VkSwapchainCreateInfoKHR ci{};
		ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		ci.surface = surface;
		ci.minImageCount = imageCount;
		ci.imageFormat = surfaceFormat.format;
		ci.imageColorSpace = surfaceFormat.colorSpace;
		ci.imageExtent = extent;
		ci.imageArrayLayers = 1;
		ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ci.preTransform = caps.currentTransform;
		ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		ci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		ci.clipped = VK_TRUE;

		/** 
		 * If we already have a swapchain, pass it as 'oldSwapchain' to the
		 * create info so the implementation can recycle resources safely.
		 */
		VkSwapchainKHR oldSwap = m_Swapchain;
		if (oldSwap != VK_NULL_HANDLE)
		{
			ci.oldSwapchain = oldSwap;
		}
	
		VkSwapchainKHR newSwap = VK_NULL_HANDLE;
        VkResult result = vkCreateSwapchainKHR(m_Device->GetLogicalDevice(), &ci, nullptr, &newSwap);
        SEDX_VK_RESULT_ASSERT(result, "Failed to create swapchain")
	
		// Fetch images for the new swapchain first
		uint32_t imgCount = 0;
		vkGetSwapchainImagesKHR(m_Device->GetLogicalDevice(), newSwap, &imgCount, nullptr);
		std::vector<VkImage> newImages(imgCount);
		vkGetSwapchainImagesKHR(m_Device->GetLogicalDevice(), newSwap, &imgCount, newImages.data());
	
		// Create image views for the new images
		std::vector<VkImageView> newImageViews(imgCount, VK_NULL_HANDLE);
		for (uint32_t i = 0; i < imgCount; ++i)
		{
			VkImageViewCreateInfo viewCI{};
			viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCI.image = newImages[i];
			viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCI.format = surfaceFormat.format;
			viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewCI.subresourceRange.levelCount = 1;
			viewCI.subresourceRange.layerCount = 1;
			SEDX_VK_RESULT_ASSERT(vkCreateImageView(m_Device->GetLogicalDevice(), &viewCI, nullptr, &newImageViews[i]), "Failed to create image view")
		}
	
		// Create a new depth image for the new extent
		VkImage newDepthImage = VK_NULL_HANDLE;
		VmaAllocation newDepthAlloc = VK_NULL_HANDLE;
		VkImageView newDepthView = VK_NULL_HANDLE;

		VkImageCreateInfo depthImageCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = m_DepthFormat,
			.extent{.width = extent.width, .height = extent.height, .depth = 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		VmaAllocationCreateInfo allocCI{
		    .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		    .usage = VMA_MEMORY_USAGE_AUTO };
        SEDX_VK_RESULT_ASSERT(vmaCreateImage(allocator, &depthImageCI, &allocCI, &newDepthImage, &newDepthAlloc, nullptr), "Failed to create depth image")

		VkImageViewCreateInfo depthViewCI{};
		depthViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depthViewCI.image = newDepthImage;
		depthViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthViewCI.format = m_DepthFormat;
		depthViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		depthViewCI.subresourceRange.levelCount = 1;
		depthViewCI.subresourceRange.layerCount = 1;
		SEDX_VK_RESULT_ASSERT(vkCreateImageView(m_Device->GetLogicalDevice(), &depthViewCI, nullptr, &newDepthView), "Failed to create depth image view")
	
		// At this point the new swapchain and its images/views/depth exist. Now
		// we can safely destroy old resources (if any) and update our members.
		if (oldSwap != VK_NULL_HANDLE)
		{
			// Destroy old image views
			for (auto &iv : m_ImageViews)
			{
			    if (iv != VK_NULL_HANDLE) vkDestroyImageView(m_Device->GetLogicalDevice(), iv, nullptr);
			}

			// Destroy old depth resources
			if (m_DepthView != VK_NULL_HANDLE)
			{
			    vkDestroyImageView(m_Device->GetLogicalDevice(), m_DepthView, nullptr); m_DepthView = VK_NULL_HANDLE;
			}

			if (m_DepthImage != VK_NULL_HANDLE)
			{
			    vmaDestroyImage(MemoryAllocator::GetAllocator(), m_DepthImage, m_DepthAlloc); m_DepthImage = VK_NULL_HANDLE; m_DepthAlloc = VK_NULL_HANDLE;
			}

			// Destroy old swapchain handle
			if (m_Swapchain != VK_NULL_HANDLE)
			{
			    vkDestroySwapchainKHR(m_Device->GetLogicalDevice(), m_Swapchain, nullptr);
			}
		}
	
		// Tick internal state to the newly created resources
		m_Swapchain = newSwap;
		m_Images = std::move(newImages);
		m_ImageViews = std::move(newImageViews);
		m_ImageFormat = surfaceFormat.format;
		m_Extent = extent;
		m_DepthImage = newDepthImage;
		m_DepthAlloc = newDepthAlloc;
		m_DepthView = newDepthView;
	
		// Creation Succeeded
        m_ImageIndex = 0;
        m_ImageAcquired = false;
		SEDX_CORE_TRACE_TAG("Swapchain", "Swapchain created successfully with {} images (format: {}, extent: {}x{})",
                            imgCount, FormatToString(surfaceFormat.format),
                            extent.width, extent.height);
		return;
	}

    void Swapchain::Recreate(VkSurfaceKHR surface, uint32_t queueFamilyIndex, VmaAllocator allocator)
	{
        Ref<Device> device = RenderContext::Get()->GetDevice();
		// Centralized recreation flow:
		//  1. Wait for device idle to ensure no commands reference swapchain resources.
		//  2. Query surface capabilities (create() will also query internally but
		//     keeping the wait and capability refresh here keeps caller simpler).
		//  3. Call create() which will create the new swapchain and swap resources.
		// The create() implementation already handles passing the old swapchain
		// through the create-info, and replaces resources after the new ones are
		// successfully created.
		vkDeviceWaitIdle(device->GetLogicalDevice());
		VkSurfaceCapabilitiesKHR caps{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->GetPhysicalDevice(), surface, &caps);
		(void)caps; // currently unused here but helpful for future policies
		Create(surface, queueFamilyIndex, allocator);
	}

    void Swapchain::AcquireNextImage()
    {
        // Reset acquisition state
        m_ImageAcquired = false;

        // When the window is minimized acquisition will fail and it's not necessary either
        if (Window::IsMinimized())
            return;

        // Ensure swapchain is valid
        if (!m_Swapchain)
            return;

        // Try to acquire, with retry after swapchain recreation
        for (uint32_t attempt = 0; attempt < 2; ++attempt)
        {
            // use per-image FrameSync objects indexed by the current semaphore_index
            // this avoids reusing a semaphore that may still be in use by presentation
            FrameSync *frameSync = m_Acquired_Semaphore[m_ImageIndex].Get();
            SEDX_CORE_ASSERT(frameSync != nullptr, "FrameSync for acquired semaphore is null");

            // ensure the semaphore is free; wait for any command list that used this semaphore
            if (CommandList* cmdList = frameSync->GetUserCmdList())
            {
                if (cmdList->GetState() == CommandState::Submitted)
                { 
                    cmdList->WaitForExecution();
                }
                SEDX_CORE_ASSERT(cmdList->GetState() == CommandState::Idle);
            }
			
            VkSemaphore vkSemaphore = frameSync->GetVkSemaphore();

            VkResult r = vkAcquireNextImageKHR(m_Device->GetLogicalDevice(), m_Swapchain, 100000000 /*100ms timeout*/, vkSemaphore, VK_NULL_HANDLE, &m_ImageIndex);
            SEDX_VK_RESULT_ASSERT(r, "Failed to acquire next swapchain image")

            if (r == VK_SUCCESS || r == VK_SUBOPTIMAL_KHR)
            {
                m_ImageAcquired = true;
                return;
            }

            if (r == VK_ERROR_OUT_OF_DATE_KHR)
            {
                // Swapchain is out of date (e.g. window resized), recreate the swapchain and try acquiring again.
                Recreate(m_Surface, m_Device->GetQueueManager()->GetFamilyIndexByType(Graphics), MemoryAllocator::GetAllocator());
            }
            else
            {
                SEDX_CORE_ERROR_TAG("Swapchain", "Failed to acquire swapchain image: {}", r);
                return;
            }
        }
    }

    VkResult Swapchain::Present(VkQueue presentQueue, uint32_t imageIndex, VkSemaphore waitSemaphore)
    {
        VkSwapchainKHR swapchain = m_Swapchain;

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = (waitSemaphore != VK_NULL_HANDLE) ? 1u : 0u;
        presentInfo.pWaitSemaphores = (waitSemaphore != VK_NULL_HANDLE) ? &waitSemaphore : nullptr;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;

        VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            SEDX_CORE_TRACE_TAG("Swapchain", "Swapchain out of date or suboptimal during present — recreation needed");
        }

        SEDX_VK_RESULT_ASSERT(result, "Failed to present swapchain image");

        return result;
    }

    void Swapchain::Destroy()
	{
        Ref<Device> device = RenderContext::Get()->GetDevice(); // Avoid passing VkDevice and just fetch it from the RenderContext singleton
		if (m_DepthView != VK_NULL_HANDLE)
		{
		    vkDestroyImageView(device->GetLogicalDevice(), m_DepthView, nullptr); m_DepthView = VK_NULL_HANDLE;
		}

		if (m_DepthImage != VK_NULL_HANDLE)
		{
		    vmaDestroyImage(MemoryAllocator::GetAllocator(), m_DepthImage, m_DepthAlloc); m_DepthImage = VK_NULL_HANDLE; m_DepthAlloc = VK_NULL_HANDLE;
		}

		for (auto &iv : m_ImageViews)
		{
		    if (iv != VK_NULL_HANDLE)
		    {
		        vkDestroyImageView(device->GetLogicalDevice(), iv, nullptr);
		    }
		}

		m_ImageViews.clear();
		m_Images.clear();

		if (m_Swapchain != VK_NULL_HANDLE)
		{
		    vkDestroySwapchainKHR(device->GetLogicalDevice(), m_Swapchain, nullptr);
		    m_Swapchain = VK_NULL_HANDLE;
		}

		if (m_Surface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(device->GetInstance(), m_Surface, nullptr);
			m_Surface = VK_NULL_HANDLE;
        }
	}

}

// -------------------------------------------------------

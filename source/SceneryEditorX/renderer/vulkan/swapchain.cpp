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
#include "SceneryEditorX/core/events/event_system.h"
#include "SceneryEditorX/core/time/fps_timer.h"
#include "SceneryEditorX/core/time/timer.h"
#include "SceneryEditorX/core/window/monitor_data.h"
#include "SceneryEditorX/core/window/window.h"
#include <tlhelp32.h>
#include <utility>
#include <vector>
#include <SDL3/SDL_vulkan.h>
#include <tracy/Tracy.hpp>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	
    #pragma region Static Properties

    /**
     * @brief Get the string representation of a Vulkan format.
     * @param format The Vulkan format.
     * @return The string representation of the format.
     */
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

    /**
     * @brief Get the string representation of a Vulkan color space.
     * @param colorSpace The Vulkan color space.
     * @return The string representation of the color space.
     */
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

    /**
     * @brief Get the surface capabilities for the given surface.
     * @param surface The Vulkan surface.
     * @return The surface capabilities.
     */
    static VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(const VkSurfaceKHR surface)
    {
        Ref<Device> device = RenderContext::Get()->GetDevice();
		VkSurfaceCapabilitiesKHR caps{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->GetPhysicalDevice(), surface, &caps);
		SEDX_CORE_TRACE_TAG("Swapchain", "Surface capabilities: minImageCount={}, maxImageCount={}, currentExtent=({}, {})",
			caps.minImageCount, caps.maxImageCount, caps.currentExtent.width, caps.currentExtent.height);
        return caps;
    }

    /**
     * @brief Get the appropriate color space for the given format.
     * @param format The Vulkan format.
     * @return The corresponding color space.
     */
    static VkColorSpaceKHR GetColorSpace(const VkFormat format)
    {
        VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;                                           // SDR
        colorSpace = format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 ? VK_COLOR_SPACE_HDR10_ST2084_EXT : colorSpace; // HDR
        return colorSpace;
    }

    /**
     * @brief Get the supported surface formats for the given surface.
     * @param surface The Vulkan surface.
     * @return A vector of supported surface formats.
     */
    static std::vector<VkSurfaceFormatKHR> GetSupportedSurfaceFormats(const VkSurfaceKHR surface)
    {
        Ref<Device> device = RenderContext::Get()->GetDevice();
        uint32_t formatCount;
        SEDX_VK_RESULT_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(device->GetPhysicalDevice(), surface, &formatCount, nullptr));

        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        SEDX_VK_RESULT_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(device->GetPhysicalDevice(), surface, &formatCount, surfaceFormats.data()));

        return surfaceFormats;
    }

    /**
     * @brief Get the supported present modes for the given surface.
     * @param surface The Vulkan surface.
     * @return A vector of supported present modes.
     */
    static std::vector<VkPresentModeKHR> GetSupportedPresentModes(const VkSurfaceKHR surface)
    {
        Ref<Device> device = RenderContext::Get()->GetDevice();
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device->GetPhysicalDevice(), surface, &presentModeCount, nullptr);

        std::vector<VkPresentModeKHR> surfacePresentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device->GetPhysicalDevice(), surface, &presentModeCount, surfacePresentModes.data());
        return surfacePresentModes;
    }

    /**
      * @brief Get the appropriate present mode for the given surface and requested mode.
      * @param surface The Vulkan surface.
      * @param mode The requested present mode.
      * @return The supported present mode.
      */
     static VkPresentModeKHR GetPresentMode(const VkSurfaceKHR surface, const VkPresentModeKHR mode)
     {
         VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
         if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
         {
             presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
         }
         else if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
         {
             presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
         }

         // return the present mode as is if the surface supports it
         std::vector<VkPresentModeKHR> surface_present_modes = GetSupportedPresentModes(surface);
         for (const VkPresentModeKHR supported_present_mode : surface_present_modes)
         {
             if (presentMode == supported_present_mode)
             {
                 return presentMode;
             }
         }

         // At this point we call back to VK_PRESENT_MODE_FIFO_KHR, which as per spec is always present
         SEDX_CORE_WARN_TAG("Swapchain", "Requested present mode is not supported. Falling back to VK_PRESENT_MODE_FIFO_KHR");
         return VK_PRESENT_MODE_FIFO_KHR;
     }

     /**
     * @brief Check if the given format and color space are supported by the surface.
     * @param surface The Vulkan surface.
     * @param format The Vulkan format.
     * @param colorSpace The Vulkan color space.
     * @return True if the format and color space are supported, false otherwise.
     */
	static bool IsFormatandColorSpaceSupported(const VkSurfaceKHR surface, VkFormat format, const VkColorSpaceKHR colorSpace)
	{
		Ref<Device> device = RenderContext::Get()->GetDevice();
		std::vector<VkSurfaceFormatKHR> supportedFormats = GetSupportedSurfaceFormats(surface);

		// Detect NVIDIA by querying the active VkPhysicalDevice properties instead of trying
		// to instantiate the internal HWDeviceInfo (incomplete here).
		bool isNvidia = false;
        if (VkPhysicalDevice phys = device->GetPhysicalDevice(); phys != VK_NULL_HANDLE)
		{
		    VkPhysicalDeviceProperties props{};
		    vkGetPhysicalDeviceProperties(phys, &props);

		    // Vendor ID 0x10DE identifies NVIDIA; also check the device name for robustness.
		    isNvidia = (props.vendorID == 0x10DE) ||
		               (strstr(props.deviceName, "Nvidia") != nullptr) ||
		               (strstr(props.deviceName, "nvidia") != nullptr);
		}
		else
		{
		    SEDX_CORE_WARN_TAG("Swapchain", "Physical device handle is null while checking vendor");
		}

		// NV historically exposes BGR ordering in some presentation formats on Windows.
		if (format == VK_FORMAT_R8G8B8A8_UNORM && isNvidia)
		{
		    format = VK_FORMAT_B8G8R8A8_UNORM;
		}

		for (const VkSurfaceFormatKHR& supportedFormat : supportedFormats)
		{
		    bool supportFormat     = supportedFormat.format == format;
		    bool supportColorSpace = supportedFormat.colorSpace == colorSpace;

		    if (supportFormat && supportColorSpace)
		    {
		        return true;
		    }
		}

		return false;
	}

    /**
     * @brief Get the supported composite alpha flags for the given surface.
     * @param surface The Vulkan surface.
     * @return The supported composite alpha flags.
     */
    static VkCompositeAlphaFlagBitsKHR GetCompositeAlphaFlags(const VkSurfaceKHR surface)
    {
        Ref<Device> device = RenderContext::Get()->GetDevice();
		std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags =
		{
		    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		    VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		    VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		    VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		};

		
		// get physical device surface capabilities
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		SEDX_VK_RESULT_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->GetPhysicalDevice(), surface, &surfaceCapabilities));

		// simply select the first composite alpha format available
		for (VkCompositeAlphaFlagBitsKHR& compositeAlpha : compositeAlphaFlags)
		{
		    if (surfaceCapabilities.supportedCompositeAlpha & compositeAlpha)
		    {
		        return compositeAlpha;
		    };
		}

		return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }

    /**
     * @brief Set the HDR specifications for the given swapchain.
     * @param swapchain The Vulkan swapchain.
     */
    static void SetHDRSpecs(const VkSwapchainKHR &swapchain)
    {
        Ref<Device> device = RenderContext::Get()->GetDevice();
        VkHdrMetadataEXT hdrMetadata          = {};
        hdrMetadata.sType                     = VK_STRUCTURE_TYPE_HDR_METADATA_EXT;
        hdrMetadata.displayPrimaryRed.x       = 0.708f;
        hdrMetadata.displayPrimaryRed.y       = 0.292f;
        hdrMetadata.displayPrimaryGreen.x     = 0.170f;
        hdrMetadata.displayPrimaryGreen.y     = 0.797f;
        hdrMetadata.displayPrimaryBlue.x      = 0.131f;
        hdrMetadata.displayPrimaryBlue.y      = 0.046f;
        hdrMetadata.whitePoint.x              = 0.3127f;
        hdrMetadata.whitePoint.y              = 0.3290f;
        const float nitsToLumin                = 10000.0f;
        hdrMetadata.maxLuminance              = MonitorData::GetLuminanceMax() * nitsToLumin;
        hdrMetadata.minLuminance              = 0.001f * nitsToLumin;
        hdrMetadata.maxContentLightLevel      = 2000.0f;
        hdrMetadata.maxFrameAverageLightLevel = 500.0f;

        PFN_vkSetHdrMetadataEXT pfnVkSetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT)vkGetDeviceProcAddr(device->GetLogicalDevice(), "vkSetHdrMetadataEXT");
        SEDX_CORE_ASSERT(pfnVkSetHdrMetadataEXT != nullptr);
        pfnVkSetHdrMetadataEXT(device->GetLogicalDevice(), 1, &swapchain, &hdrMetadata);
    }

    #pragma endregion

    // -------------------------------------------------------

    Swapchain::Swapchain()
    {
        m_Device = RenderContext::Get()->GetDevice();

        SDL_Window *sdlWindow = Window::GetWindow();
		if (!sdlWindow)
		{
		    SEDX_CORE_ERROR_TAG("Swapchain", "SDL3 window is null, cannot create Vulkan surface");
		    return;
		}

		VkInstance instance = RenderContext::Get()->GetInstance();
        if (instance == VK_NULL_HANDLE)
		{
		    SEDX_CORE_ERROR_TAG("Swapchain", "Vulkan instance is null — cannot create Vulkan surface");
		    return;
		}

		SEDX_CORE_ASSERT(SDL_Vulkan_CreateSurface(sdlWindow, instance, nullptr, &m_Surface), "Failed to create Vulkan surface for SDL window");

	    SEDX_CORE_ASSERT(m_Surface != VK_NULL_HANDLE, "Vulkan surface creation failed");
        SEDX_CORE_TRACE_TAG("Swapchain", "Vulkan surface created successfully");

		CreateSwapchain();

    }

    Swapchain::~Swapchain()
    {
        Ref<Device> device = RenderContext::Get()->GetDevice();

		if (m_DepthView != VK_NULL_HANDLE)
		{
            QueueManager::AddDeletionQueue(ResourceType::ImageView, m_DepthView);
            m_DepthView = VK_NULL_HANDLE;
		}

		if (m_DepthImage != VK_NULL_HANDLE)
		{
            QueueManager::AddDeletionQueue(ResourceType::Image, m_DepthImage);
            m_DepthImage = VK_NULL_HANDLE;
		}

		for (auto &imgView : m_ImageViews)
		{
		    if (imgView != VK_NULL_HANDLE)
		    {
				QueueManager::AddDeletionQueue(ResourceType::ImageView, imgView);
                imgView = VK_NULL_HANDLE;
		    }
		}

		if (m_Swapchain != VK_NULL_HANDLE)
		{
		    vkDestroySwapchainKHR(device->GetLogicalDevice(), m_Swapchain, nullptr);
		    m_Swapchain = VK_NULL_HANDLE;
		}

        if (m_Swapchain)
		{
			vkDestroySurfaceKHR(RenderContext::Get()->GetInstance(), m_Surface, nullptr);
			m_Surface = VK_NULL_HANDLE;
        }
    }

    void Swapchain::CreateSwapchain()
	{
        SEDX_CORE_ASSERT(m_Surface != VK_NULL_HANDLE, "Cannot create swapchain without a valid surface");

		VkSurfaceCapabilitiesKHR capabilities = GetSurfaceCapabilities(m_Surface);

        // skip if window is minimized
        if (capabilities.currentExtent.width == 0 || capabilities.currentExtent.height == 0)
        {
            SEDX_CORE_WARN_TAG("Swapchain","Window is minimized, swapchain creation skipped");
            return;
        }
    
        // Check surface supports the requested format and color space, fall back to SDR if not supported
        VkColorSpaceKHR colorSpace = GetColorSpace(m_ImageFormat);
		if (!IsFormatandColorSpaceSupported(m_Surface, m_ImageFormat, colorSpace))
		{
			SEDX_CORE_WARN_TAG("Swapchain", "Preferred format {} with color space {} not supported, falling back to SDR",
								FormatToString(m_ImageFormat), ColorSpaceToString(colorSpace));
			colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		}

		QueueManager::WaitIdleAll();

        // clamp size
        m_Width  = std::ranges::clamp(m_Width,  capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        m_Height = std::ranges::clamp(m_Height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    
		// (no debug prints)
		VkExtent2D extent = capabilities.currentExtent;
		if (std::cmp_equal(extent.width, -1))
		{
		    extent = {.width = 640, .height = 480};
		}
	
		uint32_t imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
		{
		    imageCount = capabilities.maxImageCount;
		}
	
		VkSwapchainCreateInfoKHR ci = {};
		ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		ci.surface = m_Surface;
		ci.minImageCount = imageCount;
		ci.imageFormat = m_ImageFormat;
		ci.imageColorSpace = colorSpace;
		ci.imageExtent = extent;
		ci.imageArrayLayers = 1;
		ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ci.preTransform = capabilities.currentTransform;
		ci.compositeAlpha = GetCompositeAlphaFlags(m_Surface);
		ci.presentMode = GetPresentMode(m_Surface, m_PresentMode);
		ci.clipped = VK_TRUE;
		ci.oldSwapchain = m_Swapchain;

        SEDX_VK_RESULT_ASSERT(vkCreateSwapchainKHR(m_Device->GetLogicalDevice(), &ci, nullptr, &m_Swapchain), "Failed to create swapchain");
	
        // Destroy old swapchain if it existed
        if (ci.oldSwapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_Device->GetLogicalDevice(), ci.oldSwapchain, nullptr);
        }

		// Fetch images for the new swapchain first
		uint32_t imgCount = 0;
		vkGetSwapchainImagesKHR(m_Device->GetLogicalDevice(), m_Swapchain, &imgCount, nullptr);
		vkGetSwapchainImagesKHR(m_Device->GetLogicalDevice(), m_Swapchain, &imgCount, m_Images.data());
	
		// Create image views for the new images
		for (uint32_t i = 0; i < imageCount; ++i)
		{
            if (m_ImageViews[i])
			{
                QueueManager::AddDeletionQueue(ResourceType::ImageView, m_ImageViews[i]);
            }

			VkImageViewCreateInfo viewCI = {};
			viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCI.image = m_Images[i];
			viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCI.format = m_ImageFormat;
			viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewCI.subresourceRange.baseMipLevel = 0;
		    viewCI.subresourceRange.baseArrayLayer = 0;
			viewCI.subresourceRange.levelCount = 1;
			viewCI.subresourceRange.layerCount = 1;
		    viewCI.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            };
			SEDX_VK_RESULT_ASSERT(vkCreateImageView(m_Device->GetLogicalDevice(), &viewCI, nullptr, &m_ImageViews[i]), "Failed to create image view")
		}

        // sync primitives - per-image semaphores to avoid reuse conflicts
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_AcquiredSemaphore.size()); i++)
        {
            m_AcquiredSemaphore[i] = CreateRef<FrameSync>(SyncType::Semaphore);
            m_CompleteSemaphore[i] = CreateRef<FrameSync>(SyncType::Semaphore);
        }

        if (m_ImageFormat == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
        {
            SetHDRSpecs(m_Swapchain);
        }

        m_ImageIndex    = 0;
        m_SemaphoreIndex  = 0;
        m_ImageAcquired = false;
	
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

		VmaAllocationCreateInfo allocCI{};
        allocCI.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		allocCI.usage = VMA_MEMORY_USAGE_AUTO;
        SEDX_VK_RESULT_ASSERT(vmaCreateImage(MemoryAllocator::GetAllocator(), &depthImageCI, &allocCI, &newDepthImage, &newDepthAlloc, nullptr), "Failed to create depth image")

		VkImageViewCreateInfo depthViewCI{};
		depthViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depthViewCI.image = newDepthImage;
		depthViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthViewCI.format = m_DepthFormat;
		depthViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		depthViewCI.subresourceRange.levelCount = 1;
		depthViewCI.subresourceRange.layerCount = 1;
		SEDX_VK_RESULT_ASSERT(vkCreateImageView(m_Device->GetLogicalDevice(), &depthViewCI, nullptr, &newDepthView), "Failed to create depth image view")

		// Creation Succeeded
        m_ImageIndex = 0;
        m_ImageAcquired = false;
		SEDX_CORE_TRACE_TAG("Swapchain", "Swapchain created successfully with {} images (format: {}, extent: {}x{})",
                            imgCount, FormatToString(m_ImageFormat),
                            extent.width, extent.height);
	}

    void Swapchain::Recreate()
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
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->GetPhysicalDevice(), m_Surface, &caps);
		(void)caps; // currently unused here but helpful for future policies
		CreateSwapchain();
	}

    void Swapchain::Resize(const uint32_t width, const uint32_t height)
    {
        SDL_Window *window = Window::Get().GetWindow();
        SEDX_CORE_ASSERT(window != nullptr, "Cannot resize swapchain without a valid SDL window");

        if (m_Width == width && m_Height == height)
            return;

        m_Width  = width;
        m_Height = height;

		CreateSwapchain();
    }

    void Swapchain::AcquireNextImage()
    {
        // Reset acquisition state
        m_ImageAcquired = false;

        // When the window is minimized acquisition will fail and it's not necessary either
        if (Window::IsMinimized())
            return;

        // Ensure swapchain is valid
        SEDX_CORE_ASSERT(m_Swapchain != VK_NULL_HANDLE, "Swapchain is not valid for image acquisition");

        // Try to acquire, with retry after swapchain recreation
        for (uint32_t attempt = 0; attempt < 2; ++attempt)
        {
            // use per-image FrameSync objects indexed by the current semaphore_index
            // this avoids reusing a semaphore that may still be in use by presentation
            FrameSync *frameSync = m_AcquiredSemaphore[m_ImageIndex].Get();
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

            if (r == VK_SUCCESS)
            {
                m_ImageAcquired = true;
                return;
            }

            if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR)
            {
                // Swapchain is out of date (e.g. window resized), recreate the swapchain and try acquiring again.
                Recreate();
            }

            SEDX_CORE_ERROR_TAG("Swapchain", "Failed to acquire swapchain image: {}", r);
            return;
        }
    }

    VkResult Swapchain::Present(VkQueue presentQueue, uint32_t imageIndex, VkSemaphore waitSemaphore)
    {
        // only present if we successfully acquired an image
        if (!m_ImageAcquired)
            return VK_ERROR_OUT_OF_DATE_KHR;

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = (waitSemaphore != VK_NULL_HANDLE) ? 1u : 0u;
        presentInfo.pWaitSemaphores = (waitSemaphore != VK_NULL_HANDLE) ? &waitSemaphore : nullptr;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_Swapchain;
        presentInfo.pImageIndices = &imageIndex;

        VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
		
        // clear acquisition state after presentation
        m_ImageAcquired = false;

        // recreate the swapchain if needed - we do it here so that no semaphores are being destroyed while they are being waited for
        if (m_IsDirty)
        {
            CreateSwapchain();
            m_IsDirty = false;
        }

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
			Recreate();
            SEDX_CORE_TRACE_TAG("Swapchain", "Swapchain out of date or suboptimal during present — recreation needed");
        }

        SEDX_VK_RESULT_ASSERT(result, "Failed to present swapchain image");

        return result;
    }

    #pragma region VSync Functions

    void Swapchain::SetVsync(const bool enabled)
    {
        if ((m_PresentMode == VK_PRESENT_MODE_FIFO_KHR) != enabled)
        {
            m_PresentMode = enabled ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
            m_IsDirty     = true;
            FPSTimer::OnVSyncToggled(enabled);
        }
    }

    bool Swapchain::GetVsync() const
    {
        // for v-sync, we could Mailbox for lower latency, but fifo is always supported, so we'll assume that
        return m_PresentMode == VK_PRESENT_MODE_FIFO_KHR;
    }

    #pragma endregion

}

// -------------------------------------------------------

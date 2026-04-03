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
 * barrier_info.h
 * -------------------------------------------------------
 * Created: 28/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/renderer/renderer_declarations.h>
#include <SceneryEditorX/renderer/vulkan/enums.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    class ImageResource;

    /**
     * @struct Barrier
     * @brief Represents a unified description of a barrier, 
     * which can be used for image layout transitions, image synchronization, or buffer synchronization.
     */
    struct Barrier
    {
        /**
         * @enum Type
         * @brief Represents the type of barrier, which can be an image layout transition, 
         * an image synchronization barrier, or a buffer synchronization barrier. 
         */
        enum class Type : uint8_t
        {
            ImageLayout, // layout transition
            ImageSync,   // execution/memory barrier, no layout change
            BufferSync   // buffer memory barrier
        };

        Type type = Type::ImageLayout;

        // scope control - defaults to auto for backwards compatibility
        BarrierScope scope_src = BarrierScope::Auto;
        BarrierScope scope_dst = BarrierScope::Auto;

        // for image barriers
        ImageResource* img			= nullptr;
        void* image					= nullptr; // raw handle for swapchain images
        VkFormat format				= VkFormat::VK_FORMAT_MAX_ENUM;
        uint32_t mip_index			= 0;
        uint32_t mip_range			= 1;
        uint32_t array_length		= 1;
        Layout::ImageLayout layout  = Layout::ImageLayout::MaxEnum;
        BarrierType sync_type  = BarrierType::EnsureWriteThenRead;

        // for buffer barriers
        Buffer* buffer = nullptr;
        uint64_t offset    = 0;
        uint64_t size      = 0; // 0 = whole buffer

        /**
         * @brief raw image layout transition (for swapchain etc.)
         * @param img 
         * @param new_layout 
         * @param mip 
         * @param range 
         * @return 
         */
        static Barrier ImageLayout(ImageResource* img, Layout::ImageLayout new_layout, uint32_t mip = std::numeric_limits<uint32_t>::max(), uint32_t range = 0)
        {
            Barrier b;
            b.type      = Type::ImageLayout;
            b.img   = img;
            b.layout    = new_layout;
            b.mip_index = mip;
            b.mip_range = range;
            return b;
        }

        /**
         * @brief raw image layout transition (for swapchain etc.)
         * @param img 
         * @param fmt 
         * @param mip 
         * @param range 
         * @param arr_len 
         * @param new_layout 
         * @return 
         */
        static Barrier ImageLayout(void* img, VkFormat fmt, uint32_t mip, uint32_t range, uint32_t arr_len, Layout::ImageLayout new_layout)
        {
            Barrier b;
            b.type         = Type::ImageLayout;
            b.image        = img;
            b.format       = fmt;
            b.mip_index    = mip;
            b.mip_range    = range;
            b.array_length = arr_len;
            b.layout       = new_layout;
            return b;
        }

        /**
         * @brief texture sync barrier (no layout change)
         * @param img 
         * @param sync 
         * @return 
         */
        static Barrier ImageSync(ImageResource* img, BarrierType sync)
        {
            Barrier b;
            b.type		= Type::ImageSync;
            b.img		= img;
            b.sync_type = sync;
            return b;
        }

        /**
         * @brief buffer sync barrier
         * @param buf 
         * @param off 
         * @param sz 
         * @return 
         */
        static Barrier BufferSync(Buffer* buf, uint64_t off = 0, uint64_t sz = 0)
        {
            Barrier b;
            b.type   = Type::BufferSync;
            b.buffer = buf;
            b.offset = off;
            b.size   = sz;
            return b;
        }

        /**
         * @brief 
         * @param scope 
         * @return 
         */
        Barrier& From(BarrierScope scope) { scope_src = scope; return *this; }

        /**
         * @brief 
         * @param scope 
         * @return 
         */
        Barrier& To(BarrierScope scope)   { scope_dst = scope; return *this; }
    };

    /**
     * @struct PendingBarrierInfo
     * @brief Represents information about a pending barrier.
     */
    struct PendingBarrierInfo
    {
        Barrier barrier;
        VkImage image				  = nullptr;
        uint32_t aspect_Mask		  = 0;
        uint32_t mip_Index			  = 0;
        uint32_t mip_Range			  = 0;
        uint32_t array_Length		  = 0;
        Layout::ImageLayout layoutOld = Layout::ImageLayout::MaxEnum;
        Layout::ImageLayout layoutNew = Layout::ImageLayout::MaxEnum;
        bool isDepth				  = false;

        // for image sync with per-mip views (pre-captured layouts at insert time)
        std::array<Layout::ImageLayout, MAX_MIP_COUNT> per_MipLayouts = {};
        uint32_t per_MipCount	= 0;
        bool has_PerMipViews	= false;
    };

}

// -------------------------------------------------------

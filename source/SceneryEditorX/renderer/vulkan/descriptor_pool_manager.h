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
 * descriptor_pool_manager.h
 * -------------------------------------------------------
 * Created: 06/04/2026
 * -------------------------------------------------------
 */
#pragma once
#include "device.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @class DescriptorPoolManager
     * @brief Singleton that owns one large shared VkDescriptorPool for the application lifetime.
     *
     * The pool is created with:
     *   - VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT  — allows individual set return.
     *   - VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT    — required for update-after-bind
     *     layouts (partially-bound bindless arrays).
     *
     * All descriptor set allocations done by DescriptorSet and DescriptorLayout go through
     * this manager, removing the need for per-object mini pools.
     *
     * Usage:
     * @code
     *   DescriptorPoolManager::Init(device);           // once, during renderer startup
     *   VkDescriptorSet set = DescriptorPoolManager::Get().Allocate(layout);
     *   DescriptorPoolManager::Get().Free(set);        // explicit early return (optional)
     *   DescriptorPoolManager::Shutdown();             // once, during renderer shutdown
     * @endcode
     */
    class DescriptorPoolManager
    {
    public:
        DescriptorPoolManager(const DescriptorPoolManager&)            = delete;
        DescriptorPoolManager& operator=(const DescriptorPoolManager&) = delete;

        /**
         * @brief Initialize the singleton with the given logical device.
         *
         * Creates the shared VkDescriptorPool sized for MAX_DESCRIPTOR_SET_COUNT sets
         * and a generous count per descriptor type.  Must be called before any call to
         * Get() or Allocate().
         */
        static void Init();

        /**
         * @brief Destroy the pool and release all associated descriptor sets.
         *
         * Must be called during renderer shutdown before the logical device is destroyed.
         */
        static void Shutdown();

        /**
         * @brief Return the singleton instance.
         * @note Init() must have been called before the first call to Get().
         */
        static DescriptorPoolManager& Get();

        /**
         * @brief Allocate a single VkDescriptorSet from the shared pool.
         *
         * @param layout                Layout to allocate against.
         * @param variableDescriptorCount  For variable-count (bindless) layouts: the actual
         *                                 runtime array size.  Pass 0 for fixed layouts.
         * @return VkDescriptorSet handle, or VK_NULL_HANDLE on failure.
         */
        [[nodiscard]] VkDescriptorSet Allocate(VkDescriptorSetLayout layout, uint32_t variableDescriptorCount = 0);

        /**
         * @brief Return a single descriptor set to the pool.
         *
         * Safe to call with VK_NULL_HANDLE (no-op).
         *
         * @param set Set handle to free.
         */
        void Free(VkDescriptorSet set);

        /**
         * @brief Get the raw pool handle (for external use by BindlessManager, etc.).
         * @return The underlying VkDescriptorPool.
         */
        [[nodiscard]] VkDescriptorPool GetPool() const { return m_Pool; }

        // Allow construction/destruction for the internal static instance.
        // The class remains non-copyable via deleted copy ctor/assign above.
        DescriptorPoolManager() = default;
        ~DescriptorPoolManager() = default;

    private:
        void CreatePool();

        Ref<Device> m_Device;
        VkDescriptorPool m_Pool = VK_NULL_HANDLE;
    };

} // namespace SceneryEditorX

// -------------------------------------------------------

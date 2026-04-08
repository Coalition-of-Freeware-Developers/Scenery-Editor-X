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
 * descriptor_layout.h
 * -------------------------------------------------------
 * Created: 06/04/2026
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX
{
    class Device;

    /**
     * @class DescriptorLayout
     * @brief Immutable ownership wrapper for a single VkDescriptorSetLayout.
     *
     * Constructed from a pre-filled VkDescriptorSetLayoutCreateInfo and an optional
     * debug name.  The underlying Vulkan handle is destroyed via the deferred
     * QueueManager deletion queue when the last Ref<DescriptorLayout> is released.
     *
     * Obtain instances through DescriptorLayout::Create(); never construct directly
     * with `new`.
     */
    class DescriptorLayout : public RefCounted
    {
    public:
        /**
         * @brief Factory: create a DescriptorLayout from a fully populated layout create-info.
         * @param createInfo  Fully populated VkDescriptorSetLayoutCreateInfo.
         * @param debugName   Optional UTF-8 name shown in validation layers / RenderDoc.
         * @return Ref<DescriptorLayout>, or an empty Ref if creation fails.
         */
        static Ref<DescriptorLayout> Create(const VkDescriptorSetLayoutCreateInfo& createInfo, const char* debugName = nullptr);

        /**
         * @brief Destroy the layout.
         *
         * Schedules the VkDescriptorSetLayout for deferred deletion via
         * QueueManager::AddDeletionQueue so that in-flight GPU work is not interrupted.
         */
        virtual ~DescriptorLayout() override;

        // Non-copyable — ownership is managed by Ref<T>
        DescriptorLayout(const DescriptorLayout&)            = delete;
        DescriptorLayout& operator=(const DescriptorLayout&) = delete;

        /**
         * @brief Return the raw Vulkan layout handle.
         * @return VkDescriptorSetLayout, or VK_NULL_HANDLE if creation failed.
         */
        [[nodiscard]] VkDescriptorSetLayout GetLayout() const { return m_Layout; }

        /**
         * @brief Check whether the layout was created successfully.
         * @return True if the layout handle is non-null.
         */
        [[nodiscard]] bool IsValid() const { return m_Layout != VK_NULL_HANDLE; }

        /**
         * Note: constructor must be accessible by CreateRef used in DescriptorLayout::Create.
         * The class is non-copyable and lifetime is managed via Ref<DescriptorLayout>.
         */
        explicit DescriptorLayout(VkDescriptorSetLayout layout);

    private:
        Ref<Device>           m_Device;
        VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
    };

} // namespace SceneryEditorX

// -------------------------------------------------------

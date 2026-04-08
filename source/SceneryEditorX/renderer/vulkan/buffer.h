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
 * buffer.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "memory_allocator.h"
#include <SceneryEditorX/utils/inheritance.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
class CommandList;
/**
	 * @class Buffer
	 * @brief Represents a Vulkan buffer resource, managing its lifecycle and memory allocation.
	 */
	class Buffer : public SharedObject
	{
	public:
		Buffer() = default;

		/**
		 * @brief Constructs a buffer with the specified parameters.
		 * @param stride The stride of each element in the buffer.
		 * @param elementCount The number of elements in the buffer.
		 * @param data The initial data to populate the buffer with.
		 * @param mappable Indicates whether the buffer should be mappable to CPU memory.
		 * @param name The name of the buffer.
		 */
		Buffer(size_t stride, uint32_t elementCount, const void *data, bool mappable, const char *name);

		/**
		 * @brief Creates a Vulkan buffer with the specified parameters.
		 * @param allocator The VMA allocator to use for the buffer.
		 * @param size The size of the buffer.
		 * @param usage The usage flags for the buffer.
		 * @param allocInfo The allocation create info for the buffer.
		 */
		Buffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, const VmaAllocationCreateInfo& allocInfo);

		/* @brief Destroys the buffer and releases its resources. */
		virtual ~Buffer() override;

		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;
		Buffer(Buffer&& other) noexcept;
		Buffer& operator=(Buffer&& other) noexcept;

		/**
		 * @brief Gets the Vulkan buffer handle.
		 * @return The Vulkan buffer handle.
		 */
		[[nodiscard]] VkBuffer Get() const { return m_Buffer; }

		/**
		 * @brief Gets the VMA allocation associated with the buffer.
		 * @return The VMA allocation handle.
		 */
		[[nodiscard]] VmaAllocation Allocation() const { return m_Allocation; }

		/**
		 * @brief Maps the buffer memory to a CPU-accessible address.
		 * @return A pointer to the mapped memory.
		 */
		void* Map();

		/**
		 * @brief Maps the buffer memory to a CPU-accessible address and returns it through the provided reference.
		 * @param resource The buffer resource to map.
		 * @param data A reference to a pointer that will receive the mapped memory address.
		 */
		static void Map(void *resource, void *&data);

		/* @brief Unmaps the buffer memory from a CPU-accessible address. */
		void Unmap();

		/**
		 * @brief Unmaps the buffer memory from a CPU-accessible address.
		 * @param resource The buffer resource to unmap.
		 */
		static void Unmap(void *resource);

		/**
		 * @brief Acquires a staging buffer of the specified size.
		 * @param size The size of the staging buffer to acquire.
		 * @return A pointer to the acquired staging buffer.
		 */
		static void *AcquireStagingBuffer(uint64_t size);

		/**
		 * @brief Releases a staging buffer that was previously acquired.
		 * @param buffer The staging buffer to release.
		 */
		static void ReleaseStagingBuffer(void* buffer);

		/**
		 * @brief Uploads data to a sub-region of the buffer.
		 * @param data The data to upload to the buffer.
		 * @param offsetBytes The offset in bytes within the buffer to start the upload.
		 * @param sizeBytes The size in bytes of the data to upload.
		 */
		void UploadSubRegion(const void* data, uint64_t offsetBytes, uint64_t sizeBytes);

		/**
		 * @brief Gets the device address of the buffer.
		 * @return The device address.
		 */
		VkDeviceAddress DeviceAddress();

		/**
		 * @brief Checks if the buffer is valid.
		 * @return True if the buffer is valid, false otherwise.
		 */
		[[nodiscard]] bool Valid() const { return m_Buffer != VK_NULL_HANDLE; }

		/**
		 * @brief Frees a Vulkan buffer and its associated VMA allocation.
		 * @param buffer The Vulkan buffer to free.
		 * @param allocation The VMA allocation associated with the buffer.
		 */
		static void FreeBuffer(VkBuffer buffer, VmaAllocation allocation);

		/**
		 * @brief Frees a Vulkan image and its associated VMA allocation.
		 * @param image The Vulkan image to free.
		 * @param allocation The VMA allocation associated with the image.
		 */
		static void FreeImageBuffer(VkImage image, VmaAllocation allocation);

		/*
		 * @brief Destroys the buffer and releases its resources. 
		 * @note Explicitly free the underlying VMA resources before m_Allocator destruction
		 */
		void Destroy();

		/**
		 * @brief Gets the stride of the buffer.
		 * @return The stride of the buffer.
		 */
		uint32_t GetStride() const { return m_Stride; }

		/**
		 * @brief Gets the stride of the buffer before alignment.
		 * @return The buffer stride before alignment, used for calculating buffer size and element count.
		 * @note The aligned stride may be larger than the original due to alignment requirements for GPU access.
		 */
		uint32_t GetStrideUnaligned() const { return m_StrideUnaligned; }

		/**
		 * @brief Gets the aligned buffer size based on the stride and element count.
		 * @return aligned buffer size based on the stride and element count, ensuring proper alignment for GPU access 
		 */
		uint64_t GetObjectSize() const { return static_cast<uint64_t>(m_Stride) * static_cast<uint64_t>(m_ElementCount); }

		/**
		 * @brief Updates the buffer with new data, advancing the offset for dynamic updates.
		 * @param cmdList The command list to use for the update.
		 * @param dataCPU A pointer to the CPU data to upload.
		 * @param size The size of the data to upload. If 0, the stride of the buffer is used.
		 */
		void Update(CommandList *cmdList, void* dataCPU, const uint32_t size = 0);

		/**
		 * @brief Gets the current offset of the buffer.
		 * @return The current offset of the buffer.
		 */
		uint32_t GetOffset() const { return m_Offset; }

		/* @brief Resets the offset of the buffer. */
		void ResetOffset() { m_Offset = 0; m_IsFirstUpdate = true; }

		/**
		 * @brief Gets the element count of the buffer.
		 * @return The element count of the buffer.
		 */
		uint32_t GetElementCount() const { return m_ElementCount; }

		/**
		 * @brief Gets the mapped data of the buffer.
		 * @return A pointer to the mapped data.
		 */
		void* GetMappedData() const { return m_MappedData; }

	private:
		VkBuffer m_Buffer{ VK_NULL_HANDLE };		// Vulkan buffer handle
		VmaAllocation m_Allocation{VK_NULL_HANDLE}; // VMA allocation handle for the buffer's memory
		VmaAllocator m_Allocator{VK_NULL_HANDLE};	// VMA allocator handle
		void* m_MappedData{ nullptr };				// Pointer to the mapped data
		VkDeviceAddress m_DeviceAddress{ 0 };		// Device address of the buffer
		uint32_t m_StrideUnaligned		= 0;		// Original stride before alignment, used for calculating buffer size and element count
		uint32_t m_Stride				= 0;		// Aligned stride of each element in the buffer, ensuring proper alignment for GPU access
		uint32_t m_ElementCount			= 0;		// Number of elements in the buffer, calculated based on the total buffer size and the aligned stride
		uint32_t m_Offset				= 0;		// Current offset for dynamic buffer updates, used to track where the next data should be written in the buffer
		bool m_IsFirstUpdate			= true;     // Indicates if the buffer has been updated for the first time, used to determine if the offset should be reset on the first update
		bool m_ExplicitlyMapped			= false;	// Indicates if the buffer is explicitly mapped
		bool m_IsMappable               = false;

	};

}

// -------------------------------------------------------

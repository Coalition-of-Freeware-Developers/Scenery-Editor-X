/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* buffer.h
* -------------------------------------------------------
* Created: 12/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/memory/memory.h>

/// -------------------------------------------------------

namespace SceneryEditorX::Memory
{
	struct Buffer
	{
		void* data = nullptr;
		uint64_t size = 0;

		Buffer() = default;
		Buffer(const void* data, const uint64_t size = 0) : data(const_cast<void *>(data)), size(size) { }

		static Buffer Copy(const Buffer& other)
		{
			Buffer buffer;
			buffer.Allocate(other.size);
			memcpy(buffer.data, other.data, other.size);
			return buffer;
		}

		static Buffer Copy(const void* data, const uint64_t size)
		{
			Buffer buffer;
			buffer.Allocate(size);
			if(size) memcpy(buffer.data, data, size);
			return buffer;
		}

		void Allocate(const uint64_t fsize)
		{
			delete[] static_cast<::byte *>(data);
			data = nullptr;
			size = fsize;

			if (fsize == 0)
				return;

			data = hnew byte[fsize];
		}

		void Release()
		{
			delete[] static_cast<byte *>(data);
			data = nullptr;
			size = 0;
		}

		void ZeroInitialize() const
        {
			if (data)
				memset(data, 0, size);
		}

		template<typename T>
		T& Read(const uint64_t offset = 0)
		{
			return *(T*)((byte*)data + offset);
		}

		template<typename T>
		const T& Read(const uint64_t offset = 0) const
		{
			return *(T*)((byte*)data + offset);
		}

        [[nodiscard]] byte* ReadBytes(uint64_t fsize, uint64_t offset) const
		{
			SEDX_CORE_ASSERT(offset + fsize <= size, "Buffer overflow!");
			byte* buffer = hnew byte[fsize];
			memcpy(buffer, (byte*)data + offset, fsize);
			return buffer;
		}

		void Write(const void* fdata, uint64_t fsize, uint64_t offset = 0) const
        {
			SEDX_CORE_ASSERT(offset + fsize <= size, "Buffer overflow!");
			memcpy((byte*)data + offset, fdata, fsize);
		}

		operator bool() const
		{
			return (bool)data;
		}

		byte& operator[](int index)
		{
			return ((byte*)data)[index];
		}

		byte operator[](int index) const
		{
			return ((byte*)data)[index];
		}

		template<typename T>
		T* As() const
		{
			return (T*)data;
		}

        [[nodiscard]] uint64_t GetSize() const { return size; }
	};

	struct BufferSafe : Buffer
	{
		~BufferSafe()
		{
			Release();
		}

		static BufferSafe Copy(const void* fdata, const uint64_t fsize)
		{
			BufferSafe buffer;
			buffer.Allocate(fsize);
			memcpy(buffer.data, fdata, fsize);
			return buffer;
		}
	};

}

/// -------------------------------------------------------

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
#include <cstdint>
#include <SceneryEditorX/core/memory/memory.h>

#ifndef SEDX_BYTE_DEFINED
#define SEDX_BYTE_DEFINED
using byte = unsigned char;
#endif

/// -------------------------------------------------------

namespace SceneryEditorX::Memory
{
	struct Buffer
	{
		void* data = nullptr;
		uint64_t size = 0;

		Buffer() = default;
        explicit Buffer(const void* data, const uint64_t size = 0) : data(const_cast<void *>(data)), size(size) { }

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
			return *(T*)(static_cast<byte *>(data) + offset);
		}

		template<typename T>
		const T& Read(const uint64_t offset = 0) const
		{
			return *(T*)(static_cast<byte *>(data) + offset);
		}

        [[nodiscard]] byte* ReadBytes(const uint64_t fsize, const uint64_t offset) const
		{
			SEDX_CORE_ASSERT(offset + fsize <= size, "Buffer overflow!");
			byte* buffer = hnew byte[fsize];
			memcpy(buffer, static_cast<byte *>(data) + offset, fsize);
			return buffer;
		}

		void Write(const void* fdata, const uint64_t fsize, const uint64_t offset = 0) const
        {
			SEDX_CORE_ASSERT(offset + fsize <= size, "Buffer overflow!");
			memcpy(static_cast<byte *>(data) + offset, fdata, fsize);
		}

        explicit operator bool() const
		{
			return static_cast<bool>(data);
		}

		byte& operator[](const int index)
		{
			return static_cast<byte *>(data)[index];
		}

		byte operator[](const int index) const
		{
			return static_cast<byte *>(data)[index];
		}

		template<typename T>
		T* As() const
		{
			return (T*)data;
		}

        [[nodiscard]] uint64_t GetSize() const { return size; }
	};

    /// -------------------------------------------------------

	struct BufferSafe : Buffer
	{
		~BufferSafe();

        static BufferSafe Copy(const void* fdata, const uint64_t fsize)
		{
			BufferSafe buffer;
			buffer.Allocate(fsize);
			memcpy(buffer.data, fdata, fsize);
			return buffer;
		}
	};

    /// -------------------------------------------------------

    inline BufferSafe::~BufferSafe() { Release(); }

}

/// -------------------------------------------------------

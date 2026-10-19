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
 * Created: 12/7/2025
 * -------------------------------------------------------
 */
#pragma once
#include "memory.h"
#include <cstdint>
#ifndef SEDX_BYTE_DEFINED
	#define SEDX_BYTE_DEFINED
	typedef unsigned char byte;
#endif

// -------------------------------------------------------

namespace SceneryEditorX::Memory
{
	struct Buffer
	{
		void* data = nullptr;
		uint64_t size = 0;

		Buffer() = default;
	    Buffer(const void *data, uint64_t size = 0) : data((void*)data), size(size) {}

		static Buffer Copy(const Buffer& other)
		{
			Buffer buffer;
			buffer.Allocate(other.size);
			memcpy(buffer.data, other.data, other.size);
			return buffer;
		}

		static Buffer Copy(const void* data, uint64_t size)
		{
			Buffer buffer;
			buffer.Allocate(size);
			if(size) memcpy(buffer.data, data, size);
			return buffer;
		}

		void Allocate(uint64_t fsize)
		{
			delete[] (byte*)(data);
			data = nullptr;
			size = fsize;

			if (fsize == 0)
				return;

			data = new byte[fsize];
		}

		void Release()
		{
            delete[] (byte *)(data);
			data = nullptr;
			size = 0;
		}

		void ZeroInitialize()
        {
			if (data)
				memset(data, 0, size);
		}

		template<typename T>
		T& Read(uint64_t offset = 0)
		{
            return *(T *)((byte *)(data) + offset);
		}

		template<typename T>
		const T& Read(uint64_t offset = 0) const
		{
            return *(T *)((byte *)(data) + offset);
		}

        [[nodiscard]] byte* ReadBytes(uint64_t fsize, uint64_t offset) const
		{
			SEDX_CORE_ASSERT(offset + fsize <= size, "Buffer overflow!");
			byte* buffer = new byte[fsize];
            memcpy(buffer, (byte *)(data) + offset, fsize);
			return buffer;
		}

		void Write(const void* fdata, uint64_t fsize, uint64_t offset = 0)
        {
			SEDX_CORE_ASSERT(offset + fsize <= size, "Buffer overflow!");
            memcpy((byte *)(data) + offset, fdata, fsize);
		}

	    operator bool() const
		{
			return (bool)data;
		}

		byte& operator[](int index)
		{
            return ((byte *)data)[index];
		}

		byte operator[](int index) const
		{
            return ((byte *)data)[index];
		}

		template<typename T>
		T* As() const
		{
			return (T*)data;
		}

        [[nodiscard]] inline uint64_t GetSize() const { return size; }
	};

    // -------------------------------------------------------

	struct BufferSafe : Buffer
	{
		~BufferSafe()
        {
            Release();
        }

        static BufferSafe Copy(const void* fdata, uint64_t fsize)
		{
			BufferSafe buffer;
			buffer.Allocate(fsize);
			memcpy(buffer.data, fdata, fsize);
			return buffer;
		}
	};

    // -------------------------------------------------------

}

// -------------------------------------------------------

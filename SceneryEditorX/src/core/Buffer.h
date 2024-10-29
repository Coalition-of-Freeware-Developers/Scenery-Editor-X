#pragma once

#include "../src//xpeditorpch.h"
#include "../src/core/Assert.h"

namespace SceneryEditorX
{
    struct dataBuffer
    {
        void *Data = nullptr;
        uint64_t Size = 0;
    
        dataBuffer() = default;
    
        dataBuffer(const void *data, uint64_t size = 0) : Data((void *)data), Size(size) { }
    
        static dataBuffer Copy(const dataBuffer&, other)
        {
            dataBuffer buffer;
            buffer.dataAllocation(other.Size);
            memcpy(buffer.Data, other.Data, other.Size);
            return buffer;
        }

        static dataBuffer Copy(const void* data, uint64_t size)
        {
            dataBuffer buffer;
            buffer.dataAllocation(other.Size);
            memcpy(buffer.Data, data, size);
            return buffer;
        }

        void dataAllocation(uint64_t size)
        {
            delete[] (byte*)Data;
            Data = nullptr;
            Size = size;

            if (size == 0)
                return;

            Data = new byte[size];
        }

        void memoryRelease()
        {
            delete[] (byte*)Data;
            Data = nullptr;
            Size = 0;
        }

        void bufferZerorise()
        {
            if (Data)
                memoryZero(Data, 0, Size);
        }

        template <typename T>
        T &Read(uint64_t offset = 0)
        {
            return *(T*)((byte*)Data + offset);
        }

        template <typename T>
        const T& Read(uint64_t offset = 0) const
        {
            return *(T*)((byte *)Data + offset);
        }

        byte* byteReader(uint64_t size, uint64_t offset) const
        {
            SEDX_ASSERT(offset + size <= Size, "Buffer overflow!");
            byte* buffer = new byte[size];
            memcpy(buffer, (byte*)Data + offset, size);
            return buffer;
        }

        void Write(const void* data, uint64_t size, uint64_t offset = 0)
        {
            SEDX_ASSERT(offset + size <= Size, "Buffer overflow!");
            memcpy((byte*)Data + offset, data, size);
        }

        operator bool() const
        {
            return (bool)Data;
        }

        byte& operator[](int index)
        {
            return ((byte*)Data)[index];
        }

        byte operator[](int index) const
        {
            return ((byte*)Data)[index];
        }

        template <typename T>
        T* As() const
        {
            return (T*)Data;
        }

        inline uint64_t GetSize() const
        {
            return Size;
        }
    };
}

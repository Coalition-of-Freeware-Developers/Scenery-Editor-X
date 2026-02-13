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
 * memory_stream.cpp
 * -------------------------------------------------------
 * Created: 12/7/2025
 * -------------------------------------------------------
 */
#include "memory_stream.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	/** MemoryStreamWriter */
	MemoryStreamWriter::MemoryStreamWriter(Memory::Buffer & buffer, size_t size) : m_Buffer(buffer)
	{
		if (size > buffer.size)
			buffer.Allocate((uint32_t)size);
	}

	MemoryStreamWriter::~MemoryStreamWriter() = default;

    bool MemoryStreamWriter::WriteData(const char* data, size_t size)
	{
		if (m_WritePos + size > m_Buffer.size)
			return false;

		m_Buffer.Write(data, (uint32_t)size, (uint32_t)m_WritePos);
		return true;
	}

    // -------------------------------------------------------

	/** MemoryStreamReader */
	MemoryStreamReader::MemoryStreamReader(const Memory::Buffer & buffer) : m_Buffer(buffer) {}
	MemoryStreamReader::~MemoryStreamReader() = default;

    bool MemoryStreamReader::ReadData(char* destination, size_t size)
	{
		if (m_ReadPos + size > m_Buffer.size)
			return false;

		memcpy(destination, (char*)m_Buffer.data + m_ReadPos, size);
		return true;
	}

}

// -------------------------------------------------------

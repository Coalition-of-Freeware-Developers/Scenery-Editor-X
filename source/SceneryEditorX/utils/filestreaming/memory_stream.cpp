/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* memory_stream.cpp
* -------------------------------------------------------
* Created: 12/7/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/utils/filestreaming/memory_stream.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	/** MemoryStreamWriter */
	MemoryStreamWriter::MemoryStreamWriter(Memory::Buffer& buffer, size_t size) : m_Buffer(buffer)
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

    /// -------------------------------------------------------

	/** MemoryStreamReader */
	MemoryStreamReader::MemoryStreamReader(const Memory::Buffer& buffer) : m_Buffer(buffer) {}
	MemoryStreamReader::~MemoryStreamReader() = default;

    bool MemoryStreamReader::ReadData(char* destination, size_t size)
	{
		if (m_ReadPos + size > m_Buffer.size)
			return false;

		memcpy(destination, (char*)m_Buffer.data + m_ReadPos, size);
		return true;
	}

}

/// -------------------------------------------------------

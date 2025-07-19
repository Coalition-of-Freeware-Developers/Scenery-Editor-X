/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* serializer_writer.cpp
* -------------------------------------------------------
* Created: 9/7/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/memory/buffer.h>
#include <SceneryEditorX/serialization/serializer_writer.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	void SerializeWriter::WriteBuffer(Memory::Buffer buffer, const bool writeSize)
	{
		if (writeSize)
			WriteData((char*)&buffer.size, sizeof(uint64_t));

		WriteData((char*)buffer.data, buffer.size);
	}

	void SerializeWriter::WriteZero(const uint64_t size)
	{
        constexpr char zero = 0;
		for (uint64_t i = 0; i < size; i++)
			WriteData(&zero, 1);
	}

	void SerializeWriter::WriteString(const std::string& string)
	{
		size_t size = string.size();
		WriteData((char*)&size, sizeof(size_t));
		WriteData(string.data(), sizeof(char) * string.size());
	}

}

/// -------------------------------------------------------

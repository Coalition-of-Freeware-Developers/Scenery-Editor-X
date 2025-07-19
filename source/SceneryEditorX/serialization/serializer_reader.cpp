/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* serializer_reader.cpp
* -------------------------------------------------------
* Created: 9/7/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/memory/buffer.h>
#include <SceneryEditorX/serialization/serializer_reader.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    void SerializeReader::ReadBuffer(Memory::Buffer& buffer, uint32_t size)
	{
		buffer.size = size;
		if (size == 0)
			ReadData((char*)&buffer.size, sizeof(uint64_t));

		buffer.Allocate(buffer.size);
		ReadData((char*)buffer.data, buffer.size);
	}

	void SerializeReader::ReadString(std::string& string)
	{
		size_t size;
		ReadData((char*)&size, sizeof(size_t));

		string.resize(size);
		ReadData(string.data(), sizeof(char) * size);
	}

}

/// -------------------------------------------------------

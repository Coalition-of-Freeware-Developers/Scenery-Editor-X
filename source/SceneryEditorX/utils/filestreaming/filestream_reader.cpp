/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* filestream_reader.cpp
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/utils/filestreaming/filestream_reader.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	void StreamReader::ReadBuffer(Memory::Buffer& buffer, uint32_t size)
	{
		buffer.size = size;
		if (size == 0)
			ReadData((char*)&buffer.size, sizeof(uint64_t));

		buffer.Allocate(buffer.size);
		ReadData((char*)buffer.data, buffer.size);
	}

	void StreamReader::ReadString(std::string& string)
	{
		size_t size;
		ReadData((char*)&size, sizeof(size_t));

		string.resize(size);
		ReadData(string.data(), sizeof(char) * size);
	}

}

/// -------------------------------------------------------

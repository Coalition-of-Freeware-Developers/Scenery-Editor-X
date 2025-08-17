/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* filestream_writer.cpp
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#include "filestream_writer.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	void StreamWriter::WriteBuffer(Memory::Buffer buffer, const bool writeSize)
	{
		if (writeSize)
			WriteData((char*)&buffer.size, sizeof(uint64_t));

		WriteData((char*)buffer.data, buffer.size);
	}

	void StreamWriter::WriteZero(const uint64_t size)
	{
		char zero = 0;
		for (uint64_t i = 0; i < size; i++)
			WriteData(&zero, 1);
	}

	void StreamWriter::WriteString(const std::string& string)
	{
		size_t size = string.size();
		WriteData((char*)&size, sizeof(size_t));
		WriteData(string.data(), sizeof(char) * string.size());
	}

}

/// -------------------------------------------------------

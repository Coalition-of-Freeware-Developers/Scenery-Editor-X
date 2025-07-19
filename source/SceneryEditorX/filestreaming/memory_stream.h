/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* memory_stream.h
* -------------------------------------------------------
* Created: 12/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/memory/buffer.h>
#include <SceneryEditorX/filestreaming/filestream_reader.h>
#include <SceneryEditorX/filestreaming/filestream_writer.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /// MemoryStreamWriter
	class MemoryStreamWriter : public StreamWriter
	{
	public:
		MemoryStreamWriter(Memory::Buffer& buffer, size_t size);
		MemoryStreamWriter(const MemoryStreamWriter&) = delete;
		~MemoryStreamWriter();

		bool IsStreamGood() const final { return m_WritePos < m_Buffer.size; }
		uint64_t GetStreamPosition() final { return m_WritePos; }
		void SetStreamPosition(uint64_t position) final { m_WritePos = position; }
		bool WriteData(const char* data, size_t size) final;

	private:
        Memory::Buffer& m_Buffer;
		size_t m_WritePos = 0;
	};

    /// -------------------------------------------------------

	/// MemoryStreamReader
	class MemoryStreamReader : public StreamReader
	{
	public:
		MemoryStreamReader(const Memory::Buffer& buffer);
		MemoryStreamReader(const MemoryStreamReader&) = delete;
		~MemoryStreamReader();

		bool IsStreamGood() const final { return m_ReadPos < m_Buffer.size; }
		uint64_t GetStreamPosition() final { return m_ReadPos; }
		void SetStreamPosition(uint64_t position) final { m_ReadPos = position; }
		bool ReadData(char* destination, size_t size) final;

	private:
		const Memory::Buffer& m_Buffer;
		size_t m_ReadPos = 0;
	};

}

/// -------------------------------------------------------

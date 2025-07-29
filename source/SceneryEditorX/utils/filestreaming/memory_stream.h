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
#include <SceneryEditorX/utils/filestreaming/filestream_reader.h>
#include <SceneryEditorX/utils/filestreaming/filestream_writer.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /// MemoryStreamWriter
	class MemoryStreamWriter : public StreamWriter
	{
	public:
		MemoryStreamWriter(Memory::Buffer& buffer, size_t size);
		MemoryStreamWriter(const MemoryStreamWriter&) = delete;
        virtual ~MemoryStreamWriter() override;

        [[nodiscard]] virtual bool IsStreamGood() const override final { return m_WritePos < m_Buffer.size; }
        virtual uint64_t GetStreamPosition() override final { return m_WritePos; }
        virtual void SetStreamPosition(uint64_t position) override final { m_WritePos = position; }
        virtual bool WriteData(const char* data, size_t size) override final;

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
        virtual ~MemoryStreamReader() override;

        [[nodiscard]] virtual bool IsStreamGood() const override final { return m_ReadPos < m_Buffer.size; }
        virtual uint64_t GetStreamPosition() override final { return m_ReadPos; }
        virtual void SetStreamPosition(uint64_t position) override final { m_ReadPos = position; }
        virtual bool ReadData(char* destination, size_t size) override final;

	private:
		const Memory::Buffer& m_Buffer;
		size_t m_ReadPos = 0;
	};

}

/// -------------------------------------------------------

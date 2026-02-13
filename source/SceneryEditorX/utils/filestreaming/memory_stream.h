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
 * memory_stream.h
 * -------------------------------------------------------
 * Created: 12/7/2025
 * -------------------------------------------------------
 */
#pragma once
#include "SceneryEditorX/core/memory/buffer.h"
#include "filestream_reader.h"
#include "filestream_writer.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
    /// MemoryStreamWriter
	class MemoryStreamWriter : public StreamWriter
	{
	public:
		MemoryStreamWriter(Memory::Buffer & buffer, size_t size);
		MemoryStreamWriter(const MemoryStreamWriter&) = delete;
        virtual ~MemoryStreamWriter() override;

        [[nodiscard]] virtual bool IsStreamGood() const override final { return m_WritePos < m_Buffer.size; }
        virtual uint64_t GetStreamPosition() override final { return m_WritePos; }
        virtual void SetStreamPosition(uint64_t position) override final { m_WritePos = position; }
        virtual bool WriteData(const char* data, size_t size) override final;

	private:
        Memory::Buffer & m_Buffer;
		size_t m_WritePos = 0;
	};

    // -------------------------------------------------------

	/// MemoryStreamReader
	class MemoryStreamReader : public StreamReader
	{
	public:
		MemoryStreamReader(const Memory::Buffer & buffer);
		MemoryStreamReader(const MemoryStreamReader&) = delete;
        virtual ~MemoryStreamReader() override;

        [[nodiscard]] virtual bool IsStreamGood() const override final { return m_ReadPos < m_Buffer.size; }
        virtual uint64_t GetStreamPosition() override final { return m_ReadPos; }
        virtual void SetStreamPosition(uint64_t position) override final { m_ReadPos = position; }
        virtual bool ReadData(char* destination, size_t size) override final;

	private:
		const Memory::Buffer & m_Buffer;
		size_t m_ReadPos = 0;
	};

}

// -------------------------------------------------------

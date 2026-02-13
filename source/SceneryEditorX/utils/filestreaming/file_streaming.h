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
 * file_streaming.h
 * -------------------------------------------------------
 * Created: 11/7/2025
 * -------------------------------------------------------
 */
#pragma once
#include "filestream_reader.h"
#include "filestream_writer.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	class FileStreamWriter : public StreamWriter
	{
	public:
        explicit FileStreamWriter(const std::filesystem::path& path);
		FileStreamWriter(const FileStreamWriter&) = delete;
		virtual ~FileStreamWriter() override;

        [[nodiscard]] virtual bool IsStreamGood() const override final { return m_Stream.good(); }
        virtual uint64_t GetStreamPosition() override final { return m_Stream.tellp(); }
        virtual void SetStreamPosition(uint64_t position) override final { m_Stream.seekp(position); }
        virtual bool WriteData(const char* data, size_t size) override final;

	private:
		std::filesystem::path m_Path;
		std::ofstream m_Stream;
	};

	// -------------------------------------------------------

	class FileStreamReader : public StreamReader
	{
	public:
        explicit FileStreamReader(const std::filesystem::path& path);
		FileStreamReader(const FileStreamReader&) = delete;
        virtual ~FileStreamReader() override;

        [[nodiscard]] const std::filesystem::path& GetFilePath() const { return m_Path; }
        [[nodiscard]] virtual bool IsStreamGood() const override final { return m_Stream.good(); }
        virtual uint64_t GetStreamPosition() override { return m_Stream.tellg(); }
        virtual void SetStreamPosition(uint64_t position) override { m_Stream.seekg(position); }
        virtual bool ReadData(char* destination, size_t size) override;

	private:
		std::filesystem::path m_Path;
		std::ifstream m_Stream;
	};

}

// -------------------------------------------------------

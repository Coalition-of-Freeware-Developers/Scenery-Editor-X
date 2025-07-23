/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* file_streaming.h
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/utils/filestreaming/filestream_reader.h>
#include <SceneryEditorX/utils/filestreaming/filestream_writer.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	class FileStreamWriter : public StreamWriter
	{
	public:
		FileStreamWriter(const std::filesystem::path& path);
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

	/// -------------------------------------------------------

	class FileStreamReader : public StreamReader
	{
	public:
		FileStreamReader(const std::filesystem::path& path);
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

/// -------------------------------------------------------

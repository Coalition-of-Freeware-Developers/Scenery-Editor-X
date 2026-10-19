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
 * project_cache.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "project_cache.h"
#include "project_archive.h"
#include "project_io.h"
#include <algorithm>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	namespace fs = std::filesystem;

	// Beside SourceStamp (not in the unnamed namespace) so the archives find it by argument-dependent lookup
	template<typename A> void Visit(A &a, SourceStamp &v)
	{
		a.Field("path", v.path);
		a.Field("size", v.size);
		a.Field("modified", v.modified);
		a.Field("hash", v.contentHash);
	}

	namespace
	{
		constexpr const char *SNAPSHOT_FILE = "snapshot.sex";
		constexpr const char *RECOVERY_FILE = "recovery.sex";
		constexpr const char *SOURCE_FILE = "source.txt";
		constexpr const char *BACKUP_FOLDER = "backups";
		constexpr const char *TAG_STAMP = "STMP";
		constexpr const char *TAG_RECOVERY = "RCVR";

		/* @brief The RCVR chunk. */
		struct RecoveryRecord
		{
			std::string written;
			std::string projectName;
			SourceStamp base;
		};

		template<typename A> void Visit(A &a, RecoveryRecord &v)
		{
			a.Field("written", v.written);
			a.Field("project", v.projectName);
			a.Field("base", v.base);
		}

		template<typename T>
		std::string Encode(const T &value)
		{
			std::string bytes;
			BinaryWriter writer(bytes);
			writer.Write(value);
			return bytes;
		}

		template<typename T>
		bool Decode(const std::string &bytes, T &value)
		{
			BinaryReader reader(bytes.data(), bytes.size());
			reader.Read(value);
			return reader.Ok();
		}

		std::string NormalizePath(const fs::path &path)
		{
			std::error_code ec;
			fs::path absolute = fs::absolute(path, ec);
			if (ec)
				absolute = path;
			const std::u8string generic = absolute.lexically_normal().generic_u8string();
			std::string text(generic.begin(), generic.end());
#if defined(_WIN32)
			// NTFS is case-insensitive: one cache entry per file however its path is spelled
			for (char &c : text)
			{
				if (c >= 'A' && c <= 'Z')
					c = static_cast<char>(c - 'A' + 'a');
			}
#endif
			return text;
		}

		std::string Hex(const uint64_t value)
		{
			constexpr char digits[] = "0123456789abcdef";
			std::string text(16, '0');
			for (int i = 15; i >= 0; --i)
				text[static_cast<size_t>(15 - i)] = digits[(value >> (i * 4)) & 0xF];
			return text;
		}

		std::string CompactTimestamp()
		{
			// 2026-10-18T12:00:00Z -> 20261018T120000Z: sorts in time order and is a valid file name
			std::string text;
			for (const char c : GetUtcTimestamp())
			{
				if (c != '-' && c != ':')
					text.push_back(c);
			}
			return text;
		}

		bool ReadStamp(const std::string &file, SourceStamp &stamp)
		{
			std::string payload;
			return FindSexChunk(file.data(), file.size(), TAG_STAMP, payload) && Decode(payload, stamp);
		}
	}

	bool GetSourceStamp(const fs::path &path, SourceStamp &stamp, const bool hashContent)
	{
		std::error_code ec;
		stamp = SourceStamp{};
		stamp.path = NormalizePath(path);
		if (!fs::is_regular_file(path, ec))
			return false;
		stamp.size = fs::file_size(path, ec);
		if (ec)
			return false;
		const auto time = fs::last_write_time(path, ec);
		if (ec)
			return false;
		stamp.modified = static_cast<int64_t>(time.time_since_epoch().count());
		if (hashContent)
		{
			std::string bytes;
			if (!ReadFileBytes(path, bytes))
				return false;
			stamp.contentHash = HashFnv1a64(bytes);
		}
		return true;
	}

	fs::path ProjectCache::GetEntryFolder(const fs::path &source) const
	{
		return m_Root / Hex(HashFnv1a64(NormalizePath(source)));
	}

	bool ProjectCache::StoreSnapshot(const fs::path &source, const SceneryProject &project, std::string *error, const SexWriteOptions &options) const
	{
		if (!IsEnabled())
			return false;
		SourceStamp stamp;
		if (!GetSourceStamp(source, stamp))
		{
			if (error)
				*error = "No project file to take a snapshot of";
			return false;
		}

		SexWriteOptions write = options;
		write.extraChunks.push_back({ TAG_STAMP, Encode(stamp) });
		const fs::path folder = GetEntryFolder(source);
		WriteFileAtomic(folder / SOURCE_FILE, stamp.path + "\n", nullptr);
		return WriteFileAtomic(folder / SNAPSHOT_FILE, WriteSex(project, write), error);
	}

	bool ProjectCache::LoadSnapshot(const fs::path &source, SceneryProject &out, std::string *error) const
	{
		if (!IsEnabled())
			return false;
		std::string file;
		if (!ReadFileBytes(GetEntryFolder(source) / SNAPSHOT_FILE, file))
			return false;

		SourceStamp cached, current;
		if (!ReadStamp(file, cached) || !GetSourceStamp(source, current) || !cached.SameFile(current))
			return false;	// Stale: the project file changed (edited by hand, a git checkout, ...)

		std::vector<std::string> ignored;
		if (!ParseSex(file.data(), file.size(), out, &ignored, error))
		{
			out = SceneryProject{};
			return false;
		}
		return true;
	}

	bool ProjectCache::WriteRecovery(const fs::path &source, const SceneryProject &project, std::string *error, const SexWriteOptions &options) const
	{
		if (!IsEnabled())
			return false;
		RecoveryRecord record;
		record.written = GetUtcTimestamp();
		record.projectName = project.info.name;
		GetSourceStamp(source, record.base);		// A project never saved has no base: the stamp keeps just the path

		SexWriteOptions write = options;
		write.extraChunks.push_back({ TAG_RECOVERY, Encode(record) });
		const fs::path folder = GetEntryFolder(source);
		WriteFileAtomic(folder / SOURCE_FILE, record.base.path + "\n", nullptr);
		return WriteFileAtomic(folder / RECOVERY_FILE, WriteSex(project, write), error);
	}

	std::optional<RecoveryInfo> ProjectCache::GetRecovery(const fs::path &source) const
	{
		if (!IsEnabled())
			return std::nullopt;
		const fs::path path = GetEntryFolder(source) / RECOVERY_FILE;
		std::string file;
		if (!ReadFileBytes(path, file))
			return std::nullopt;

		std::string payload;
		RecoveryRecord record;
		if (!FindSexChunk(file.data(), file.size(), TAG_RECOVERY, payload) || !Decode(payload, record))
			return std::nullopt;

		RecoveryInfo info;
		info.file = path;
		info.projectName = record.projectName;
		info.written = record.written;
		info.base = record.base;
		SourceStamp current;
		const bool exists = GetSourceStamp(source, current);
		info.sourceChangedSince = record.base.size != 0 && (!exists || !record.base.SameFile(current));
		return info;
	}

	bool ProjectCache::LoadRecovery(const fs::path &source, SceneryProject &out, std::vector<std::string> *warnings, std::string *error) const
	{
		std::string file;
		if (!IsEnabled() || !ReadFileBytes(GetEntryFolder(source) / RECOVERY_FILE, file, error))
			return false;
		// A recovery is a last resort: keep whatever chunks survived
		SexReadOptions options;
		options.allowPartial = true;
		return ParseSex(file.data(), file.size(), out, warnings, error, options);
	}

	void ProjectCache::ClearRecovery(const fs::path &source) const
	{
		if (!IsEnabled())
			return;
		std::error_code ec;
		fs::remove(GetEntryFolder(source) / RECOVERY_FILE, ec);
	}

	bool ProjectCache::StoreBackup(const fs::path &source, const size_t keep, std::string *error) const
	{
		std::error_code ec;
		if (!IsEnabled() || !fs::is_regular_file(source, ec))
			return false;

		const fs::path folder = GetEntryFolder(source) / BACKUP_FOLDER;
		fs::create_directories(folder, ec);
		// "<UTC time>-<n>-<file name>": two saves in the same second still get their own backup
		const std::string stamp = CompactTimestamp();
		const std::u8string name = source.filename().u8string();
		const auto backupPath = [&](const int counter)
		{
			std::string number = std::to_string(counter);
			number.insert(0, number.size() < 3 ? 3 - number.size() : 0, '0');	// Zero padded so names sort in order
			const std::string prefix = stamp + "-" + number + "-";
			return folder / fs::path(std::u8string(prefix.begin(), prefix.end()) + name);
		};
		int counter = 1;
		while (fs::exists(backupPath(counter), ec))
			++counter;
		const fs::path target = backupPath(counter);

		std::string bytes;
		if (!ReadFileBytes(source, bytes, error) || !WriteFileAtomic(target, bytes, error))
			return false;

		std::vector<fs::path> backups = ListBackups(source);
		for (size_t i = keep; i < backups.size(); ++i)
			fs::remove(backups[i], ec);
		return true;
	}

	std::vector<fs::path> ProjectCache::ListBackups(const fs::path &source) const
	{
		std::vector<fs::path> backups;
		std::error_code ec;
		const fs::path folder = GetEntryFolder(source) / BACKUP_FOLDER;
		if (!IsEnabled() || !fs::is_directory(folder, ec))
			return backups;
		for (const fs::directory_entry &entry : fs::directory_iterator(folder, ec))
		{
			if (entry.is_regular_file(ec))
				backups.push_back(entry.path());
		}
		// Names start with a UTC timestamp: reverse name order is newest first
		std::ranges::sort(backups, [](const fs::path &a, const fs::path &b) { return a.filename().u8string() > b.filename().u8string(); });
		return backups;
	}

	void ProjectCache::Remove(const fs::path &source) const
	{
		if (!IsEnabled())
			return;
		std::error_code ec;
		fs::remove_all(GetEntryFolder(source), ec);
	}

	std::vector<RecoveryInfo> ProjectCache::ListRecoveries() const
	{
		std::vector<RecoveryInfo> recoveries;
		std::error_code ec;
		if (!IsEnabled() || !fs::is_directory(m_Root, ec))
			return recoveries;
		for (const fs::directory_entry &entry : fs::directory_iterator(m_Root, ec))
		{
			if (!entry.is_directory(ec) || !fs::exists(entry.path() / RECOVERY_FILE, ec))
				continue;
			std::string sourceText;
			if (!ReadFileBytes(entry.path() / SOURCE_FILE, sourceText))
				continue;
			while (!sourceText.empty() && (sourceText.back() == '\n' || sourceText.back() == '\r'))
				sourceText.pop_back();
			const fs::path source(std::u8string(sourceText.begin(), sourceText.end()));
			if (std::optional<RecoveryInfo> info = GetRecovery(source))
				recoveries.push_back(std::move(*info));
		}
		std::ranges::sort(recoveries, [](const RecoveryInfo &a, const RecoveryInfo &b) { return a.written > b.written; });
		return recoveries;
	}
}

// -------------------------------------------------------

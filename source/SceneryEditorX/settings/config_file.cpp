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
 * config_file.cpp
 * -------------------------------------------------------
 * Created: 24/10/2026
 * -------------------------------------------------------
 */
#include "config_file.h"
#include <atomic>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <libconfig.h++>
#include <limits>
#include <random>
#include <SceneryEditorX/filesystem/vfs.h>
#include <type_traits>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace fs = std::filesystem;
	using libconfig::Config;
	using libconfig::ConfigException;
	using libconfig::ParseException;
	using libconfig::Setting;

	enum class ConfigFile::ValueKind : uint8_t
	{
		Bool,
		Int,
		Int64,
		Float,
		String
	};

	namespace
	{
		std::unique_ptr<Config> MakeConfig()
		{
			auto config = std::make_unique<Config>();
			config->setOptions(Config::OptionAutoConvert | Config::OptionOpenBraceOnSeparateLine | Config::OptionAllowOverrides);
			config->setTabWidth(2);
			return config;
		}

		bool Fail(std::string *error, std::string message)
		{
			if (error)
				*error = std::move(message);
			return false;
		}

		VFS::FileSystem &FileSystemOf(VFS::FileSystem *vfs)
		{
			return vfs ? *vfs : VFS::Get();
		}

		/* @brief "ui.font_size" -> { "ui", "font_size" } (empty parts dropped). */
		std::vector<std::string> SplitPath(const std::string_view path)
		{
			std::vector<std::string> parts;
			size_t start = 0;
			while (start <= path.size())
			{
				size_t end = path.find('.', start);
				if (end == std::string_view::npos)
					end = path.size();
				if (end > start)
					parts.emplace_back(path.substr(start, end - start));
				start = end + 1;
			}
			return parts;
		}

		/* @brief The setting at a path, or null (no exceptions: a missing group is just "not there"). */
		Setting *FindSetting(const Config &config, const std::vector<std::string> &parts)
		{
			Setting *current = &config.getRoot();
			for (const std::string &part : parts)
			{
				if (!current->isGroup() || !current->exists(part))
					return nullptr;
				current = &(*current)[part.c_str()];
			}
			return current;
		}

		Setting::Type TypeOf(const uint8_t kind)
		{
			static constexpr Setting::Type TYPES[] = { Setting::TypeBoolean, Setting::TypeInt, Setting::TypeInt64, Setting::TypeFloat, Setting::TypeString };
			return TYPES[kind];
		}

		/**
		 * @brief True when two numbers come out the same in the file: libconfig writes floats with a fixed number of
		 * decimals, so 0.1f (0.10000000149...) and the 0.1 read back from "0.1" are one value, not a change.
		 */
		bool SameAsWritten(const double a, const double b, const int decimals)
		{
			char x[400];
			char y[400];
			std::snprintf(x, sizeof(x), "%.*f", decimals, a);
			std::snprintf(y, sizeof(y), "%.*f", decimals, b);
			return std::strcmp(x, y) == 0;
		}

		bool EqualsNoCase(const std::string_view a, const std::string_view b)
		{
			if (a.size() != b.size())
				return false;
			for (size_t i = 0; i < a.size(); ++i)
			{
				const auto lower = [](const char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; };
				if (lower(a[i]) != lower(b[i]))
					return false;
			}
			return true;
		}

		template <typename T>
		bool ParseNumber(const std::string_view text, T &value)
		{
			const char *first = text.data();
			const char *last = text.data() + text.size();
			while (first < last && (*first == ' ' || *first == '\t'))
				++first;
			const auto [end, ec] = std::from_chars(first, last, value);
			return ec == std::errc() && end == last;
		}

		/* @brief A number value as T, with int / int64 / float converting into one another (and text that is a number). */
		template <typename T>
		bool ReadNumber(const Setting &setting, T &value)
		{
			switch (setting.getType())
			{
				case Setting::TypeInt:
				case Setting::TypeInt64:
				{
					const long long number = setting;
					if constexpr (std::is_floating_point_v<T>)
						value = static_cast<T>(number);
					else
					{
						if (number < static_cast<long long>(std::numeric_limits<T>::min()) || number > static_cast<long long>(std::numeric_limits<T>::max()))
							return false;
						value = static_cast<T>(number);
					}
					return true;
				}
				case Setting::TypeFloat:
				{
					const double number = setting;
					if constexpr (std::is_floating_point_v<T>)
						value = static_cast<T>(number);
					else
					{
						if (!(number >= static_cast<double>(std::numeric_limits<T>::min()) && number <= static_cast<double>(std::numeric_limits<T>::max())))
							return false;
						value = static_cast<T>(number);
					}
					return true;
				}
				case Setting::TypeString:
					return ParseNumber(setting.c_str(), value);
				default:
					return false;
			}
		}

		std::string ToText(const Setting &setting)
		{
			switch (setting.getType())
			{
				case Setting::TypeBoolean:	return static_cast<bool>(setting) ? "true" : "false";
				case Setting::TypeInt:
				case Setting::TypeInt64:	return std::to_string(static_cast<long long>(setting));
				case Setting::TypeFloat:
				{
					char text[64];
					const auto [end, ec] = std::to_chars(text, text + sizeof(text), static_cast<double>(setting));
					return ec == std::errc() ? std::string(text, end) : std::string();
				}
				case Setting::TypeString:	return setting.c_str();
				default:					return {};
			}
		}

		/* @brief A file in the editor's temp folder nothing else uses (libconfig can only write to a FILE *). */
		fs::path MakeScratchPath()
		{
			static std::atomic<uint64_t> counter{ 0 };
			static const uint64_t seed = []
			{
				std::random_device device;
				return (static_cast<uint64_t>(device()) << 32) ^ device() ^ static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
			}();
			const uint64_t value = seed ^ (++counter * 0x9e3779b97f4a7c15ull);
			char name[48];
			std::snprintf(name, sizeof(name), "config-%016llx.cfg", static_cast<unsigned long long>(value));
			fs::path folder = VFS::GetAppLocation(VFS::AppLocation::Temp);
			if (folder.empty() || !VFS::CreateFolders(folder))
			{
				std::error_code ec;
				folder = fs::temp_directory_path(ec);
			}
			return folder / name;
		}

		std::FILE *OpenScratch(const fs::path &path)
		{
		#if defined(_WIN32)
			std::FILE *file = nullptr;
			return _wfopen_s(&file, path.c_str(), L"w+b") == 0 ? file : nullptr;
		#else
			return std::fopen(path.c_str(), "w+b");
		#endif
		}

		/* @brief The document as libconfig writes it (through a scratch file: its writer takes a FILE * only). */
		bool Render(const Config &config, std::string &text, std::string *error)
		{
			const fs::path scratch = MakeScratchPath();
			std::FILE *file = OpenScratch(scratch);
			if (!file)
				return Fail(error, "Could not create " + VFS::ToUtf8(scratch));

			bool ok = true;
			try
			{
				config.write(file);
			}
			catch (const ConfigException &e)
			{
				ok = Fail(error, std::string("Could not format the settings: ") + e.what());
			}

			if (ok)
			{
				ok = std::fflush(file) == 0 && std::fseek(file, 0, SEEK_END) == 0;
				const long size = ok ? std::ftell(file) : -1;
				ok = ok && size >= 0 && std::fseek(file, 0, SEEK_SET) == 0;
				if (ok)
				{
					text.resize(static_cast<size_t>(size));
					ok = size == 0 || std::fread(text.data(), 1, text.size(), file) == text.size();
				}
				if (!ok)
					Fail(error, "Could not format the settings (" + VFS::ToUtf8(scratch) + " couldn't be read back)");
			}

			std::fclose(file);
			std::error_code ec;
			fs::remove(scratch, ec);
			return ok;
		}
	}

	// -------------------------------------------------------

	ConfigFile::ConfigFile() : m_Config(MakeConfig())
	{
	}

	ConfigFile::ConfigFile(std::string location, VFS::FileSystem *vfs) : m_Config(MakeConfig()), m_Location(std::move(location)), m_Vfs(vfs)
	{
	}

	ConfigFile::~ConfigFile() = default;

	void ConfigFile::SetLocation(std::string location, VFS::FileSystem *vfs)
	{
		std::scoped_lock lock(m_Mutex);
		m_Location = std::move(location);
		m_Vfs = vfs;
	}

	std::string ConfigFile::GetLocation() const
	{
		std::scoped_lock lock(m_Mutex);
		return m_Location;
	}

	bool ConfigFile::IsVirtual() const
	{
		std::scoped_lock lock(m_Mutex);
		std::string mount, relative;
		return VFS::SplitVirtualPath(m_Location, mount, relative);
	}

	fs::path ConfigFile::GetPhysicalPath() const
	{
		std::scoped_lock lock(m_Mutex);
		std::string mount, relative;
		if (!VFS::SplitVirtualPath(m_Location, mount, relative))
			return VFS::ResolveCase(VFS::FromUtf8(m_Location));

		const VFS::FileSystem &vfs = FileSystemOf(m_Vfs);
		if (const std::optional<fs::path> found = vfs.Find(m_Location))
			return *found;
		return vfs.Resolve(m_Location).value_or(fs::path());
	}

	bool ConfigFile::FileExists() const
	{
		std::scoped_lock lock(m_Mutex);
		std::string mount, relative;
		if (VFS::SplitVirtualPath(m_Location, mount, relative))
			return FileSystemOf(m_Vfs).Exists(m_Location) && !FileSystemOf(m_Vfs).IsDirectory(m_Location);
		return !m_Location.empty() && VFS::IsFile(VFS::FromUtf8(m_Location));
	}

	ConfigFile::LoadResult ConfigFile::Load(std::string *error)
	{
		std::scoped_lock lock(m_Mutex);
		m_Config = MakeConfig();
		m_Dirty = false;

		if (m_Location.empty())
		{
			Fail(error, "The settings file has no location");
			return LoadResult::Failed;
		}

		std::string mount, relative;
		const bool isVirtual = VFS::SplitVirtualPath(m_Location, mount, relative);
		const VFS::FileSystem &vfs = FileSystemOf(m_Vfs);
		if (isVirtual && !vfs.IsMounted(mount))
		{
			Fail(error, "Nothing is mounted as \"" + mount + "\" (for " + m_Location + ")");
			return LoadResult::Failed;
		}
		if (isVirtual ? !vfs.Exists(m_Location) : !VFS::IsFile(VFS::FromUtf8(m_Location)))
			return LoadResult::Missing;

		std::string bytes;
		if (!ReadBytes(bytes, error))
			return LoadResult::Failed;

		// Files saved by Windows editors may start with a UTF-8 byte order mark, which libconfig's grammar rejects
		std::string_view text = bytes;
		if (text.starts_with("\xEF\xBB\xBF"))
			text.remove_prefix(3);

		std::unique_ptr<Config> config = MakeConfig();
		try
		{
			config->readString(std::string(text));
		}
		catch (const ParseException &e)
		{
			Fail(error, m_Location + " line " + std::to_string(e.getLine()) + ": " + (e.getError() ? e.getError() : "parse error"));
			return LoadResult::Invalid;
		}
		catch (const ConfigException &e)
		{
			Fail(error, m_Location + ": " + e.what());
			return LoadResult::Invalid;
		}

		m_Config = std::move(config);
		return LoadResult::Loaded;
	}

	bool ConfigFile::Save(std::string *error)
	{
		std::scoped_lock lock(m_Mutex);
		if (m_Location.empty())
			return Fail(error, "The settings file has no location");

		std::string text;
		if (!Render(*m_Config, text, error) || !WriteBytes(text, error))
			return false;
		m_Dirty = false;
		return true;
	}

	bool ConfigFile::Backup(const std::string_view suffix, std::string *error) const
	{
		std::scoped_lock lock(m_Mutex);
		if (m_Location.empty() || suffix.empty())
			return Fail(error, "Nothing to back up");

		const std::string target = m_Location + std::string(suffix);
		std::string mount, relative;
		if (VFS::SplitVirtualPath(m_Location, mount, relative))
			return FileSystemOf(m_Vfs).Copy(m_Location, target, true, error);

		const fs::path source = VFS::ResolveCase(VFS::FromUtf8(m_Location));
		return VFS::CopyRegularFile(source, VFS::FromUtf8(target), true, error);
	}

	bool ConfigFile::Parse(const std::string_view text, std::string *error)
	{
		std::unique_ptr<Config> config = MakeConfig();
		try
		{
			config->readString(std::string(text));
		}
		catch (const ParseException &e)
		{
			return Fail(error, "line " + std::to_string(e.getLine()) + ": " + (e.getError() ? e.getError() : "parse error"));
		}
		catch (const ConfigException &e)
		{
			return Fail(error, e.what());
		}

		std::scoped_lock lock(m_Mutex);
		m_Config = std::move(config);
		m_Dirty = true;
		return true;
	}

	std::string ConfigFile::Serialize() const
	{
		std::scoped_lock lock(m_Mutex);
		std::string text;
		Render(*m_Config, text, nullptr);
		return text;
	}

	void ConfigFile::Clear()
	{
		std::scoped_lock lock(m_Mutex);
		m_Config = MakeConfig();
		m_Dirty = true;
	}

	bool ConfigFile::IsDirty() const
	{
		std::scoped_lock lock(m_Mutex);
		return m_Dirty;
	}

	// -------------------------------------------------------

	bool ConfigFile::HasOption(const std::string_view path) const
	{
		std::scoped_lock lock(m_Mutex);
		const std::vector<std::string> parts = SplitPath(path);
		const Setting *setting = parts.empty() ? nullptr : FindSetting(*m_Config, parts);
		return setting && setting->isScalar();
	}

	bool ConfigFile::HasGroup(const std::string_view path) const
	{
		std::scoped_lock lock(m_Mutex);
		const Setting *setting = FindSetting(*m_Config, SplitPath(path));
		return setting && setting->isGroup();
	}

	bool ConfigFile::RemoveOption(const std::string_view path)
	{
		std::vector<std::string> parts = SplitPath(path);
		if (parts.empty())
			return false;
		const std::string name = std::move(parts.back());
		parts.pop_back();

		std::scoped_lock lock(m_Mutex);
		Setting *parent = FindSetting(*m_Config, parts);
		if (!parent || !parent->isGroup() || !parent->exists(name))
			return false;
		try
		{
			parent->remove(name);
		}
		catch (const ConfigException &)
		{
			return false;
		}
		m_Dirty = true;
		return true;
	}

	template <typename T>
	bool ConfigFile::Store(const std::string_view path, const ValueKind kind, const T &value)
	{
		const std::vector<std::string> parts = SplitPath(path);
		if (parts.empty())
			return false;

		std::scoped_lock lock(m_Mutex);
		try
		{
			// The groups on the way: created when missing, never made out of a value that's in the way
			Setting *group = &m_Config->getRoot();
			for (size_t i = 0; i + 1 < parts.size(); ++i)
			{
				if (!group->exists(parts[i]))
				{
					group = &group->add(parts[i], Setting::TypeGroup);
					m_Dirty = true;
					continue;
				}
				Setting &child = (*group)[parts[i].c_str()];
				if (!child.isGroup())
					return false;
				group = &child;
			}

			const std::string &name = parts.back();
			const Setting::Type type = TypeOf(static_cast<uint8_t>(kind));
			if (group->exists(name))
			{
				Setting &existing = (*group)[name.c_str()];
				if (!existing.isScalar())
					return false;	// A group or a list is there: settings never replace one with a value
				if (existing.getType() == type)
				{
					bool same = false;
					if constexpr (std::is_same_v<T, bool>)
						same = static_cast<bool>(existing) == value;
					else if constexpr (std::is_same_v<T, int>)
						same = static_cast<int>(existing) == value;
					else if constexpr (std::is_same_v<T, long long>)
						same = static_cast<long long>(existing) == value;
					else if constexpr (std::is_same_v<T, double>)
						same = SameAsWritten(static_cast<double>(existing), value, m_Config->getFloatPrecision());
					else
						same = value == existing.c_str();
					if (!same)
					{
						existing = value;
						m_Dirty = true;
					}
					return true;
				}
				group->remove(name);	// Another type (an older version wrote it differently): replaced
			}

			group->add(name, type) = value;
			m_Dirty = true;
			return true;
		}
		catch (const ConfigException &)
		{
			return false;	// A name libconfig doesn't allow ("2d", "a b" ...)
		}
	}

	bool ConfigFile::AddBoolOption(const std::string_view path, const bool value)
	{
		return Store(path, ValueKind::Bool, value);
	}

	bool ConfigFile::AddIntOption(const std::string_view path, const int value)
	{
		return Store(path, ValueKind::Int, value);
	}

	bool ConfigFile::AddInt64Option(const std::string_view path, const int64_t value)
	{
		return Store(path, ValueKind::Int64, static_cast<long long>(value));
	}

	bool ConfigFile::AddFloatOption(const std::string_view path, const double value)
	{
		return Store(path, ValueKind::Float, value);
	}

	bool ConfigFile::AddStringOption(const std::string_view path, const std::string_view value)
	{
		return Store(path, ValueKind::String, std::string(value));
	}

	bool ConfigFile::GetBoolOption(const std::string_view path, const bool defaultValue) const
	{
		std::scoped_lock lock(m_Mutex);
		const Setting *setting = FindSetting(*m_Config, SplitPath(path));
		if (!setting)
			return defaultValue;
		switch (setting->getType())
		{
			case Setting::TypeBoolean:
				return static_cast<bool>(*setting);
			case Setting::TypeString:
			{
				// SetOption() stores text: "true" / "false" (and 1 / 0, yes / no) read back as booleans
				const std::string_view text = setting->c_str();
				if (EqualsNoCase(text, "true") || EqualsNoCase(text, "yes") || text == "1")
					return true;
				if (EqualsNoCase(text, "false") || EqualsNoCase(text, "no") || text == "0")
					return false;
				return defaultValue;
			}
			default:
				return defaultValue;
		}
	}

	int ConfigFile::GetIntOption(const std::string_view path, const int defaultValue) const
	{
		std::scoped_lock lock(m_Mutex);
		const Setting *setting = FindSetting(*m_Config, SplitPath(path));
		int value = defaultValue;
		return setting && ReadNumber(*setting, value) ? value : defaultValue;
	}

	int64_t ConfigFile::GetInt64Option(const std::string_view path, const int64_t defaultValue) const
	{
		std::scoped_lock lock(m_Mutex);
		const Setting *setting = FindSetting(*m_Config, SplitPath(path));
		long long value = defaultValue;
		return setting && ReadNumber(*setting, value) ? static_cast<int64_t>(value) : defaultValue;
	}

	double ConfigFile::GetFloatOption(const std::string_view path, const double defaultValue) const
	{
		std::scoped_lock lock(m_Mutex);
		const Setting *setting = FindSetting(*m_Config, SplitPath(path));
		double value = defaultValue;
		return setting && ReadNumber(*setting, value) ? value : defaultValue;
	}

	std::string ConfigFile::GetStringOption(const std::string_view path, const std::string_view defaultValue) const
	{
		std::scoped_lock lock(m_Mutex);
		const Setting *setting = FindSetting(*m_Config, SplitPath(path));
		if (setting && setting->getType() == Setting::TypeString)
			return setting->c_str();
		return std::string(defaultValue);
	}

	bool ConfigFile::SetOption(const std::string_view key, const std::string_view value)
	{
		return AddStringOption(key, value);
	}

	bool ConfigFile::GetOption(const std::string_view key, std::string &value) const
	{
		std::scoped_lock lock(m_Mutex);
		const std::vector<std::string> parts = SplitPath(key);
		const Setting *setting = parts.empty() ? nullptr : FindSetting(*m_Config, parts);
		if (!setting || !setting->isScalar())
			return false;
		value = ToText(*setting);
		return true;
	}

	std::vector<std::string> ConfigFile::GetChildNames(const std::string_view groupPath) const
	{
		std::scoped_lock lock(m_Mutex);
		std::vector<std::string> names;
		const Setting *group = FindSetting(*m_Config, SplitPath(groupPath));
		if (!group || !group->isGroup())
			return names;
		for (int i = 0; i < group->getLength(); ++i)
		{
			if (const char *name = (*group)[i].getName())
				names.emplace_back(name);
		}
		return names;
	}

	std::vector<std::string> ConfigFile::GetOptionPaths() const
	{
		std::scoped_lock lock(m_Mutex);
		std::vector<std::string> paths;
		const std::function<void(const Setting &, const std::string &)> visit = [&](const Setting &group, const std::string &prefix)
		{
			for (int i = 0; i < group.getLength(); ++i)
			{
				const Setting &child = group[i];
				const char *name = child.getName();
				if (!name)
					continue;
				const std::string path = prefix.empty() ? std::string(name) : prefix + "." + name;
				if (child.isGroup())
					visit(child, path);
				else if (child.isScalar())
					paths.push_back(path);
			}
		};
		visit(m_Config->getRoot(), {});
		return paths;
	}

	// -------------------------------------------------------

	bool ConfigFile::ReadBytes(std::string &bytes, std::string *error) const
	{
		std::string mount, relative;
		if (VFS::SplitVirtualPath(m_Location, mount, relative))
			return FileSystemOf(m_Vfs).Read(m_Location, bytes, error);
		return VFS::ReadFile(VFS::FromUtf8(m_Location), bytes, error);
	}

	bool ConfigFile::WriteBytes(const std::string_view bytes, std::string *error) const
	{
		std::string mount, relative;
		if (VFS::SplitVirtualPath(m_Location, mount, relative))
			return FileSystemOf(m_Vfs).Write(m_Location, bytes, error);
		return VFS::WriteFileAtomic(VFS::FromUtf8(m_Location), bytes, error);
	}

}

// -------------------------------------------------------

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
 * xp_library.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "xp_library.h"
#include "xp_text.h"
#include <SceneryEditorX/filesystem/vfs.h>
#include <algorithm>
#include <cmath>
#include <system_error>
#include <unordered_set>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		constexpr size_t MAX_WARNINGS = 32;

		struct TypeInfo
		{
			AssetType type;
			const char *extension;
			const char *name;
		};

		constexpr TypeInfo TYPE_INFO[] =
		{
			{ AssetType::Object,		".obj", "Object" },
			{ AssetType::Facade,		".fac", "Facade" },
			{ AssetType::Forest,		".for", "Forest" },
			{ AssetType::Line,			".lin", "Line" },
			{ AssetType::Polygon,		".pol", "Polygon" },
			{ AssetType::String,		".str", "Object String" },
			{ AssetType::AutogenPoint,	".agp", "Autogen Point" },
			{ AssetType::AutogenBlock,	".agb", "Autogen Block" },
			{ AssetType::AutogenString,	".ags", "Autogen String" },
			{ AssetType::Network,		".net", "Road Network" },
			{ AssetType::Terrain,		".ter", "Terrain" },
			{ AssetType::Beach,			".bch", "Beach" },
			{ AssetType::Decal,			".dcl", "Decal" },
		};

		uint8_t ParseSeasons(const std::string_view text, bool &ok)
		{
			ok = true;
			uint8_t bits = 0;
			size_t start = 0;
			while (start <= text.size())
			{
				const size_t comma = text.find(',', start);
				const std::string_view word = text.substr(start, comma == std::string_view::npos ? std::string_view::npos : comma - start);
				if (word == "spr")
					bits |= SeasonSpring;
				else if (word == "sum")
					bits |= SeasonSummer;
				else if (word == "fal")
					bits |= SeasonFall;
				else if (word == "win")
					bits |= SeasonWinter;
				else
					ok = false;

				if (comma == std::string_view::npos)
					break;
				start = comma + 1;
			}
			return bits;
		}

		bool ParseHeader(LineReader &reader, const std::string_view keyword, uint32_t &version, std::string *error)
		{
			std::string_view line;
			std::vector<std::string_view> tokens;
			std::string_view header[3];
			for (std::string_view &entry : header)
			{
				do
				{
					if (!reader.Next(line))
					{
						if (error)
							*error = "File is too short to contain a header";
						return false;
					}
					Tokenize(line, tokens);
				}
				while (tokens.empty());
				entry = tokens[0];
			}

			std::string_view origin = header[0];
			if (origin.size() >= 3 && static_cast<unsigned char>(origin[0]) == 0xEF) // UTF-8 BOM
				origin.remove_prefix(3);

			if ((origin != "A" && origin != "I") || header[2] != keyword)
			{
				if (error)
					*error = "Not an X-Plane " + std::string(keyword) + " file";
				return false;
			}

			version = static_cast<uint32_t>(std::max(0, ParseInt(header[1], 0)));
			return true;
		}
	}

	// -------------------------------------------------------

	AssetType GetAssetType(const std::string_view path)
	{
		const size_t dot = path.rfind('.');
		if (dot == std::string_view::npos)
			return AssetType::Other;

		const std::string extension = ToLower(path.substr(dot));
		for (const TypeInfo &info : TYPE_INFO)
		{
			if (extension == info.extension)
				return info.type;
		}
		return AssetType::Other;
	}

	const char *GetAssetTypeName(const AssetType type)
	{
		for (const TypeInfo &info : TYPE_INFO)
		{
			if (info.type == type)
				return info.name;
		}
		return "Other";
	}

	const char *GetAssetTypeExtension(const AssetType type)
	{
		for (const TypeInfo &info : TYPE_INFO)
		{
			if (info.type == type)
				return info.extension;
		}
		return "";
	}

	const char *GetExportKindName(const ExportKind kind)
	{
		switch (kind)
		{
			case ExportKind::Export:	return "EXPORT";
			case ExportKind::Extend:	return "EXPORT_EXTEND";
			case ExportKind::Exclude:	return "EXPORT_EXCLUDE";
			case ExportKind::Backup:	return "EXPORT_BACKUP";
		}
		return "EXPORT";
	}

	std::string FormatSeasons(const uint8_t seasons)
	{
		if (seasons == 0 || seasons == SeasonAll)
			return "all year";

		std::string text;
		const auto add = [&text](const char *name)
		{
			if (!text.empty())
				text += ',';
			text += name;
		};
		if (seasons & SeasonSpring) add("spr");
		if (seasons & SeasonSummer) add("sum");
		if (seasons & SeasonFall)   add("fal");
		if (seasons & SeasonWinter) add("win");
		return text;
	}

	const char *GetPackageSourceName(const PackageSource source)
	{
		switch (source)
		{
			case PackageSource::CustomScenery:	return "Custom Scenery";
			case PackageSource::DefaultScenery:	return "Default Scenery";
			case PackageSource::GlobalScenery:	return "Global Scenery";
			case PackageSource::Tracked:		return "Tracked Folder";
		}
		return "";
	}

	bool LibraryRegion::Contains(const double latitude, const double longitude) const
	{
		if (all || !bitmap.empty())
			return true;

		const int tileLat = static_cast<int>(std::floor(latitude));
		const int tileLon = static_cast<int>(std::floor(longitude));
		return std::ranges::any_of(rects, [&](const std::array<int, 4> &r)
		{
			return tileLon >= r[0] && tileLat >= r[1] && tileLon <= r[2] && tileLat <= r[3];
		});
	}

	// -------------------------------------------------------

	bool ParseLibrary(const std::string_view text, LibraryFile &out, std::string *error)
	{
		out = LibraryFile{};
		LineReader reader(text);
		if (!ParseHeader(reader, "LIBRARY", out.version, error))
			return false;

		std::string currentRegion;
		int definingRegion = -1;
		bool isPrivate = false;
		bool deprecated = false;
		bool semiDeprecated = false;
		uint32_t publicDate = 0;

		const auto warn = [&out, &reader](const std::string &message)
		{
			if (out.warnings.size() < MAX_WARNINGS)
				out.warnings.push_back("line " + std::to_string(reader.GetLineNumber()) + ": " + message);
		};

		const auto add_export = [&](const ExportKind kind, const float ratio, const uint8_t seasons, const std::string_view virtualPath, const std::string_view realPath)
		{
			if (virtualPath.empty() || realPath.empty())
			{
				warn("Export without a virtual or real path");
				return;
			}

			LibraryExport entry;
			entry.virtualPath = std::string(virtualPath);
			entry.realPath = std::string(realPath);
			entry.kind = kind;
			entry.ratio = ratio;
			entry.seasons = seasons;
			entry.region = currentRegion;
			entry.isPrivate = isPrivate;
			entry.deprecated = deprecated;
			entry.semiDeprecated = semiDeprecated;
			entry.publicDate = publicDate;
			entry.line = reader.GetLineNumber();
			out.exports.push_back(std::move(entry));
		};

		std::string_view line;
		std::vector<std::string_view> t;
		while (reader.Next(line))
		{
			Tokenize(line, t);
			if (t.empty() || t[0].front() == '#')
				continue;

			const std::string_view cmd = t[0];

			// EXPORT <vpath> <real path...>, and the EXTEND / EXCLUDE / BACKUP variants
			if (cmd == "EXPORT" || cmd == "EXPORT_EXTEND" || cmd == "EXPORT_EXCLUDE" || cmd == "EXPORT_BACKUP")
			{
				const ExportKind kind = cmd == "EXPORT" ? ExportKind::Export : cmd == "EXPORT_EXTEND" ? ExportKind::Extend
									  : cmd == "EXPORT_EXCLUDE" ? ExportKind::Exclude : ExportKind::Backup;
				add_export(kind, 1.0f, 0, t.size() > 1 ? t[1] : std::string_view(), RestOfLine(line, 2));
				continue;
			}

			// EXPORT_RATIO <quantity> <vpath> <real path...>
			if (cmd == "EXPORT_RATIO")
			{
				add_export(ExportKind::Export, t.size() > 1 ? ParseFloat(t[1], 1.0f) : 1.0f, 0, t.size() > 2 ? t[2] : std::string_view(), RestOfLine(line, 3));
				continue;
			}

			// EXPORT_SEASON <seasons> <vpath> <real path...> (and _EXTEND_ / _EXCLUDE_), EXPORT_RATIO_SEASON <seasons> <quantity> ...
			if (cmd == "EXPORT_SEASON" || cmd == "EXPORT_EXTEND_SEASON" || cmd == "EXPORT_EXCLUDE_SEASON" || cmd == "EXPORT_RATIO_SEASON")
			{
				bool seasonsOk = true;
				const uint8_t seasons = t.size() > 1 ? ParseSeasons(t[1], seasonsOk) : 0;
				if (!seasonsOk)
					warn("Unknown season in '" + std::string(t.size() > 1 ? t[1] : "") + "' (use spr, sum, fal, win)");

				if (cmd == "EXPORT_RATIO_SEASON")
				{
					add_export(ExportKind::Export, t.size() > 2 ? ParseFloat(t[2], 1.0f) : 1.0f, seasons, t.size() > 3 ? t[3] : std::string_view(), RestOfLine(line, 4));
				}
				else
				{
					const ExportKind kind = cmd == "EXPORT_SEASON" ? ExportKind::Export : cmd == "EXPORT_EXTEND_SEASON" ? ExportKind::Extend : ExportKind::Exclude;
					add_export(kind, 1.0f, seasons, t.size() > 2 ? t[2] : std::string_view(), RestOfLine(line, 3));
				}
				continue;
			}

			if (cmd == "REGION_DEFINE")
			{
				LibraryRegion region;
				region.name = t.size() > 1 ? std::string(t[1]) : std::string();
				out.regions.push_back(std::move(region));
				definingRegion = static_cast<int>(out.regions.size()) - 1;
				continue;
			}

			if (cmd == "REGION_ALL" || cmd == "REGION_RECT" || cmd == "REGION_BITMAP" || cmd == "REGION_DREF")
			{
				if (definingRegion < 0)
				{
					warn(std::string(cmd) + " before REGION_DEFINE");
					continue;
				}

				LibraryRegion &region = out.regions[definingRegion];
				if (cmd == "REGION_ALL")
				{
					region.all = true;
				}
				else if (cmd == "REGION_RECT")
				{
					if (t.size() < 5)
					{
						warn("REGION_RECT needs west south east north");
						continue;
					}
					region.rects.push_back({ ParseInt(t[1]), ParseInt(t[2]), ParseInt(t[3]), ParseInt(t[4]) });
				}
				else if (cmd == "REGION_BITMAP")
				{
					region.bitmap = std::string(RestOfLine(line, 1));
					region.rects.clear();
				}
				else
				{
					if (t.size() < 4)
					{
						warn("REGION_DREF needs dataref comparison value");
						continue;
					}
					region.conditions.push_back({ std::string(t[1]), std::string(t[2]), std::string(t[3]) });
				}
				continue;
			}

			if (cmd == "REGION")
			{
				currentRegion = t.size() > 1 ? std::string(t[1]) : std::string();
				continue;
			}

			if (cmd == "PUBLIC")
			{
				isPrivate = false;
				deprecated = false;
				semiDeprecated = false;
				publicDate = t.size() > 1 ? static_cast<uint32_t>(std::max(0, ParseInt(t[1], 0))) : 0;
				continue;
			}

			if (cmd == "PRIVATE")			{ isPrivate = true; continue; }
			if (cmd == "DEPRECATED")		{ deprecated = true; continue; }
			if (cmd == "SEMI_DEPRECATED")	{ semiDeprecated = true; continue; }

			warn("Unknown command '" + std::string(cmd) + "'");
		}

		return true;
	}

	bool LoadLibraryFile(const std::filesystem::path &path, LibraryFile &out, std::string *error)
	{
		std::string text;
		if (!ReadTextFile(path, text))
		{
			if (error)
				*error = "Could not read '" + path.string() + "'";
			return false;
		}
		return ParseLibrary(text, out, error);
	}

	// -------------------------------------------------------

	bool ParseSceneryPacks(const std::string_view text, const std::filesystem::path &xplaneRoot, std::vector<SceneryPackEntry> &out, std::string *error)
	{
		out.clear();
		LineReader reader(text);
		uint32_t version = 0;
		if (!ParseHeader(reader, "SCENERY", version, error))
		{
			// The header's second line is "1000 Version"; ParseHeader only reads its first word.
			return false;
		}

		std::string_view line;
		std::vector<std::string_view> t;
		while (reader.Next(line))
		{
			Tokenize(line, t);
			if (t.empty() || t[0].front() == '#')
				continue;

			const bool enabled = t[0] == "SCENERY_PACK";
			if (!enabled && t[0] != "SCENERY_PACK_DISABLED")
				continue;

			SceneryPackEntry entry;
			entry.enabled = enabled;
			entry.rawPath = std::string(RestOfLine(line, 1));
			if (entry.rawPath.empty())
				continue;

			if (entry.rawPath == "*GLOBAL_AIRPORTS*")
			{
				entry.isGlobalAirports = true;
				entry.path = xplaneRoot / "Global Scenery" / "Global Airports";
			}
			else
			{
				std::string trimmed = entry.rawPath;
				while (!trimmed.empty() && (trimmed.back() == '/' || trimmed.back() == '\\'))
					trimmed.pop_back();

				std::filesystem::path path = std::filesystem::path(trimmed).make_preferred();
				entry.path = path.is_absolute() ? path : (xplaneRoot / path);
			}

			out.push_back(std::move(entry));
		}

		return true;
	}

	// -------------------------------------------------------

	bool LibraryAsset::IsPrivate() const
	{
		return !providers.empty() && std::ranges::all_of(providers, [](const AssetProvider &p) { return p.isPrivate; });
	}

	bool LibraryAsset::IsDeprecated() const
	{
		return !providers.empty() && std::ranges::all_of(providers, [](const AssetProvider &p) { return p.deprecated || p.semiDeprecated; });
	}

	void LibraryIndex::Build(std::vector<LibraryPackage> packages, const BuildOptions &options)
	{
		m_Packages = std::move(packages);
		m_Assets.clear();
		m_Regions.clear();
		m_Lookup.clear();
		m_ExportCount = 0;
		m_MissingFileCount = 0;

		std::unordered_map<std::string, LibraryAsset> assets;
		std::unordered_map<std::string, std::optional<std::filesystem::path>> existsCache;	// Export file -> as the disk spells it
		std::unordered_set<std::string> regionNames;

		for (uint32_t packageIndex = 0; packageIndex < static_cast<uint32_t>(m_Packages.size()); ++packageIndex)
		{
			LibraryPackage &package = m_Packages[packageIndex];
			package.exportCount = 0;
			package.assetCount = 0;
			package.missingFileCount = 0;
			package.typeCounts.fill(0);
			package.warnings.clear();
			package.error.clear();

			if (!package.tracked || !package.hasLibrary)
				continue;

			LibraryFile file;
			std::string error;
			if (!LoadLibraryFile(package.path / "library.txt", file, &error))
			{
				package.error = error;
				continue;
			}

			package.warnings = std::move(file.warnings);
			for (LibraryRegion &region : file.regions)
			{
				if (regionNames.insert(region.name).second)
					m_Regions.push_back(std::move(region));
			}

			std::unordered_set<std::string> packageAssets;
			for (LibraryExport &entry : file.exports)
			{
				AssetProvider provider;
				provider.package = packageIndex;
				provider.kind = entry.kind;
				provider.ratio = entry.ratio;
				provider.seasons = entry.seasons;
				provider.region = std::move(entry.region);
				provider.isPrivate = entry.isPrivate;
				provider.deprecated = entry.deprecated;
				provider.semiDeprecated = entry.semiDeprecated;
				provider.realPath = (package.path / ToNativeRelativePath(entry.realPath)).lexically_normal();

				if (options.checkFilesExist)
				{
					const std::string key = provider.realPath.string();
					auto [it, inserted] = existsCache.try_emplace(key);
					if (inserted)
						it->second = VFS::FindCaseInsensitive(provider.realPath);
					provider.fileExists = it->second.has_value();
					if (it->second)
						provider.realPath = *it->second;
				}

				++package.exportCount;
				++m_ExportCount;
				if (!provider.fileExists)
				{
					++package.missingFileCount;
					++m_MissingFileCount;
				}

				if (packageAssets.insert(entry.virtualPath).second)
					++package.typeCounts[static_cast<size_t>(GetAssetType(entry.virtualPath))];

				auto [it, inserted] = assets.try_emplace(entry.virtualPath);
				if (inserted)
				{
					it->second.virtualPath = entry.virtualPath;
					it->second.type = GetAssetType(entry.virtualPath);
				}
				it->second.providers.push_back(std::move(provider));
			}

			package.assetCount = static_cast<uint32_t>(packageAssets.size());
		}

		m_Assets.reserve(assets.size());
		for (auto &[path, asset] : assets)
		{
			std::ranges::stable_sort(asset.providers, [this](const AssetProvider &a, const AssetProvider &b)
			{
				return m_Packages[a.package].priority < m_Packages[b.package].priority;
			});

			// The highest-priority package that really exports the path wins; EXPORT_BACKUP only fills gaps.
			asset.resolved = -1;
			for (size_t i = 0; i < asset.providers.size(); ++i)
			{
				if (asset.providers[i].kind != ExportKind::Backup)
				{
					asset.resolved = static_cast<int>(i);
					break;
				}
			}
			if (asset.resolved < 0 && !asset.providers.empty())
				asset.resolved = 0;

			m_Assets.push_back(std::move(asset));
		}

		std::ranges::sort(m_Assets, [](const LibraryAsset &a, const LibraryAsset &b) { return a.virtualPath < b.virtualPath; });

		m_Lookup.reserve(m_Assets.size());
		for (uint32_t i = 0; i < static_cast<uint32_t>(m_Assets.size()); ++i)
			m_Lookup.emplace(m_Assets[i].virtualPath, i);
	}

	const LibraryAsset *LibraryIndex::Find(const std::string_view virtualPath) const
	{
		const auto it = m_Lookup.find(std::string(virtualPath));
		return it != m_Lookup.end() ? &m_Assets[it->second] : nullptr;
	}

	std::filesystem::path LibraryIndex::Resolve(const std::string_view virtualPath) const
	{
		const LibraryAsset *asset = Find(virtualPath);
		const AssetProvider *provider = asset ? asset->GetResolved() : nullptr;
		return provider ? VFS::ResolveCase(provider->realPath) : std::filesystem::path();
	}

}

// -------------------------------------------------------

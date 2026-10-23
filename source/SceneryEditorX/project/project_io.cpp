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
 * project_io.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "project_io.h"
#include "project_edx.h"
#include <SceneryEditorX/filesystem/vfs.h>
#include <algorithm>
#include <fstream>
#include <functional>
#include <map>
#include <set>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	namespace fs = std::filesystem;

	namespace
	{
		std::string ToLowerAscii(std::string text)
		{
			for (char &c : text)
			{
				if (c >= 'A' && c <= 'Z')
					c = static_cast<char>(c - 'A' + 'a');
			}
			return text;
		}

		bool EndsWith(const std::string &text, const std::string_view suffix)
		{
			return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
		}

		using VFS::ToUtf8;
	}

	const char *GetProjectFormatName(const ProjectFormat format)
	{
		switch (format)
		{
			case ProjectFormat::Edx:		return "Scenery Editor X project (.edX)";
			case ProjectFormat::Sex:		return "Scenery Editor X binary project (.sex)";
			case ProjectFormat::Wed:		return "WorldEditor project (earth.wed.xml)";
			case ProjectFormat::Unknown:	break;
		}
		return "Unknown";
	}

	ProjectFormat GetFormatForPath(const fs::path &path)
	{
		const std::string name = ToLowerAscii(ToUtf8(path.filename()));
		if (EndsWith(name, ".edx"))
			return ProjectFormat::Edx;
		if (EndsWith(name, ".sex"))
			return ProjectFormat::Sex;
		if (EndsWith(name, ".wed.xml") || name == WED_FILE_NAME)
			return ProjectFormat::Wed;
		return ProjectFormat::Unknown;
	}

	ProjectFormat SniffFormat(const std::string_view bytes)
	{
		if (IsSexData(bytes.data(), bytes.size()))
			return ProjectFormat::Sex;
		size_t i = 0;
		if (bytes.size() >= 3 && static_cast<uint8_t>(bytes[0]) == 0xEF && static_cast<uint8_t>(bytes[1]) == 0xBB && static_cast<uint8_t>(bytes[2]) == 0xBF)
			i = 3;
		while (i < bytes.size() && (bytes[i] == ' ' || bytes[i] == '\t' || bytes[i] == '\r' || bytes[i] == '\n'))
			++i;
		if (i < bytes.size() && bytes[i] == '{')
			return ProjectFormat::Edx;
		if (i < bytes.size() && bytes[i] == '<' && bytes.find("<doc", i) != std::string_view::npos)
			return ProjectFormat::Wed;
		return ProjectFormat::Unknown;
	}

	bool ReadFileBytes(const fs::path &path, std::string &bytes, std::string *error)
	{
		return VFS::ReadFile(path, bytes, error);
	}

	bool WriteFileAtomic(const fs::path &path, const std::string_view bytes, std::string *error)
	{
		return VFS::WriteFileAtomic(path, bytes, error);
	}

	bool ParseProject(const std::string_view bytes, const ProjectFormat format, SceneryProject &out, std::vector<std::string> *warnings, std::string *error,
		const LoadOptions &options)
	{
		switch (format)
		{
			case ProjectFormat::Edx:	return ParseEdx(bytes, out, warnings, error);
			case ProjectFormat::Sex:	return ParseSex(bytes.data(), bytes.size(), out, warnings, error, options.sex);
			case ProjectFormat::Wed:	return ParseWed(bytes, out, warnings, error, options.wed);
			case ProjectFormat::Unknown: break;
		}
		if (error)
			*error = "Not a project file Scenery Editor X knows (.edX, .sex or earth.wed.xml)";
		return false;
	}

	bool LoadProjectFile(const fs::path &path, SceneryProject &out, ProjectFormat *format, std::vector<std::string> *warnings, std::string *error,
		const LoadOptions &options)
	{
		out = SceneryProject{};
		std::string bytes;
		if (!ReadFileBytes(path, bytes, error))
			return false;

		// The content decides when the name and the bytes disagree (a .edX that is really binary, ...)
		ProjectFormat detected = SniffFormat(bytes);
		const ProjectFormat named = GetFormatForPath(path);
		if (detected == ProjectFormat::Unknown)
			detected = named;
		else if (named != ProjectFormat::Unknown && named != detected && warnings)
			warnings->push_back(ToUtf8(path.filename()) + " holds a " + GetProjectFormatName(detected) + "; it was read as one");
		if (format)
			*format = detected;

		std::string reason;
		if (!ParseProject(bytes, detected, out, warnings, &reason, options))
		{
			if (error)
				*error = ToUtf8(path.filename()) + ": " + reason;
			return false;
		}

		if (detected == ProjectFormat::Wed)
		{
			// earth.wed.xml carries no name: the pack folder is it
			const std::string pack = ToUtf8(fs::absolute(path).parent_path().filename());
			out.info.name = pack.empty() ? "WorldEditor Project" : pack;
			out.info.packageName = out.info.name;
		}
		else if (out.info.name.empty())
			out.info.name = ToUtf8(path.stem());
		return true;
	}

	std::string SerializeProject(const SceneryProject &project, const ProjectFormat format, std::vector<std::string> *warnings, const SaveOptions &options)
	{
		switch (format)
		{
			case ProjectFormat::Edx:	return WriteEdx(project);
			case ProjectFormat::Sex:	return WriteSex(project, options.sex);
			case ProjectFormat::Wed:	return WriteWed(project, warnings, options.wed);
			case ProjectFormat::Unknown: break;
		}
		return {};
	}

	bool SaveProjectFile(const fs::path &path, const SceneryProject &project, ProjectFormat format, std::vector<std::string> *warnings, std::string *error,
		const SaveOptions &options)
	{
		if (format == ProjectFormat::Unknown)
			format = GetFormatForPath(path);
		if (format == ProjectFormat::Unknown)
		{
			if (error)
				*error = "Pick a project format: " + ToUtf8(path.filename()) + " isn't .edX, .sex or earth.wed.xml";
			return false;
		}
		return WriteFileAtomic(path, SerializeProject(project, format, warnings, options), error);
	}

	std::vector<fs::path> WriteGitFiles(const fs::path &projectFolder)
	{
		std::vector<fs::path> created;
		const auto write = [&](const char *name, const std::string_view text)
		{
			const fs::path path = projectFolder / name;
			std::error_code ec;
			if (fs::exists(path, ec))
				return;
			if (WriteFileAtomic(path, text, nullptr))
				created.push_back(path);
		};

		write(".gitattributes",
			"# Scenery Editor X project: keep the text formats diffable and the binary ones untouched\n"
			"*.edX text eol=lf\n"
			"*.edx text eol=lf\n"
			"*.wed.xml text eol=lf\n"
			"*.dat text eol=lf\n"
			"*.txt text eol=lf\n"
			"*.sex binary\n"
			"*.dsf binary\n"
			"*.dds binary\n"
			"*.png binary\n"
			"*.obj text eol=lf\n");
		write(".gitignore",
			"# Scenery Editor X temporary files (saves write *.tmp-* then rename)\n"
			"*.tmp-*\n"
			"*.old-*\n"
			"earth.wed.bak.xml\n");
		return created;
	}

	// -------------------------------------------------------

	XPlane::AptFile BuildAptFile(const SceneryProject &project)
	{
		// Airports hidden in the hierarchy (themselves or through a group) stay out, as WED's export leaves hidden things out
		std::set<std::string> hidden;
		const std::function<void(const std::vector<SceneryItem> &, bool)> collect = [&](const std::vector<SceneryItem> &items, const bool parentHidden)
		{
			for (const SceneryItem &item : items)
			{
				const bool isHidden = parentHidden || item.hidden;
				if (item.kind == ItemKind::Airport && isHidden)
					hidden.insert(item.airportId);
				collect(item.children, isHidden);
			}
		};
		collect(project.items, false);

		XPlane::AptFile file;
		file.version = 1200;
		file.headerComment = "Generated by Scenery Editor X";
		for (const ProjectAirport &airport : project.airports)
		{
			if (!hidden.contains(airport.airport.id))
				file.airports.push_back(airport.airport);
		}
		return file;
	}

	size_t AddAptAirports(SceneryProject &project, const XPlane::AptFile &file)
	{
		size_t count = 0;
		for (const XPlane::AptAirport &airport : file.airports)
		{
			if (ProjectAirport *existing = project.FindAirport(airport.id))
				existing->airport = airport;
			else
			{
				ProjectAirport added;
				added.airport = airport;
				project.airports.push_back(std::move(added));
			}
			++count;
		}
		NormalizeProject(project);	// Places the new airports in the hierarchy
		return count;
	}

	std::string WriteLibraryText(const XPlane::LibraryFile &library)
	{
		std::string out = "A\n" + std::to_string(library.version == 0 ? 800 : library.version) + "\nLIBRARY\n";

		for (const XPlane::LibraryRegion &region : library.regions)
		{
			out += "\nREGION_DEFINE " + region.name + "\n";
			if (region.all)
				out += "REGION_ALL\n";
			for (const std::array<int, 4> &rect : region.rects)
				out += "REGION_RECT " + std::to_string(rect[0]) + " " + std::to_string(rect[1]) + " " + std::to_string(rect[2]) + " " + std::to_string(rect[3]) + "\n";
			if (!region.bitmap.empty())
				out += "REGION_BITMAP " + region.bitmap + "\n";
			for (const XPlane::LibraryRegion::Condition &condition : region.conditions)
				out += "REGION_DREF " + condition.dataref + " " + condition.comparison + " " + condition.value + "\n";
		}

		// Worldwide exports first, then each region's (REGION can't switch back to worldwide); order kept within each
		std::vector<const XPlane::LibraryExport *> ordered;
		std::vector<std::string> regionOrder;
		for (const XPlane::LibraryExport &entry : library.exports)
		{
			if (entry.region.empty())
				ordered.push_back(&entry);
			else if (std::ranges::find(regionOrder, entry.region) == regionOrder.end())
				regionOrder.push_back(entry.region);
		}
		for (const std::string &region : regionOrder)
		{
			for (const XPlane::LibraryExport &entry : library.exports)
			{
				if (entry.region == region)
					ordered.push_back(&entry);
			}
		}

		std::string region;
		bool isPrivate = false, deprecated = false, semiDeprecated = false;
		uint32_t publicDate = 0;
		out.push_back('\n');
		for (const XPlane::LibraryExport *entry : ordered)
		{
			if (entry->region != region)
			{
				region = entry->region;
				out += "\nREGION " + region + "\n";
			}
			if (entry->isPrivate != isPrivate || entry->deprecated != deprecated || entry->semiDeprecated != semiDeprecated || entry->publicDate != publicDate)
			{
				// PUBLIC resets every flag (and sets the date); the others are switched on after it
				out += entry->publicDate != 0 ? "PUBLIC " + std::to_string(entry->publicDate) + "\n" : std::string("PUBLIC\n");
				if (entry->isPrivate)
					out += "PRIVATE\n";
				if (entry->deprecated)
					out += "DEPRECATED\n";
				if (entry->semiDeprecated)
					out += "SEMI_DEPRECATED\n";
				isPrivate = entry->isPrivate;
				deprecated = entry->deprecated;
				semiDeprecated = entry->semiDeprecated;
				publicDate = entry->publicDate;
			}

			char ratio[32];
			std::snprintf(ratio, sizeof(ratio), "%g", static_cast<double>(entry->ratio));
			std::string seasonList;
			for (const auto &[bit, name] : { std::pair<uint8_t, const char *>{ XPlane::SeasonSpring, "spr" }, { XPlane::SeasonSummer, "sum" },
				{ XPlane::SeasonFall, "fal" }, { XPlane::SeasonWinter, "win" } })
			{
				if (entry->seasons & bit)
					seasonList += (seasonList.empty() ? "" : ",") + std::string(name);
			}

			if (entry->seasons != 0)
			{
				if (entry->ratio != 1.0f)
					out += std::string("EXPORT_RATIO_SEASON ") + seasonList + " " + ratio + " ";
				else if (entry->kind == XPlane::ExportKind::Extend)
					out += "EXPORT_EXTEND_SEASON " + seasonList + " ";
				else if (entry->kind == XPlane::ExportKind::Exclude)
					out += "EXPORT_EXCLUDE_SEASON " + seasonList + " ";
				else
					out += "EXPORT_SEASON " + seasonList + " ";
			}
			else if (entry->ratio != 1.0f)
				out += std::string("EXPORT_RATIO ") + ratio + " ";
			else
				out += std::string(XPlane::GetExportKindName(entry->kind)) + " ";
			out += entry->virtualPath + " " + entry->realPath + "\n";
		}
		return out;
	}
}

// -------------------------------------------------------

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
 * project_document.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "project_document.h"
#include <algorithm>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	namespace fs = std::filesystem;

	namespace
	{
		bool Contains(const SceneryItem &item, const std::string_view id)
		{
			if (item.id == id)
				return true;
			return std::ranges::any_of(item.children, [id](const SceneryItem &child) { return Contains(child, id); });
		}

		void CollectAirportIds(const SceneryItem &item, std::vector<std::string> &ids)
		{
			if (item.kind == ItemKind::Airport)
				ids.push_back(item.airportId);
			for (const SceneryItem &child : item.children)
				CollectAirportIds(child, ids);
		}
	}

	// -------------------------------------------------------

	void ProjectDocument::ResetState()
	{
		m_Revision = 0;
		m_SavedRevision = 0;
		m_AutosavedRevision = 0;
		++m_Structure;
		m_InfoRevision = m_LibrariesRevision = m_ItemsRevision = m_LibraryRevision = 1;
		m_AirportRevisions.assign(m_Project.airports.size(), 1);
		m_Index.clear();
		m_IndexStructure = ~uint64_t(0);
		m_EncodeCache.Clear();
		m_FromSnapshot = false;
		m_Warnings.clear();
	}

	void ProjectDocument::New(std::string name, const ProjectKind kind)
	{
		Close();
		m_Project = MakeNewProject(std::move(name), kind);
		m_Project.info.editorVersion = m_EditorVersion;
		m_Path.clear();
		m_Format = ProjectFormat::Edx;
		ResetState();
		m_Open = true;
		// A new project counts as changed until it's saved somewhere
		m_Revision = 1;
	}

	void ProjectDocument::Adopt(SceneryProject project)
	{
		Close();
		m_Project = std::move(project);
		NormalizeProject(m_Project);
		m_Path.clear();
		m_Format = ProjectFormat::Edx;
		ResetState();
		m_Open = true;
		m_Revision = 1;
	}

	bool ProjectDocument::Open(const fs::path &path, std::string *error, const OpenOptions &options)
	{
		SceneryProject project;
		std::vector<std::string> warnings;
		ProjectFormat format = GetFormatForPath(path);
		bool fromSnapshot = false;

		// The snapshot is only good for text formats: a .sex already is one
		if (options.useSnapshot && m_Cache && format != ProjectFormat::Sex && m_Cache->LoadSnapshot(path, project))
			fromSnapshot = true;
		else if (!LoadProjectFile(path, project, &format, &warnings, error, options.load))
			return false;

		Close();
		m_Project = std::move(project);
		m_Path = path;
		m_Format = format;
		ResetState();
		m_Warnings = std::move(warnings);
		m_FromSnapshot = fromSnapshot;
		m_Open = true;

		if (!fromSnapshot && m_Cache && format != ProjectFormat::Sex)
			m_Cache->StoreSnapshot(path, m_Project);
		return true;
	}

	bool ProjectDocument::Write(const fs::path &path, ProjectFormat format, const bool bind, std::string *error)
	{
		if (!m_Open)
		{
			if (error)
				*error = "No project is open";
			return false;
		}
		if (format == ProjectFormat::Unknown)
			format = GetFormatForPath(path);
		if (format == ProjectFormat::Unknown)
		{
			if (error)
				*error = "Save as .edX, .sex or earth.wed.xml";
			return false;
		}

		// The edit date and editor version move only when the content changed, so saving an unchanged project leaves git clean.
		const bool changed = IsDirty() || !HasFile() || fs::path(path) != m_Path;
		const ProjectInfo previous = m_Project.info;
		if (changed)
		{
			m_Project.info.modified = GetUtcTimestamp();
			if (!m_EditorVersion.empty())
				m_Project.info.editorVersion = m_EditorVersion;
			if (m_Project.info.created.empty())
				m_Project.info.created = m_Project.info.modified;
		}

		std::error_code ec;
		if (m_Cache && m_SaveSettings.backup && fs::exists(path, ec))
			m_Cache->StoreBackup(path, m_SaveSettings.backupsKept);

		SaveOptions options;
		options.wed = m_SaveSettings.wed;
		const SexSectionRevisions revisions = GetSectionRevisions();
		if (m_SaveSettings.useEncodeCache)
		{
			options.sex.cache = &m_EncodeCache;
			options.sex.revisions = &revisions;
		}
		std::vector<std::string> warnings;
		if (!WriteFileAtomic(path, SerializeProject(m_Project, format, &warnings, options), error))
		{
			m_Project.info = previous;
			return false;
		}
		if (changed)
			Touch(Section::Info, NO_INDEX);	// The header changed in memory too; the next autosave must not reuse it

		if (bind)
		{
			const fs::path oldKey = GetRecoveryKey();
			m_Path = path;
			m_Format = format;
			m_SavedRevision = m_Revision;
			m_AutosavedRevision = m_Revision;
			m_Warnings = std::move(warnings);
			if (m_Cache)
			{
				m_Cache->ClearRecovery(oldKey);
				m_Cache->ClearRecovery(path);
				if (m_SaveSettings.updateSnapshot && format != ProjectFormat::Sex)
					m_Cache->StoreSnapshot(path, m_Project);
			}
		}
		return true;
	}

	bool ProjectDocument::Save(std::string *error)
	{
		if (!HasFile())
		{
			if (error)
				*error = "The project hasn't been saved yet: choose where (Save As)";
			return false;
		}
		return Write(m_Path, m_Format, true, error);
	}

	bool ProjectDocument::SaveAs(const fs::path &path, const ProjectFormat format, std::string *error)
	{
		return Write(path, format, true, error);
	}

	bool ProjectDocument::ExportTo(const fs::path &path, ProjectFormat format, std::vector<std::string> *warnings, std::string *error) const
	{
		if (!m_Open)
		{
			if (error)
				*error = "No project is open";
			return false;
		}
		if (format == ProjectFormat::Unknown)
			format = GetFormatForPath(path);
		SaveOptions options;
		options.wed = m_SaveSettings.wed;
		return SaveProjectFile(path, m_Project, format, warnings, error, options);
	}

	void ProjectDocument::Close(const bool keepRecovery)
	{
		if (!m_Open)
			return;
		if (m_Cache && !keepRecovery)
			m_Cache->ClearRecovery(GetRecoveryKey());
		m_Project = SceneryProject{};
		m_Path.clear();
		m_Format = ProjectFormat::Unknown;
		m_Open = false;
		ResetState();
	}

	bool ProjectDocument::Autosave(std::string *error)
	{
		if (!m_Open || !m_Cache || !IsDirty() || m_AutosavedRevision == m_Revision)
			return true;
		const SexSectionRevisions revisions = GetSectionRevisions();
		SexWriteOptions options;
		options.cache = &m_EncodeCache;
		options.revisions = &revisions;
		if (!m_Cache->WriteRecovery(GetRecoveryKey(), m_Project, error, options))
			return false;
		m_AutosavedRevision = m_Revision;
		return true;
	}

	std::optional<RecoveryInfo> ProjectDocument::GetRecovery() const
	{
		return m_Cache ? m_Cache->GetRecovery(GetRecoveryKey()) : std::nullopt;
	}

	bool ProjectDocument::RestoreRecovery(std::vector<std::string> *warnings, std::string *error)
	{
		if (!m_Cache)
			return false;
		SceneryProject recovered;
		if (!m_Cache->LoadRecovery(GetRecoveryKey(), recovered, warnings, error))
			return false;
		m_Project = std::move(recovered);
		const fs::path path = m_Path;
		const ProjectFormat format = m_Format;
		ResetState();
		m_Path = path;
		m_Format = format;
		m_Open = true;
		m_Revision = 1;					// Restored edits are unsaved
		m_AutosavedRevision = 1;		// ...and already in the recovery file
		return true;
	}

	void ProjectDocument::DiscardRecovery()
	{
		if (m_Cache)
			m_Cache->ClearRecovery(GetRecoveryKey());
	}

	std::string ProjectDocument::GetTitle() const
	{
		if (!m_Open)
			return {};
		const std::string name = m_Project.info.name.empty() ? std::string("Untitled Project") : m_Project.info.name;
		return IsDirty() ? name + " *" : name;
	}

	fs::path ProjectDocument::GetRecoveryKey() const
	{
		// An unsaved project's edits are kept under its uuid until it has a file
		return HasFile() ? m_Path : fs::path("untitled") / (m_Project.info.uuid + EDX_EXTENSION);
	}

	// -------------------------------------------------------

	void ProjectDocument::Touch(const Section section, const size_t airport)
	{
		const uint64_t revision = m_Revision + 1 > 1 ? m_Revision + 1 : 2;
		switch (section)
		{
			case Section::Info:			m_InfoRevision = revision; break;
			case Section::Libraries:	m_LibrariesRevision = revision; break;
			case Section::Items:		m_ItemsRevision = revision; ++m_Structure; break;
			case Section::Library:		m_LibraryRevision = revision; break;
			case Section::Airport:
				m_AirportRevisions.resize(m_Project.airports.size(), revision);
				if (airport < m_AirportRevisions.size())
					m_AirportRevisions[airport] = revision;
				else
					std::ranges::fill(m_AirportRevisions, revision);
				break;
			case Section::All:
				m_InfoRevision = m_LibrariesRevision = m_ItemsRevision = m_LibraryRevision = revision;
				m_AirportRevisions.assign(m_Project.airports.size(), revision);
				++m_Structure;
				break;
		}
	}

	void ProjectDocument::Mark(const Section section, const size_t airport)
	{
		if (!m_Open)
			return;
		Touch(section, airport);
		++m_Revision;
		if (m_Revision < 2)
			m_Revision = 2;		// Revision 1 means "new / restored, unsaved"
	}

	SexSectionRevisions ProjectDocument::GetSectionRevisions() const
	{
		SexSectionRevisions revisions;
		revisions.info = m_InfoRevision;
		revisions.libraries = m_LibrariesRevision;
		revisions.items = m_ItemsRevision;
		revisions.library = m_LibraryRevision;
		revisions.airports = m_AirportRevisions;
		// An airport added without Mark() must never be served from the cache
		revisions.airports.resize(m_Project.airports.size(), 0);
		return revisions;
	}

	void ProjectDocument::RebuildIndex()
	{
		m_Index.clear();
		m_Project.ForEachItem([this](SceneryItem &item, int)
		{
			m_Index.emplace(item.id, &item);
			return true;
		});
		m_IndexStructure = m_Structure;
	}

	SceneryItem *ProjectDocument::FindItem(const std::string_view id)
	{
		if (m_IndexStructure != m_Structure)
			RebuildIndex();
		const auto it = m_Index.find(std::string(id));
		return it != m_Index.end() ? it->second : nullptr;
	}

	std::string ProjectDocument::AddItem(SceneryItem item, const std::string_view parentId, const size_t index)
	{
		if (!m_Open)
			return {};
		std::vector<SceneryItem> *list = &m_Project.items;
		if (!parentId.empty())
		{
			SceneryItem *parent = FindItem(parentId);
			if (!parent || !parent->IsContainer())
				return {};
			list = &parent->children;
		}

		// Ids stay unique across the whole tree (the item and anything it brings along)
		const std::function<void(SceneryItem &)> assignIds = [&](SceneryItem &entry)
		{
			while (entry.id.empty() || FindItem(entry.id))
				entry.id = MakeItemId();
			for (SceneryItem &child : entry.children)
				assignIds(child);
		};
		assignIds(item);

		const std::string id = item.id;
		list->insert(list->begin() + static_cast<std::ptrdiff_t>(std::min(index, list->size())), std::move(item));
		Mark(Section::Items);
		return id;
	}

	bool ProjectDocument::RemoveItem(const std::string_view id)
	{
		size_t index = 0;
		std::vector<SceneryItem> *list = m_Project.FindParentList(id, &index);
		if (!list)
			return false;

		std::vector<std::string> airports;
		CollectAirportIds((*list)[index], airports);
		list->erase(list->begin() + static_cast<std::ptrdiff_t>(index));
		for (const std::string &airportId : airports)
		{
			const size_t airport = FindAirportIndex(airportId);
			if (airport == NO_INDEX)
				continue;
			m_Project.airports.erase(m_Project.airports.begin() + static_cast<std::ptrdiff_t>(airport));
			if (airport < m_AirportRevisions.size())
				m_AirportRevisions.erase(m_AirportRevisions.begin() + static_cast<std::ptrdiff_t>(airport));
		}
		Mark(Section::Items);
		return true;
	}

	bool ProjectDocument::MoveItem(const std::string_view id, const std::string_view newParentId, const size_t index)
	{
		SceneryItem *item = FindItem(id);
		if (!item)
			return false;
		if (!newParentId.empty())
		{
			SceneryItem *parent = FindItem(newParentId);
			if (!parent || !parent->IsContainer() || Contains(*item, newParentId))
				return false;	// Not a container, or the item's own subtree
		}

		size_t from = 0;
		std::vector<SceneryItem> *source = m_Project.FindParentList(id, &from);
		SceneryItem moving = std::move((*source)[from]);
		source->erase(source->begin() + static_cast<std::ptrdiff_t>(from));
		++m_Structure;	// The pointers in the index moved

		std::vector<SceneryItem> *target = &m_Project.items;
		if (!newParentId.empty())
			target = &FindItem(newParentId)->children;
		target->insert(target->begin() + static_cast<std::ptrdiff_t>(std::min(index, target->size())), std::move(moving));
		Mark(Section::Items);
		return true;
	}

	ProjectAirport *ProjectDocument::AddAirport(ProjectAirport airport, const std::string_view parentId)
	{
		if (!m_Open)
			return nullptr;
		const std::string id = airport.airport.id;
		if (const size_t existing = FindAirportIndex(id); existing != NO_INDEX)
		{
			m_Project.airports[existing] = std::move(airport);
			Mark(Section::Airport, existing);
			return &m_Project.airports[existing];
		}

		m_Project.airports.push_back(std::move(airport));
		m_AirportRevisions.resize(m_Project.airports.size(), 0);
		Mark(Section::Airport, m_Project.airports.size() - 1);

		SceneryItem item;
		item.kind = ItemKind::Airport;
		item.name = m_Project.airports.back().airport.name;
		item.airportId = id;
		SceneryItem topLevel = item;
		if (AddItem(std::move(item), parentId).empty())
			AddItem(std::move(topLevel));	// The parent wasn't a container: place it at the top level
		return &m_Project.airports.back();
	}

	size_t ProjectDocument::FindAirportIndex(const std::string_view airportId) const
	{
		for (size_t i = 0; i < m_Project.airports.size(); ++i)
		{
			if (m_Project.airports[i].airport.id == airportId)
				return i;
		}
		return NO_INDEX;
	}
}

// -------------------------------------------------------

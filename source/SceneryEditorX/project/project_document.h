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
 * project_document.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "project_cache.h"
#include "project_io.h"
#include "scenery_project.h"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::ProjectFile
{
	struct DocumentOpenOptions
	{
		LoadOptions load;
		bool useSnapshot = true;		// Read the cache snapshot when the file is unchanged since it was taken
	};

	struct DocumentSaveSettings
	{
		bool backup = true;				// Copy the previous file into the cache's backups first
		size_t backupsKept = 5;
		bool updateSnapshot = true;
		bool useEncodeCache = false;	// Reuse unchanged encoded sections for .sex (autosave always does)
		WedExportOptions wed;
	};

	/**
	 * @class ProjectDocument
	 * @brief A project open for editing: the data, the file it belongs to, whether it changed, and the edit cache.
	 *
	 * Edits go through Mark() (or the item helpers, which call it): each section of the project carries the revision of
	 * its last edit, so autosaves (and .sex saves) only encode the airports and sections that changed, and the item
	 * lookup index is rebuilt only after the hierarchy changed. Saves are atomic, keep a backup of the previous file in
	 * the cache, refresh the snapshot that makes the next open instant and remove the crash-recovery file.
	 * Not thread safe: one owner (the editor's main thread).
	 */
	class ProjectDocument
	{
	public:
		/* @brief Parts of a project with their own revision. */
		enum class Section : uint8_t
		{
			Info,			// Header, datum, preferences, properties
			Libraries,
			Airport,		// One airport (pass its index)
			Items,			// The scene hierarchy
			Library,		// Library projects' library.txt
			All
		};

		using OpenOptions = DocumentOpenOptions;
		using SaveSettings = DocumentSaveSettings;

		static constexpr size_t NO_INDEX = static_cast<size_t>(-1);

		ProjectDocument() = default;

		/* @brief The cache to use (snapshots, recovery, backups); null disables it. The cache must outlive the document. */
		void SetCache(const ProjectCache *cache) { m_Cache = cache; }
		void SetEditorVersion(std::string version) { m_EditorVersion = std::move(version); }
		SaveSettings &GetSaveSettings() { return m_SaveSettings; }

		// Lifecycle ----------------------------------------------------

		/* @brief Starts a new, unsaved project. */
		void New(std::string name, ProjectKind kind = ProjectKind::Scenery);

		/* @brief Starts from a project in memory (an import), unsaved. */
		void Adopt(SceneryProject project);

		/* @brief Opens a project file (.edX, .sex or earth.wed.xml). The open document is replaced only when it succeeds. */
		bool Open(const std::filesystem::path &path, std::string *error = nullptr, const OpenOptions &options = {});

		/* @brief Saves to the file the project belongs to. False for a project that has none yet (use SaveAs). */
		bool Save(std::string *error = nullptr);

		/* @brief Saves to another file and binds the project to it. Unknown format: from the name. */
		bool SaveAs(const std::filesystem::path &path, ProjectFormat format = ProjectFormat::Unknown, std::string *error = nullptr);

		/* @brief Writes a copy in some format (WorldEditor, apt.dat via BuildAptFile, ...) without rebinding or clearing the dirty flag. */
		bool ExportTo(const std::filesystem::path &path, ProjectFormat format, std::vector<std::string> *warnings = nullptr, std::string *error = nullptr) const;

		/**
		 * @brief Closes the project.
		 * @param keepRecovery True keeps the autosaved edits for the next open (the editor is closing with unsaved work
		 * the user didn't discard); false removes them.
		 */
		void Close(bool keepRecovery = false);

		/* @brief Writes unsaved edits to the recovery file when they changed since the last autosave. True when nothing failed. */
		bool Autosave(std::string *error = nullptr);

		/* @brief Recovery data for the open project's file, if an earlier session left some. */
		[[nodiscard]] std::optional<RecoveryInfo> GetRecovery() const;

		/* @brief Replaces the open project with its recovered edits (the document is dirty afterwards). */
		bool RestoreRecovery(std::vector<std::string> *warnings = nullptr, std::string *error = nullptr);
		void DiscardRecovery();

		// State --------------------------------------------------------

		[[nodiscard]] bool IsOpen() const { return m_Open; }
		[[nodiscard]] bool HasFile() const { return !m_Path.empty(); }
		[[nodiscard]] bool IsDirty() const { return m_Open && m_Revision != m_SavedRevision; }
		[[nodiscard]] const std::filesystem::path &GetPath() const { return m_Path; }
		[[nodiscard]] ProjectFormat GetFormat() const { return m_Format; }
		[[nodiscard]] bool WasLoadedFromSnapshot() const { return m_FromSnapshot; }
		[[nodiscard]] const std::vector<std::string> &GetWarnings() const { return m_Warnings; }
		[[nodiscard]] uint64_t GetRevision() const { return m_Revision; }
		[[nodiscard]] uint64_t GetStructureRevision() const { return m_Structure; }

		/* @brief "MySceneryProject *" when there are unsaved changes. */
		[[nodiscard]] std::string GetTitle() const;

		[[nodiscard]] SceneryProject &GetProject() { return m_Project; }
		[[nodiscard]] const SceneryProject &GetProject() const { return m_Project; }

		// Edits --------------------------------------------------------

		/* @brief Records an edit to a section (airport: its index). Call after changing GetProject() directly. */
		void Mark(Section section, size_t airport = NO_INDEX);

		/* @brief An item by id, through an index rebuilt only after the hierarchy changed. */
		[[nodiscard]] SceneryItem *FindItem(std::string_view id);

		/**
		 * @brief Adds an item to the hierarchy.
		 * @param parentId A group / airport item, or empty for the top level.
		 * @param index Position among the parent's children (NO_INDEX: at the end).
		 * @return The item's id (a new one when it had none or a taken one), empty when the parent isn't a container.
		 */
		std::string AddItem(SceneryItem item, std::string_view parentId = {}, size_t index = NO_INDEX);

		/* @brief Removes an item and its children. Removing an airport's item removes the airport too. */
		bool RemoveItem(std::string_view id);

		/* @brief Moves an item under another container (or the top level). Refuses to move an item into itself. */
		bool MoveItem(std::string_view id, std::string_view newParentId, size_t index = NO_INDEX);

		/* @brief Adds an airport and its hierarchy item (an airport with the same id is replaced in place). */
		ProjectAirport *AddAirport(ProjectAirport airport, std::string_view parentId = {});

		/* @brief Index of an airport by id, NO_INDEX when there is none. */
		[[nodiscard]] size_t FindAirportIndex(std::string_view airportId) const;

		/* @brief The encode cache autosaves use (hits / misses, for diagnostics and tests). */
		[[nodiscard]] const SexEncodeCache &GetEncodeCache() const { return m_EncodeCache; }

	private:
		bool Write(const std::filesystem::path &path, ProjectFormat format, bool bind, std::string *error);
		void ResetState();
		void Touch(Section section, size_t airport);
		[[nodiscard]] SexSectionRevisions GetSectionRevisions() const;
		[[nodiscard]] std::filesystem::path GetRecoveryKey() const;
		void RebuildIndex();

		SceneryProject m_Project;
		std::filesystem::path m_Path;
		ProjectFormat m_Format = ProjectFormat::Unknown;
		bool m_Open = false;
		bool m_FromSnapshot = false;
		std::vector<std::string> m_Warnings;

		uint64_t m_Revision = 0;
		uint64_t m_SavedRevision = 0;
		uint64_t m_AutosavedRevision = 0;
		uint64_t m_Structure = 0;

		// Section revisions (edit counter value of each section's last edit)
		uint64_t m_InfoRevision = 1;
		uint64_t m_LibrariesRevision = 1;
		uint64_t m_ItemsRevision = 1;
		uint64_t m_LibraryRevision = 1;
		std::vector<uint64_t> m_AirportRevisions;

		std::unordered_map<std::string, SceneryItem *> m_Index;
		uint64_t m_IndexStructure = ~uint64_t(0);

		const ProjectCache *m_Cache = nullptr;
		SexEncodeCache m_EncodeCache;
		SaveSettings m_SaveSettings;
		std::string m_EditorVersion;
	};
}

// -------------------------------------------------------

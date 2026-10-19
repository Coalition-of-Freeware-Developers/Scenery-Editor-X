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
 * xp_airport_document.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_airport_index.h"
#include "xp_apt.h"
#include <filesystem>
#include <string>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * @class AirportDocument
	 * @brief An airport open for editing: its apt.dat rows, the scenery pack they live in and whether they changed.
	 *
	 * A scenery pack holds "Earth nav data/apt.dat", which may list several airports; saving rewrites that file with
	 * this airport's rows replaced and the others kept. Airports that come from Global Airports or default scenery are
	 * read-only (X-Plane and the Gateway own those files): edit them, then Save As into a pack of your own.
	 */
	class AirportDocument
	{
	public:
		AptAirport airport;

		[[nodiscard]] const std::filesystem::path &GetAptDatPath() const { return m_AptDat; }
		[[nodiscard]] const std::filesystem::path &GetPackageFolder() const { return m_Package; }
		[[nodiscard]] const std::string &GetPackageName() const { return m_PackageName; }
		[[nodiscard]] bool IsReadOnly() const { return m_ReadOnly; }
		[[nodiscard]] bool IsDirty() const { return m_Dirty; }
		[[nodiscard]] bool HasFile() const { return !m_AptDat.empty(); }

		/* @brief Counts every edit; scene builders compare it to know when something changed. */
		[[nodiscard]] uint64_t GetRevision() const { return m_Revision; }

		/* @brief Counts the edits that change geometry (not just where a point feature sits), which need a rebuild. */
		[[nodiscard]] uint64_t GetGeometryRevision() const { return m_GeometryRevision; }

		/**
		 * @brief Records an edit (bumps the revisions and marks the document dirty).
		 * @param geometryChanged False when only a point feature moved or turned (position, heading), which the scene
		 * applies by moving its entity instead of rebuilding the meshes.
		 */
		void MarkDirty(bool geometryChanged = true);

		/* @brief Clears the dirty flag after the airport was saved somewhere else (into a project file). */
		void ClearDirty() { m_Dirty = false; }

		/* @brief "KSEA  Seattle Tacoma Intl" plus "*" when dirty: the title for the UI. */
		[[nodiscard]] std::string GetTitle() const;

		/* @brief Loads an airport the index found (its byte range of the apt.dat). */
		static bool Open(const AirportIndex &index, const AirportEntry &entry, AirportDocument &out, std::string *error = nullptr);

		/**
		 * @brief Loads an airport straight from an apt.dat (the first one, or the one with `airportId`).
		 * @param readOnly Treat the file as not ours (default scenery).
		 */
		static bool OpenFile(const std::filesystem::path &aptDat, std::string_view airportId, AirportDocument &out, std::string *error = nullptr, bool readOnly = false);

		/**
		 * @brief Creates a scenery pack folder with "Earth nav data/apt.dat" holding one new airport, and opens it.
		 * @param packageFolder The pack folder to create (must not already hold an apt.dat).
		 */
		static bool Create(const std::filesystem::path &packageFolder, const AptAirport &airport, AirportDocument &out, std::string *error = nullptr);

		/* @brief Rewrites the pack's apt.dat with this airport's rows (other airports in the file are kept). Fails when read-only. */
		bool Save(std::string *error = nullptr);

		/**
		 * @brief Writes this airport into another scenery pack (created when missing; an existing apt.dat there gets
		 * this airport added or replaced) and binds the document to it. The document is writable afterwards.
		 */
		bool SaveAs(const std::filesystem::path &packageFolder, std::string *error = nullptr);

		/* @brief Drops the file binding (a copy to be saved elsewhere). */
		void Detach();

	private:
		std::filesystem::path m_AptDat;
		std::filesystem::path m_Package;
		std::string m_PackageName;
		std::string m_OriginalId;			// Id the airport had in the file, to find its rows when saving
		bool m_ReadOnly = false;
		bool m_Dirty = false;
		uint64_t m_Revision = 0;
		uint64_t m_GeometryRevision = 0;
	};

	// -------------------------------------------------------

	/* @brief True when a folder is a scenery pack with airports ("Earth nav data/apt.dat"). */
	bool IsAirportPackage(const std::filesystem::path &folder);

	/* @brief The apt.dat path of a scenery pack folder. */
	std::filesystem::path GetPackageAptDat(const std::filesystem::path &folder);

	/* @brief True for packs X-Plane ships (Global Scenery, Resources) that the editor shouldn't write to. */
	bool IsProtectedPackage(const std::filesystem::path &folder, const std::filesystem::path &xplaneRoot);

	/**
	 * @brief Adds a pack to the top of Custom Scenery/scenery_packs.ini (highest priority) when it isn't listed.
	 * X-Plane does this itself on its next start; doing it here lets the editor's airport index see the pack in order.
	 */
	bool RegisterSceneryPack(const std::filesystem::path &xplaneRoot, const std::filesystem::path &packageFolder, std::string *error = nullptr);

	/**
	 * @brief A new airport ready to edit: metadata, one runway (land) / water runway (seaplane) / helipad (heliport)
	 * centered on the datum, a windsock and a beacon.
	 */
	AptAirport MakeNewAirport(std::string id, std::string name, double latitude, double longitude, AirportType type, int elevationFt = 0);
}

// -------------------------------------------------------

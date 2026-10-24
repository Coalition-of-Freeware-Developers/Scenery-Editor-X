/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_library_txt.cpp
 * -------------------------------------------------------
 * library.txt (xp_library): parsing, the region tests the library browser runs, and the library index the editor
 * builds from every package (priority, EXPORT_EXTEND / EXCLUDE / BACKUP resolution) with the fuzzed file as one
 * package's library.txt.
 * -------------------------------------------------------
 */
#include "common/fuzz_assets.h"
#include "common/fuzz_common.h"
#include <SceneryEditorX/xplane/xp_library.h>
#include <fstream>

using namespace SceneryEditorX::XPlane;
namespace fs = std::filesystem;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size)
{
	LibraryFile library;
	std::string error;
	if (!ParseLibrary(SceneryEditorX::Fuzz::AsText(data, size), library, &error))
		return 0;

	for (const LibraryRegion &region : library.regions)
	{
		for (const auto &[latitude, longitude] : { std::pair{ 0.0, 0.0 }, std::pair{ 47.5, -122.3 }, std::pair{ -89.99, 179.99 }, std::pair{ 90.0, -180.0 } })
			(void)region.Contains(latitude, longitude);
	}
	for (const LibraryExport &entry : library.exports)
	{
		(void)GetAssetTypeName(GetAssetType(entry.virtualPath));
		(void)FormatSeasons(entry.seasons);
	}

	// The index reads library.txt from the package folder: two packages, the fuzzed one above a fixed one
	const fs::path root = SceneryEditorX::Fuzz::GetAssetPack() / "library";
	fs::create_directories(root / "fuzzed");
	fs::create_directories(root / "base");
	{
		std::ofstream out(root / "fuzzed" / "library.txt", std::ios::binary | std::ios::trunc);
		out.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(size));
	}
	if (!fs::exists(root / "base" / "library.txt"))
	{
		std::ofstream out(root / "base" / "library.txt", std::ios::binary);
		out << "A\n800\nLIBRARY\n\nEXPORT lib/airport/ground/pavement/asphalt/strips.pol strips.pol\nEXPORT_BACKUP lib/g10/x.obj x.obj\n";
	}

	std::vector<LibraryPackage> packages(2);
	packages[0].name = "fuzzed";
	packages[0].path = root / "fuzzed";
	packages[0].hasLibrary = true;
	packages[0].priority = 0;
	packages[1].name = "base";
	packages[1].path = root / "base";
	packages[1].hasLibrary = true;
	packages[1].priority = 1;
	packages[1].source = PackageSource::GlobalScenery;

	LibraryIndex index;
	LibraryIndex::BuildOptions options;
	options.checkFilesExist = false;
	index.Build(std::move(packages), options);
	for (const LibraryAsset &asset : index.GetAssets())
	{
		SEDX_FUZZ_CHECK(asset.resolved >= -1 && asset.resolved < static_cast<int>(asset.providers.size()), "resolved provider of " + asset.virtualPath);
		for (const AssetProvider &provider : asset.providers)
			SEDX_FUZZ_CHECK(provider.package < index.GetPackages().size(), "provider package of " + asset.virtualPath);
		(void)index.Find(asset.virtualPath);
		(void)index.Resolve(asset.virtualPath);
		(void)asset.IsPrivate();
		(void)asset.IsDeprecated();
	}
	return 0;
}

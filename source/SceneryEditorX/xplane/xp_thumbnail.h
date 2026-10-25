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
 * xp_thumbnail.h
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_assets.h"
#include "xp_obj8.h"
#include "xp_overlay.h"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * Asset thumbnails: a small software renderer that draws an X-Plane asset into an RGBA image on any thread, so the
	 * editor can show (and cache) a picture of every library item and project art file. Standard library only.
	 *
	 * What is drawn follows WorldEditor's library preview (WED_LibraryPreviewPane):
	 *  - .obj   LOD 0 at rest (every animation dataref 0)
	 *  - .fac   a 20 m square, 10 m high (a middle floor's height for type 2), its front facing the camera and each side
	 *           showing another wall type, on a lawn (water for piers); open facades leave a gap in the last side
	 *  - .for   a 6 x 6 grid of trees at the forest's spacing and jitter (3 x 3 with fewer than four kinds of tree),
	 *           kinds by their percentages, heights between their limits
	 *  - .str   its first object
	 *  - .agp   the tile with its objects
	 *  - .pol   one texture repeat, flat, in the aspect of its SCALE
	 *  - .lin   one texture repeat upright, each S_OFFSET row as a strip as wide as it paints (on a light background
	 *           when the paint is dark)
	 *  - .png / .dds and other images: the image itself, fitted
	 * The rest (.net, .ter, .agb, .ags, .bch, .dcl) has no thumbnail.
	 *
	 * Local frame: x east, y up, z south (the overlay builder's). The 3D view looks from the south, turned 30 degrees
	 * toward the east and 20 degrees above (WorldEditor's 10 and 10, opened up to read at thumbnail size), lit from the
	 * viewer's upper left.
	 */

	/* @brief Bump when thumbnails change, so cached ones are drawn again. */
	constexpr uint32_t THUMBNAIL_VERSION = 1;

	/* @brief An 8-bit RGBA image (straight alpha). */
	struct RgbaImage
	{
		uint32_t width = 0;
		uint32_t height = 0;
		std::vector<uint8_t> pixels;	// width * height * 4

		[[nodiscard]] bool IsValid() const { return width > 0 && height > 0 && pixels.size() == static_cast<size_t>(width) * height * 4; }
	};

	/**
	 * @brief Decodes one mip of a DDS file: the smallest with its longer side at least `minSize` texels (the first when
	 * none is). BC1 (DXT1), BC2 (DXT3), BC3 (DXT5), BC4 / BC5 (ATI1, ATI2, BC4U, BC5U and the DX10 formats) and
	 * uncompressed 8 to 32-bit formats with channel masks. BC7 isn't decoded. Rows stay in file order (top first).
	 */
	bool DecodeDds(std::span<const uint8_t> bytes, uint32_t minSize, RgbaImage &out, std::string *error = nullptr);

	/* @brief Reverses the row order. */
	void FlipRows(RgbaImage &image);

	/* @brief Halves an image (box filter, alpha weighted) until its longer side is at most maxSize. */
	void ShrinkImage(RgbaImage &image, uint32_t maxSize);

	/**
	 * @brief Reads an image file for a thumbnail (a concrete file: the .dds / .png choice is already made), row 0 at the
	 * bottom (X-Plane's t = 0). `minSize` is the size the thumbnail needs: a smaller mip, or a shrunk image, will do.
	 */
	using ThumbnailTextureLoader = std::function<bool(const std::filesystem::path &file, uint32_t minSize, RgbaImage &out)>;

	struct ThumbnailVertex
	{
		float position[3] = { 0.0f, 0.0f, 0.0f };
		float normal[3] = { 0.0f, 1.0f, 0.0f };
		float uv[2] = { 0.0f, 0.0f };
	};

	struct ThumbnailMesh
	{
		std::vector<ThumbnailVertex> vertices;
		std::vector<uint32_t> indices;					// Triangles, either winding (everything is drawn two-sided)
		int texture = -1;								// Index into ThumbnailScene::textures, -1 for none
		float color[4] = { 0.7f, 0.7f, 0.7f, 1.0f };	// Tint, or the color without a texture (sRGB)
		float cutoff = 0.5f;							// Texels with less alpha aren't drawn (3D meshes)
		bool draped = false;							// On the ground: drawn first, blended in order, then the 3D meshes
		bool lit = true;								// False: its color as it is (a ground plane)
		bool frame = true;								// False: left out of the framing (a ground plane reaching past the frame)
	};

	struct ThumbnailScene
	{
		std::vector<ThumbnailMesh> meshes;
		std::vector<RgbaImage> textures;				// Row 0 at the bottom
	};

	enum class ThumbnailFraming : uint8_t
	{
		Object,		// A three-quarter view from the south-east
		Ground		// Draped art: a steep view from the south
	};

	struct ThumbnailOptions
	{
		uint32_t size = 128;							// Pixels, square
		uint32_t supersample = 2;						// Samples per pixel along each axis
		ThumbnailFraming framing = ThumbnailFraming::Object;
		float margin = 0.06f;							// Fraction of the size left around the asset
	};

	/**
	 * @brief Draws a scene into a size x size image with a transparent background, framed to fill it.
	 * @return False when nothing was drawn (no triangles).
	 */
	bool RenderThumbnail(const ThumbnailScene &scene, const ThumbnailOptions &options, RgbaImage &out);

	/**
	 * @brief Fits an image (row 0 at the bottom) into size x size, centered; the output's row 0 is the top.
	 * @param aspect Width over height to show it at; 0 keeps the image's.
	 */
	bool FitThumbnailImage(const RgbaImage &image, uint32_t size, RgbaImage &out, float aspect = 0.0f);

	/**
	 * @brief A painted line as WorldEditor previews it: one texture repeat from the bottom to the top, each S_OFFSET row
	 * a strip centered where the line runs and as wide as it paints (relative to the repeat length), drawn in file order.
	 * Dark paint gets a light background.
	 * @param texture The line's texture, row 0 at the bottom.
	 */
	bool RenderLineThumbnail(const LineAsset &asset, const RgbaImage &texture, uint32_t size, RgbaImage &out);

	/* @brief True for the files ThumbnailBuilder can draw (by extension). */
	bool CanMakeThumbnail(std::string_view path);

	/* @brief True for image files (drawn as themselves). */
	bool IsThumbnailImageFile(std::string_view path);

	/**
	 * @brief A texture reference as X-Plane loads it: "foo.png" loads "foo.dds" when that file exists.
	 * @return The file, or empty when neither exists.
	 */
	std::filesystem::path ResolveThumbnailTexture(const std::filesystem::path &baseFolder, std::string_view reference);

	/**
	 * @class ThumbnailBuilder
	 * @brief Builds and renders thumbnails. It caches parsed assets, objects and textures, so one builder should
	 * serve a worker thread for many thumbnails (it is not thread-safe itself).
	 */
	class ThumbnailBuilder
	{
	public:
		/**
		 * @param resolver Finds library paths ("lib/...") and paths relative to a scenery pack (absolute paths are used
		 * as they are).
		 * @param textures Reads image files.
		 */
		ThumbnailBuilder(AssetResolver resolver, ThumbnailTextureLoader textures);
		~ThumbnailBuilder();

		/**
		 * @brief The 3D scene of an asset file (.obj, .fac, .for, .str, .agp) and how to frame it.
		 * @return False when the file can't be read, the asset draws nothing, or it is drawn flat (error says why).
		 */
		bool BuildScene(const std::filesystem::path &file, ThumbnailScene &scene, ThumbnailFraming &framing, std::string *error = nullptr);

		/* @brief Draws an asset file: images fitted, polygons and lines flat, the rest built and rendered. Output row 0 is the top. */
		bool Render(const std::filesystem::path &file, const ThumbnailOptions &options, RgbaImage &out, std::string *error = nullptr);

		/* @brief Adds an object's LOD 0 at rest, turned to a heading (true degrees) and moved to (x, y, z). */
		void AddObject(const Obj8Data &object, const std::filesystem::path &objectFile, const float position[3], float heading, ThumbnailScene &scene);

		/* @brief Adds an overlay build: its meshes, and its objects loaded from their files. */
		void AddOverlay(const OverlayBuild &build, ThumbnailScene &scene);

		/* @brief Adds WorldEditor's forest preview: a grid of the forest's trees (see the file comment). */
		void AddForest(const ForestAsset &forest, const std::filesystem::path &forestFile, ThumbnailScene &scene);

		/* @brief Drops the cached assets, objects and textures. */
		void ClearCache();

	private:
		int LoadTexture(const std::filesystem::path &file, ThumbnailScene &scene);
		const Obj8Data *GetObject(const std::filesystem::path &file);

		AssetResolver m_Resolver;
		ThumbnailTextureLoader m_Textures;
		std::unique_ptr<OverlayBuilder> m_Overlay;
		std::map<std::filesystem::path, std::unique_ptr<Obj8Data>> m_Objects;	// nullptr: couldn't be read
		std::map<std::filesystem::path, std::shared_ptr<const RgbaImage>> m_TextureCache;
		std::map<std::filesystem::path, int> m_SceneTextures;					// For the scene being built
		uint32_t m_TextureSize = 256;											// Texels the textures are read at (longer side)
	};

}

// -------------------------------------------------------

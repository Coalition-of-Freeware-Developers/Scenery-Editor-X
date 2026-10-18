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
 * xp_texture_loader.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <vulkan/vulkan.h>
#include <SceneryEditorX/renderer/vulkan/image_resource.h>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * Texture loading for X-Plane art.
	 *
	 * Orientation: X-Plane texture coordinates start at the bottom-left, and X-Plane's DDS files are stored
	 * bottom row first for that reason. PNGs decode top row first, so they are flipped on load; every texture
	 * then has row 0 at t = 0 and meshes keep their (s, t) coordinates unchanged.
	 *
	 * Lookup: like X-Plane, a reference to "foo.png" loads "foo.dds" when that file exists.
	 */

	/* @brief What a texture is used for (selects the fallback and the alpha scan). */
	enum class TextureUsage : uint8_t
	{
		Albedo,
		Normal,
		Lit,
		Data
	};

	/* @brief What the loader found out about a texture file. */
	struct TextureInfo
	{
		std::filesystem::path path;		// File that was loaded
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t mipCount = 0;
		VkFormat format = VK_FORMAT_UNDEFINED;
		bool fromDds = false;
		bool hasAlpha = false;			// Some texels have alpha below ~1 (drives alpha testing)
		std::string error;
	};

	/* @brief A decoded 8-bit RGBA image, row 0 at the bottom (X-Plane orientation). */
	struct CpuImage
	{
		uint32_t width = 0;
		uint32_t height = 0;
		std::vector<uint8_t> pixels;	// width * height * 4

		[[nodiscard]] bool IsValid() const { return width > 0 && height > 0 && pixels.size() == static_cast<size_t>(width) * height * 4; }
	};

	/**
	 * @struct DecodedTexture
	 * @brief The CPU half of a texture load: the file read and decoded, its mip chain built, waiting for Upload().
	 * Default-constructed it stands for "no texture" (Upload() returns nullptr with info as set).
	 */
	struct DecodedTexture
	{
		TextureInfo info;					// Also filled in on failure (error)
		std::vector<MipBytes> mips;			// Empty when decoding failed or the GPU cache already had the file
		std::string cacheKey;				// Empty for "no texture"
		std::string name;					// Debug name of the GPU image
		std::filesystem::path filePath;		// The image's resource file path (empty for generated maps)
		bool transparent = false;			// Flag the image transparent (albedo with alpha)
		bool cached = false;				// Already uploaded: Upload() returns the cached image
		bool valid = false;					// Upload() will return an image
		bool cacheFailure = false;			// Remember the failure so the file isn't decoded again
	};

	/**
	 * @class TextureLoader
	 * @brief Loads DDS (BC1-5, BC7, RGBA8) and PNG / JPG / TGA / BMP files into sampled GPU images, cached by path.
	 *
	 * A load has two halves: Decode() reads and decodes the file and builds the mip chain (any thread, no GPU work),
	 * Upload() creates the GPU image (main thread: it uploads through the immediate graphics queue). Load() does both.
	 */
	class TextureLoader
	{
	public:
		/**
		 * @brief Finds the file X-Plane would load for a texture reference: a .dds next to the named file wins.
		 * @param baseFolder Folder the reference is relative to (the OBJ's folder).
		 * @param reference Path as written in the art file.
		 * @return The file, or an empty path when neither the .dds nor the named file exists.
		 */
		static std::filesystem::path ResolveTexturePath(const std::filesystem::path &baseFolder, std::string_view reference);

		/**
		 * @brief Loads (or returns the cached) GPU texture for a file: Decode() then Upload(). Main thread.
		 * @param path File to load (see ResolveTexturePath()).
		 * @param usage What the texture is for.
		 * @param info Receives size, format and alpha information (optional).
		 * @return The texture, or nullptr when the file can't be read or decoded.
		 */
		static Ref<ImageResource> Load(const std::filesystem::path &path, TextureUsage usage, TextureInfo *info = nullptr);

		/**
		 * @brief Reads and decodes a texture file and builds its mip chain, without touching the GPU. Thread-safe.
		 * Skips the decoding when the GPU cache already has the file.
		 * @param path File to load (see ResolveTexturePath()); empty gives "no texture".
		 * @param usage What the texture is for.
		 */
		static DecodedTexture Decode(const std::filesystem::path &path, TextureUsage usage);

		/**
		 * @brief Creates the GPU image for a decoded texture and caches it (or returns the image already cached for the
		 * file, e.g. when two loads decoded it at once). Main thread. Consumes the texture's mips.
		 * @param texture From Decode() or DecodePackedMaterial().
		 * @param info Receives size, format and alpha information (optional).
		 * @return The texture, or nullptr when there is none or decoding failed.
		 */
		static Ref<ImageResource> Upload(DecodedTexture &texture, TextureInfo *info = nullptr);

		/**
		 * @brief Decodes a texture into 8-bit RGBA with row 0 at the bottom. Handles PNG / JPG / TGA / BMP and
		 * uncompressed DDS; block-compressed DDS files can't be decoded on the CPU.
		 */
		static bool DecodeRGBA(const std::filesystem::path &path, CpuImage &out, std::string *error = nullptr);

		/**
		 * @brief Builds the renderer's packed material map (R occlusion, G roughness, B metalness) from an X-Plane
		 * normal map: alpha is gloss (scaled by GLOBAL_specular), blue is metalness with NORMAL_METALNESS.
		 * Cached per (normal map, metalness flag, specular). Main thread: DecodePackedMaterial() then Upload().
		 */
		static Ref<ImageResource> LoadPackedMaterial(const std::filesystem::path &normalMapPath, bool normalMetalness, float specular);

		/* @brief The CPU half of LoadPackedMaterial(): decodes the normal map and builds the packed map. Thread-safe. */
		static DecodedTexture DecodePackedMaterial(const std::filesystem::path &normalMapPath, bool normalMetalness, float specular);

		/* @brief Releases every cached texture. Only safe when no material references them any more. */
		static void ClearCache();

		/* @brief Number of cached textures and their approximate GPU size in bytes. */
		static size_t GetCacheCount();
		static uint64_t GetCacheBytes();
	};

}

// -------------------------------------------------------

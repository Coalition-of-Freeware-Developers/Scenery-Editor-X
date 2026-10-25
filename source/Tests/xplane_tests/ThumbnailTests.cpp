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
 * ThumbnailTests.cpp
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#include <catch2/catch_all.hpp>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/xplane/xp_library.h>
#include <SceneryEditorX/xplane/xp_thumbnail.h>
#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// -------------------------------------------------------

namespace fs = std::filesystem;
using namespace SceneryEditorX::XPlane;

namespace
{
	void Put32(std::vector<uint8_t> &bytes, const size_t offset, const uint32_t value)
	{
		for (int i = 0; i < 4; ++i)
			bytes[offset + i] = static_cast<uint8_t>(value >> (8 * i));
	}

	/* @brief A DDS header: fourCC formats when fourCC is set, else masked pixels. */
	std::vector<uint8_t> DdsHeader(const uint32_t width, const uint32_t height, const uint32_t mips, const char *fourCC, const uint32_t bits = 0,
		const std::array<uint32_t, 4> masks = {}, const uint32_t pixelFlags = 0)
	{
		std::vector<uint8_t> bytes(128, 0);
		std::memcpy(bytes.data(), "DDS ", 4);
		Put32(bytes, 4, 124);
		Put32(bytes, 8, 0x1 | 0x2 | 0x4 | 0x1000 | (mips > 1 ? 0x20000 : 0));
		Put32(bytes, 12, height);
		Put32(bytes, 16, width);
		Put32(bytes, 28, mips);
		Put32(bytes, 76, 32);
		if (fourCC)
		{
			Put32(bytes, 80, 0x4);
			std::memcpy(&bytes[84], fourCC, 4);
		}
		else
		{
			Put32(bytes, 80, pixelFlags);
			Put32(bytes, 88, bits);
			for (int i = 0; i < 4; ++i)
				Put32(bytes, 92 + 4 * i, masks[static_cast<size_t>(i)]);
		}
		Put32(bytes, 108, 0x1000);
		return bytes;
	}

	void Append(std::vector<uint8_t> &bytes, const std::vector<uint8_t> &more)
	{
		bytes.insert(bytes.end(), more.begin(), more.end());
	}

	/* @brief A BC1 block of one 565 color. */
	std::vector<uint8_t> SolidBc1(const uint16_t color)
	{
		return { static_cast<uint8_t>(color), static_cast<uint8_t>(color >> 8), static_cast<uint8_t>(color), static_cast<uint8_t>(color >> 8), 0, 0, 0, 0 };
	}

	const uint8_t *Pixel(const RgbaImage &image, const uint32_t x, const uint32_t y)
	{
		return &image.pixels[(static_cast<size_t>(y) * image.width + x) * 4];
	}

	void WriteText(const fs::path &file, const std::string &text)
	{
		fs::create_directories(file.parent_path());
		std::ofstream out(file, std::ios::binary);
		out << text;
	}

	void WriteBytes(const fs::path &file, const std::vector<uint8_t> &bytes)
	{
		fs::create_directories(file.parent_path());
		std::ofstream out(file, std::ios::binary);
		out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
	}

	/* @brief Reads DDS files the way the editor does (other formats need the engine's decoder). */
	bool LoadDds(const fs::path &file, const uint32_t minSize, RgbaImage &out)
	{
		std::string bytes;
		if (!SceneryEditorX::VFS::ReadFile(file, bytes))
			return false;
		if (!DecodeDds(std::span(reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size()), minSize, out))
			return false;
		FlipRows(out);
		return true;
	}

	/* @brief An uncompressed PNG (stored deflate blocks), for looking at thumbnails while debugging. */
	void WritePng(const fs::path &file, const RgbaImage &image)
	{
		const auto crc = [](const std::vector<uint8_t> &data, const size_t start)
		{
			uint32_t c = 0xFFFFFFFFu;
			for (size_t i = start; i < data.size(); ++i)
			{
				c ^= data[i];
				for (int k = 0; k < 8; ++k)
					c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
			}
			return c ^ 0xFFFFFFFFu;
		};
		const auto be32 = [](std::vector<uint8_t> &out, const uint32_t v)
		{
			for (int i = 3; i >= 0; --i)
				out.push_back(static_cast<uint8_t>(v >> (8 * i)));
		};
		std::vector<uint8_t> raw;
		for (uint32_t y = 0; y < image.height; ++y)
		{
			raw.push_back(0);
			raw.insert(raw.end(), image.pixels.begin() + static_cast<ptrdiff_t>(y) * image.width * 4, image.pixels.begin() + static_cast<ptrdiff_t>(y + 1) * image.width * 4);
		}
		std::vector<uint8_t> zlib = { 0x78, 0x01 };
		for (size_t at = 0; at < raw.size(); at += 65535)
		{
			const uint16_t length = static_cast<uint16_t>(std::min<size_t>(65535, raw.size() - at));
			zlib.push_back(at + length >= raw.size() ? 1 : 0);
			zlib.push_back(static_cast<uint8_t>(length));
			zlib.push_back(static_cast<uint8_t>(length >> 8));
			zlib.push_back(static_cast<uint8_t>(~length));
			zlib.push_back(static_cast<uint8_t>(~length >> 8));
			zlib.insert(zlib.end(), raw.begin() + static_cast<ptrdiff_t>(at), raw.begin() + static_cast<ptrdiff_t>(at + length));
		}
		uint32_t a = 1, b = 0;
		for (const uint8_t byte : raw)
		{
			a = (a + byte) % 65521;
			b = (b + a) % 65521;
		}
		be32(zlib, (b << 16) | a);

		std::vector<uint8_t> png = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
		const auto chunk = [&](const char *type, const std::vector<uint8_t> &data)
		{
			be32(png, static_cast<uint32_t>(data.size()));
			const size_t start = png.size();
			png.insert(png.end(), type, type + 4);
			png.insert(png.end(), data.begin(), data.end());
			be32(png, crc(png, start));
		};
		std::vector<uint8_t> header;
		be32(header, image.width);
		be32(header, image.height);
		header.insert(header.end(), { 8, 6, 0, 0, 0 });
		chunk("IHDR", header);
		chunk("IDAT", zlib);
		chunk("IEND", {});
		WriteBytes(file, png);
	}

	/* @brief A quad from four corners (counter-clockwise), one color. */
	ThumbnailMesh Quad(const std::array<std::array<float, 3>, 4> &corners, const std::array<float, 3> &normal, const std::array<float, 4> &color, const bool draped = false)
	{
		ThumbnailMesh mesh;
		const float uvs[4][2] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		for (int i = 0; i < 4; ++i)
		{
			ThumbnailVertex vertex;
			std::memcpy(vertex.position, corners[static_cast<size_t>(i)].data(), sizeof(vertex.position));
			std::memcpy(vertex.normal, normal.data(), sizeof(vertex.normal));
			vertex.uv[0] = uvs[i][0];
			vertex.uv[1] = uvs[i][1];
			mesh.vertices.push_back(vertex);
		}
		mesh.indices = { 0, 1, 2, 0, 2, 3 };
		std::memcpy(mesh.color, color.data(), sizeof(mesh.color));
		mesh.draped = draped;
		return mesh;
	}

	ThumbnailMesh GroundQuad(const float half, const std::array<float, 4> &color, const float y = 0.0f, const bool draped = false)
	{
		return Quad({ { { -half, y, half }, { half, y, half }, { half, y, -half }, { -half, y, -half } } }, { 0.0f, 1.0f, 0.0f }, color, draped);
	}

	size_t CountOpaque(const RgbaImage &image)
	{
		size_t count = 0;
		for (size_t i = 3; i < image.pixels.size(); i += 4)
			count += image.pixels[i] > 128 ? 1 : 0;
		return count;
	}
}

// -------------------------------------------------------

TEST_CASE("DDS blocks decode the way Direct3D defines them", "[thumbnail][dds]")
{
	SECTION("BC1 with four colors")
	{
		std::vector<uint8_t> file = DdsHeader(4, 4, 1, "DXT1");
		Append(file, { 0x00, 0xF8, 0x1F, 0x00, 0xE4, 0x00, 0x00, 0x00 });	// Red, blue; texels 0..3 use codes 0..3
		RgbaImage image;
		REQUIRE(DecodeDds(file, 1, image));
		REQUIRE(image.width == 4);
		REQUIRE(image.height == 4);
		const uint8_t expected[4][4] = { { 255, 0, 0, 255 }, { 0, 0, 255, 255 }, { 170, 0, 85, 255 }, { 85, 0, 170, 255 } };
		for (uint32_t i = 0; i < 4; ++i)
		{
			for (int c = 0; c < 4; ++c)
				CHECK(Pixel(image, i, 0)[c] == expected[i][c]);
		}
		CHECK(Pixel(image, 0, 3)[0] == 255);
	}

	SECTION("BC1 with three colors and transparent black")
	{
		std::vector<uint8_t> file = DdsHeader(4, 4, 1, "DXT1");
		Append(file, { 0x1F, 0x00, 0x00, 0xF8, 0xE0, 0x00, 0x00, 0x00 });	// Blue < red: texels 2, 3 use codes 2, 3
		RgbaImage image;
		REQUIRE(DecodeDds(file, 1, image));
		CHECK(Pixel(image, 0, 0)[2] == 255);
		CHECK(Pixel(image, 2, 0)[0] == 127);
		CHECK(Pixel(image, 2, 0)[2] == 127);
		CHECK(Pixel(image, 3, 0)[3] == 0);
	}

	SECTION("BC2 explicit alpha")
	{
		std::vector<uint8_t> file = DdsHeader(4, 4, 1, "DXT3");
		Append(file, { 0x0F, 0x50, 0, 0, 0, 0, 0, 0 });
		Append(file, SolidBc1(0xF800));
		RgbaImage image;
		REQUIRE(DecodeDds(file, 1, image));
		CHECK(Pixel(image, 0, 0)[3] == 255);
		CHECK(Pixel(image, 1, 0)[3] == 0);
		CHECK(Pixel(image, 3, 0)[3] == 85);
		CHECK(Pixel(image, 0, 0)[0] == 255);
	}

	SECTION("BC3 interpolated alpha, both modes")
	{
		std::vector<uint8_t> file = DdsHeader(4, 4, 1, "DXT5");
		Append(file, { 255, 0, 0x88, 0x0E, 0, 0, 0, 0 });	// Codes 0, 1, 2, 7
		Append(file, SolidBc1(0xF800));
		RgbaImage image;
		REQUIRE(DecodeDds(file, 1, image));
		CHECK(Pixel(image, 0, 0)[3] == 255);
		CHECK(Pixel(image, 1, 0)[3] == 0);
		CHECK(Pixel(image, 2, 0)[3] == 219);
		CHECK(Pixel(image, 3, 0)[3] == 36);

		std::vector<uint8_t> six = DdsHeader(4, 4, 1, "DXT5");
		Append(six, { 0, 255, 0x8E, 0x0C, 0, 0, 0, 0 });	// a0 <= a1: codes 6, 1, 2, 6 -> 0, 255, 51, 0
		Append(six, SolidBc1(0xF800));
		REQUIRE(DecodeDds(six, 1, image));
		CHECK(Pixel(image, 0, 0)[3] == 0);
		CHECK(Pixel(image, 1, 0)[3] == 255);
		CHECK(Pixel(image, 2, 0)[3] == 51);
	}

	SECTION("Uncompressed BGRA through the channel masks")
	{
		std::vector<uint8_t> file = DdsHeader(2, 1, 1, nullptr, 32, { 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000 }, 0x41);
		Append(file, { 10, 20, 30, 40, 1, 2, 3, 4 });
		RgbaImage image;
		REQUIRE(DecodeDds(file, 1, image));
		CHECK(Pixel(image, 0, 0)[0] == 30);
		CHECK(Pixel(image, 0, 0)[1] == 20);
		CHECK(Pixel(image, 0, 0)[2] == 10);
		CHECK(Pixel(image, 0, 0)[3] == 40);
		CHECK(Pixel(image, 1, 0)[0] == 3);
	}

	SECTION("BC7, damaged and short files are refused")
	{
		std::vector<uint8_t> bc7 = DdsHeader(4, 4, 1, "DX10");
		bc7.resize(148, 0);
		Put32(bc7, 128, 98);
		bc7.resize(164, 0);
		RgbaImage image;
		std::string error;
		CHECK_FALSE(DecodeDds(bc7, 1, image, &error));
		CHECK(error.find("BC7") != std::string::npos);

		std::vector<uint8_t> shortFile = DdsHeader(8, 8, 1, "DXT1");
		Append(shortFile, SolidBc1(0xF800));	// One block of four
		CHECK_FALSE(DecodeDds(shortFile, 1, image));
		CHECK_FALSE(DecodeDds(std::vector<uint8_t>(64, 0), 1, image));
	}
}

TEST_CASE("DDS decoding picks the smallest mip a thumbnail still needs", "[thumbnail][dds]")
{
	// 8x8 red, 4x4 green, 2x2 blue, 1x1 white
	std::vector<uint8_t> file = DdsHeader(8, 8, 4, "DXT1");
	for (int i = 0; i < 4; ++i)
		Append(file, SolidBc1(0xF800));
	Append(file, SolidBc1(0x07E0));
	Append(file, SolidBc1(0x001F));
	Append(file, SolidBc1(0xFFFF));

	RgbaImage image;
	REQUIRE(DecodeDds(file, 4, image));
	CHECK(image.width == 4);
	CHECK(Pixel(image, 0, 0)[1] == 255);
	REQUIRE(DecodeDds(file, 100, image));
	CHECK(image.width == 8);
	CHECK(Pixel(image, 0, 0)[0] == 255);
	REQUIRE(DecodeDds(file, 1, image));
	CHECK(image.width == 1);
	CHECK(Pixel(image, 0, 0)[2] == 255);
	REQUIRE(DecodeDds(file, 3, image));
	CHECK(image.width == 4);

	// A mip chain cut short: the levels that are there
	file.resize(file.size() - 8);
	REQUIRE(DecodeDds(file, 1, image));
	CHECK(image.width == 2);
}

TEST_CASE("Thumbnail images are fitted, shrunk and flipped", "[thumbnail]")
{
	RgbaImage image;
	image.width = 4;
	image.height = 2;
	image.pixels.assign(32, 255);
	for (uint32_t x = 0; x < 4; ++x)
	{
		uint8_t *bottom = &image.pixels[x * 4];		// Row 0: the bottom, red
		bottom[1] = bottom[2] = 0;
		uint8_t *top = &image.pixels[(4 + x) * 4];	// Row 1: the top, blue
		top[0] = top[1] = 0;
	}

	RgbaImage fitted;
	REQUIRE(FitThumbnailImage(image, 8, fitted));
	REQUIRE(fitted.width == 8);
	REQUIRE(fitted.height == 8);
	CHECK(Pixel(fitted, 4, 0)[3] == 0);			// Letterboxed
	CHECK(Pixel(fitted, 4, 7)[3] == 0);
	CHECK(Pixel(fitted, 4, 2)[2] == 255);		// The top row first
	CHECK(Pixel(fitted, 4, 2)[0] == 0);
	CHECK(Pixel(fitted, 4, 5)[0] == 255);
	CHECK(Pixel(fitted, 4, 5)[3] == 255);

	RgbaImage flipped = image;
	FlipRows(flipped);
	CHECK(Pixel(flipped, 0, 0)[2] == 255);

	RgbaImage big;
	big.width = 64;
	big.height = 32;
	big.pixels.assign(static_cast<size_t>(64) * 32 * 4, 200);
	ShrinkImage(big, 16);
	CHECK(big.width == 16);
	CHECK(big.height == 8);
	CHECK(Pixel(big, 3, 3)[0] == 200);
}

TEST_CASE("Dark line paint is shown on a light background", "[thumbnail][render]")
{
	LineAsset line;
	line.scaleS = 2.0f;
	line.scaleT = 4.0f;
	line.segments.push_back({ 0, 0.25f, 0.5f, 0.75f });
	RgbaImage texture;
	texture.width = 4;
	texture.height = 1;
	texture.pixels.assign(16, 20);	// Nearly black, opaque
	for (size_t i = 3; i < texture.pixels.size(); i += 4)
		texture.pixels[i] = 255;

	RgbaImage image;
	REQUIRE(RenderLineThumbnail(line, texture, 32, image));
	CHECK(Pixel(image, 0, 0)[3] == 255);		// The background
	CHECK(Pixel(image, 0, 0)[0] > 200);
	CHECK(Pixel(image, 16, 16)[0] < 40);		// The paint: 0.5 x 2 / 4 = a quarter of the width
	CHECK(Pixel(image, 10, 16)[0] > 200);
}

TEST_CASE("The thumbnail renderer frames, depth-tests and blends", "[thumbnail][render]")
{
	ThumbnailOptions options;
	options.size = 64;

	SECTION("A ground square fills the frame from above")
	{
		ThumbnailScene scene;
		scene.meshes.push_back(GroundQuad(5.0f, { 1.0f, 0.0f, 0.0f, 1.0f }));
		options.framing = ThumbnailFraming::Ground;
		RgbaImage image;
		REQUIRE(RenderThumbnail(scene, options, image));
		REQUIRE(image.width == 64);
		const uint8_t *center = Pixel(image, 32, 32);
		CHECK(center[3] == 255);
		CHECK(center[0] > 150);
		CHECK(center[1] < 20);
		CHECK(Pixel(image, 0, 0)[3] == 0);
		// Framed: it spans most of the width
		size_t row = 0;
		for (uint32_t x = 0; x < 64; ++x)
			row += Pixel(image, x, 32)[3] > 128 ? 1 : 0;
		CHECK(row > 40);
	}

	SECTION("The nearer wall hides the farther one whatever the order")
	{
		// Walls facing south; the camera is south of them, so z = 3 is nearer than z = 0
		const auto wall = [](const float z, const std::array<float, 4> &color)
		{
			return Quad({ { { -2.0f, 0.0f, z }, { 2.0f, 0.0f, z }, { 2.0f, 3.0f, z }, { -2.0f, 3.0f, z } } }, { 0.0f, 0.0f, 1.0f }, color);
		};
		for (const bool nearFirst : { true, false })
		{
			ThumbnailScene scene;
			if (nearFirst)
			{
				scene.meshes.push_back(wall(3.0f, { 0.0f, 0.0f, 1.0f, 1.0f }));
				scene.meshes.push_back(wall(0.0f, { 1.0f, 0.0f, 0.0f, 1.0f }));
			}
			else
			{
				scene.meshes.push_back(wall(0.0f, { 1.0f, 0.0f, 0.0f, 1.0f }));
				scene.meshes.push_back(wall(3.0f, { 0.0f, 0.0f, 1.0f, 1.0f }));
			}
			RgbaImage image;
			REQUIRE(RenderThumbnail(scene, options, image));
			const uint8_t *center = Pixel(image, 32, 32);
			CHECK(center[2] > 100);
			CHECK(center[0] < 20);
		}
	}

	SECTION("Draped meshes paint over each other in order, under the 3D ones")
	{
		ThumbnailScene scene;
		scene.meshes.push_back(GroundQuad(5.0f, { 1.0f, 0.0f, 0.0f, 1.0f }, 0.0f, true));
		scene.meshes.push_back(GroundQuad(5.0f, { 0.0f, 1.0f, 0.0f, 1.0f }, -0.5f, true));	// Lower, but later
		options.framing = ThumbnailFraming::Ground;
		RgbaImage image;
		REQUIRE(RenderThumbnail(scene, options, image));
		CHECK(Pixel(image, 32, 32)[1] > 100);
		CHECK(Pixel(image, 32, 32)[0] < 20);
	}

	SECTION("Texels under the cutoff aren't drawn")
	{
		ThumbnailScene scene;
		RgbaImage texture;
		texture.width = 4;
		texture.height = 1;
		texture.pixels = { 255, 255, 255, 0, 255, 255, 255, 0, 255, 255, 255, 255, 255, 255, 255, 255 };
		scene.textures.push_back(texture);
		ThumbnailMesh mesh = GroundQuad(5.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
		mesh.texture = 0;
		scene.meshes.push_back(mesh);
		scene.meshes.push_back(GroundQuad(0.01f, { 1.0f, 1.0f, 1.0f, 1.0f }, 0.0f));	// Keeps the framing when half is cut
		options.framing = ThumbnailFraming::Ground;
		RgbaImage image;
		REQUIRE(RenderThumbnail(scene, options, image));
		CHECK(Pixel(image, 18, 32)[3] == 0);		// West half: alpha 0
		CHECK(Pixel(image, 46, 32)[3] == 255);
	}

	SECTION("Nothing to draw")
	{
		ThumbnailScene scene;
		RgbaImage image;
		CHECK_FALSE(RenderThumbnail(scene, options, image));
	}
}

TEST_CASE("Thumbnail builder draws each kind of asset as WorldEditor previews it", "[thumbnail][builder]")
{
	const fs::path root = fs::temp_directory_path() / "sedx_thumbnail_test";
	fs::remove_all(root);
	std::vector<uint8_t> texture = DdsHeader(4, 4, 1, "DXT1");
	Append(texture, SolidBc1(0x07E0));	// Green
	WriteBytes(root / "tex.dds", texture);

	// A wall 2 m wide and 2 m tall facing south, its texture named .png as X-Plane art often is
	WriteText(root / "wall.obj", "I\n800\nOBJ\n\nTEXTURE tex.png\nPOINT_COUNTS 4 0 0 6\n"
		"VT -1 0 0 0 0 1 0 0\nVT 1 0 0 0 0 1 1 0\nVT 1 2 0 0 0 1 1 1\nVT -1 2 0 0 0 1 0 1\n"
		"IDX 0\nIDX 1\nIDX 2\nIDX 0\nIDX 2\nIDX 3\n\nTRIS 0 6\n");
	WriteText(root / "paving.pol", "A\n850\nDRAPED_POLYGON\n\nTEXTURE tex.dds\nSCALE 10 10\n");
	WriteText(root / "wide.pol", "A\n850\nDRAPED_POLYGON\n\nTEXTURE tex.dds\nSCALE 20 10\n");
	WriteText(root / "line.lin", "A\n850\nLINE_PAINT\n\nTEXTURE tex.dds\nTEX_WIDTH 4\nTEX_HEIGHT 4\nSCALE 4 8\nS_OFFSET 0 1 2 3\n");
	WriteText(root / "trees.for", "A\n800\nFOREST\n\nTEXTURE tex.dds\nSCALE_X 4\nSCALE_Y 4\nSPACING 10 10\nRANDOM 0 0\nTREE 0 0 4 4 2 100 8 8 2 0\n");
	WriteText(root / "walls.str", "A\n800\nOBJECT_STRING\n\nOBJECT 0 0 wall.obj\nOBJECT 0 0 missing.obj\n");
	WriteText(root / "notes.txt", "no thumbnail");

	CHECK(CanMakeThumbnail("lib/a/b.obj"));
	CHECK(CanMakeThumbnail("C:/x/Y.FAC"));
	CHECK(CanMakeThumbnail("tex.png"));
	CHECK_FALSE(CanMakeThumbnail("roads.net"));
	CHECK_FALSE(CanMakeThumbnail("folder.v2/readme"));
	CHECK(fs::equivalent(ResolveThumbnailTexture(root, "tex.png"), root / "tex.dds"));
	CHECK(ResolveThumbnailTexture(root, "missing.png").empty());

	ThumbnailBuilder builder(nullptr, LoadDds);
	ThumbnailOptions options;
	options.size = 64;

	RgbaImage image;
	std::string error;
	REQUIRE(builder.Render(root / "wall.obj", options, image, &error));
	const uint8_t *center = Pixel(image, 32, 32);
	CHECK(center[3] == 255);
	CHECK(center[1] > center[0] + 50);
	CHECK(center[1] > center[2] + 50);

	// Polygons: one texture repeat, flat, in the aspect of SCALE
	ThumbnailScene scene;
	ThumbnailFraming framing = ThumbnailFraming::Object;
	CHECK_FALSE(builder.BuildScene(root / "paving.pol", scene, framing, &error));
	REQUIRE(builder.Render(root / "paving.pol", options, image, &error));
	CHECK(CountOpaque(image) == 64 * 64);
	REQUIRE(builder.Render(root / "wide.pol", options, image, &error));
	CHECK(Pixel(image, 32, 4)[3] == 0);
	CHECK(Pixel(image, 32, 32)[1] == 255);

	// Lines: the S_OFFSET row upright, a quarter of the repeat's length wide (4 m of 8 m across a 0.5 wide row)
	REQUIRE(builder.Render(root / "line.lin", options, image, &error));
	CHECK(Pixel(image, 32, 32)[1] == 255);
	CHECK(Pixel(image, 32, 2)[3] == 255);
	CHECK(Pixel(image, 26, 32)[3] == 255);
	CHECK(Pixel(image, 21, 32)[3] == 0);
	CHECK(Pixel(image, 43, 32)[3] == 0);

	// Forests: a 3 x 3 grid of one kind of tree, two crossed quads each
	REQUIRE(builder.BuildScene(root / "trees.for", scene, framing, &error));
	REQUIRE(scene.meshes.size() == 1);
	CHECK(scene.meshes[0].vertices.size() == 9 * 2 * 4);
	REQUIRE(builder.Render(root / "trees.for", options, image, &error));
	CHECK(CountOpaque(image) > 64 * 64 / 10);

	// Strings: the first object
	REQUIRE(builder.BuildScene(root / "walls.str", scene, framing, &error));
	REQUIRE(scene.meshes.size() == 1);
	CHECK(scene.meshes[0].vertices.size() == 4);

	REQUIRE(builder.Render(root / "tex.dds", options, image, &error));
	CHECK(Pixel(image, 32, 32)[1] == 255);

	CHECK_FALSE(builder.Render(root / "notes.txt", options, image, &error));
	CHECK_FALSE(builder.Render(root / "missing.obj", options, image, &error));
	fs::remove_all(root);
}

TEST_CASE("Thumbnails of X-Plane 12's default library assets", "[thumbnail][xplane-install]")
{
	const std::vector<fs::path> installs = SceneryEditorX::VFS::FindXPlaneInstalls(12);
	if (installs.empty())
	{
		WARN("No X-Plane 12 install found (x-plane_install_12.txt): skipped");
		return;
	}
	const fs::path scenery = installs.front() / "Resources" / "default scenery";
	if (!fs::exists(scenery))
	{
		WARN("No Resources/default scenery: skipped");
		return;
	}

	// The default libraries, for the library paths the assets name
	std::vector<LibraryPackage> packages;
	for (const fs::directory_entry &entry : fs::directory_iterator(scenery))
	{
		if (!fs::exists(entry.path() / "library.txt"))
			continue;
		LibraryPackage package;
		package.name = entry.path().filename().string();
		package.path = entry.path();
		package.hasLibrary = true;
		package.priority = static_cast<int>(packages.size());
		packages.push_back(package);
	}
	auto index = std::make_shared<LibraryIndex>();
	index->Build(packages, LibraryIndex::BuildOptions{ false });
	ThumbnailBuilder builder([index](const std::string_view path) { return index->Resolve(path); }, LoadDds);

	const char *dump = std::getenv("SEDX_THUMBNAIL_DUMP");
	const fs::path samples[] = {
		"airport scenery/Ramp_Equipment/Air_Start_1.obj",
		"airport scenery/Common_Elements/barriers/apt_barricade_2.fac",
		"airport scenery/Common_Elements/barriers/apt_barricade_1_mixed.str",
		"airport scenery/ground/pavement/mosaic_01.pol",
		"airport scenery/ground/water/shore_grass_1_2m.lin",
		"airport scenery/Ramp_Equipment/400cm_Jetway_2.agp",
		"1200 forests/fal/AG_temp_1.for",
	};
	ThumbnailOptions options;
	options.size = 128;
	for (const fs::path &sample : samples)
	{
		const fs::path file = scenery / sample;
		if (!fs::exists(file))
		{
			WARN("Not in this install: " << sample.generic_string());
			continue;
		}
		INFO(sample.generic_string());
		RgbaImage image;
		std::string error;
		CHECK(builder.Render(file, options, image, &error));
		INFO(error);
		CHECK(CountOpaque(image) > 128 * 128 / 50);
		if (dump && image.IsValid())
			WritePng(fs::path(dump) / (sample.stem().string() + ".png"), image);
	}
}

// -------------------------------------------------------

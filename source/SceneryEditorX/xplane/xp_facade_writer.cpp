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
 * xp_facade_writer.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "xp_facade_writer.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <SceneryEditorX/filesystem/vfs.h>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		/**
		 * @brief The number to write for a value the parser derives from it (pixels times 1 / TEX_SIZE, meters over a
		 * texture span): a round one when it reads back exactly, else the float nearest the estimate that does.
		 */
		template<typename Reads>
		float FindPrintable(const double estimate, Reads &&reads)
		{
			if (!std::isfinite(estimate))
				return 0.0f;
			for (const double step : { 1.0, 10.0, 100.0, 1000.0, 10000.0, 100000.0 })
			{
				const float candidate = static_cast<float>(std::round(estimate * step) / step);
				if (reads(candidate))
					return candidate;
			}
			const float center = static_cast<float>(estimate);
			float up = center, down = center;
			for (int i = 0; i < 256; ++i)
			{
				if (reads(up))
					return up;
				if (reads(down))
					return down;
				up = std::nextafter(up, std::numeric_limits<float>::infinity());
				down = std::nextafter(down, -std::numeric_limits<float>::infinity());
			}
			return center;
		}

		/* @brief A normalized texture coordinate in a TEX_SIZE's pixels (the parser reads value * (1 / size)); itself when size is 0. */
		float ToPixels(const float normalized, const float size)
		{
			if (size <= 0.0f)
				return normalized;
			const float scale = 1.0f / size;
			return FindPrintable(static_cast<double>(normalized) * size, [&](const float pixels) { return pixels * scale == normalized; });
		}

		class FacadeWriter
		{
		public:
			explicit FacadeWriter(const FacadeAsset &asset) : m_Asset(asset) {}

			std::string Write()
			{
				m_Out.clear();
				Line("A");
				Line(m_Asset.IsType2() ? "1000" : "800");
				Line("FACADE");
				Line("");
				WriteSwitches();
				if (m_Asset.IsType2())
				{
					WriteScrapers();
					WriteShaders();
					if (m_Asset.roofScaleS != 1.0f || m_Asset.roofScaleT != 1.0f)
						Line("ROOF_SCALE " + N(m_Asset.roofScaleS) + " " + N(m_Asset.roofScaleT));
					if (!m_Asset.objects.empty())
					{
						Line("");
						for (const std::string &object : m_Asset.objects)
							Line("OBJ " + object);
					}
					for (const FacadeAsset::Floor &floor : m_Asset.floors)
						WriteFloor(floor);
				}
				else
				{
					WriteShaders();
					Line("");
					Line("LOD 0 " + N(m_Asset.lodFar));
					WriteScrapers();
					WriteType1Roof();
					for (const FacadeAsset::Wall &wall : m_Asset.walls)
						WriteType1Wall(wall);
				}
				return std::move(m_Out);
			}

		private:
			static std::string N(const float value) { return FormatFacadeNumber(value); }

			void Line(const std::string &text)
			{
				m_Out += text;
				m_Out += '\n';
			}

			void WriteSwitches()
			{
				const FacadeAsset &a = m_Asset;
				Line(a.ring ? "RING 1" : "RING 0");
				if (a.graded)
					Line("GRADED");
				if (!a.layerGroup.empty())
					Line("LAYER_GROUP " + a.layerGroup + " " + std::to_string(a.layerOffset));
				if (!a.drapedLayerGroup.empty())
					Line("LAYER_GROUP_DRAPED " + a.drapedLayerGroup + " " + std::to_string(a.drapedLayerOffset));
				if (a.IsType2())
				{
					if (a.noRoofMesh)
						Line("NO_ROOF_MESH");
					if (a.noWallMesh)
						Line("NO_WALL_MESH");
					return;
				}
				if (a.doubled)
					Line("DOUBLED 1");
				if (a.floorsMin != 1)
					Line("FLOORS_MIN " + std::to_string(a.floorsMin));
				if (a.floorsMax != 999)
					Line("FLOORS_MAX " + std::to_string(a.floorsMax));
				if (!a.hardWall.empty())
					Line("HARD_WALL " + a.hardWall);
				if (!a.hardRoof.empty())
					Line("HARD_ROOF " + a.hardRoof);
			}

			void WriteScrapers()
			{
				for (const FacadeAsset::Scraper &scraper : m_Asset.scrapers)
				{
					Line("FACADE_SCRAPER " + N(scraper.minAgl) + " " + N(scraper.maxAgl) + " " + N(scraper.step) + " " + std::to_string(scraper.floors));
					for (const std::string &row : scraper.rows)
						Line(row);
				}
			}

			void WriteShaders()
			{
				const FacadeAsset &a = m_Asset;
				Line("");
				Line("SHADER_WALL");
				if (!a.wallTexture.empty())
					Line("TEXTURE " + a.wallTexture);
				if (!a.wallTextureNormal.empty())
					Line("TEXTURE_NORMAL " + N(a.wallTextureNormalScale) + " " + a.wallTextureNormal);
				if (a.twoSided)
					Line("TWO_SIDED 1");
				if (a.noBlendCutoff >= 0.0f)
					Line("NO_BLEND " + N(a.noBlendCutoff));
				for (const std::string &row : a.wallShaderRows)
					Line(row);

				if (a.roofTexture.empty() && a.roofTextureNormal.empty() && a.roofShaderRows.empty())
					return;
				Line("");
				Line("SHADER_ROOF");
				if (!a.roofTexture.empty())
					Line("TEXTURE " + a.roofTexture);
				if (!a.roofTextureNormal.empty())
					Line("TEXTURE_NORMAL " + N(a.roofTextureNormalScale) + " " + a.roofTextureNormal);
				for (const std::string &row : a.roofShaderRows)
					Line(row);
			}

			/* @brief TEX_SIZE when the rows that follow use another one than the one in effect ("1 1": fractions again). */
			void TextureSize(const float width, const float height)
			{
				if (width == m_TexWidth && height == m_TexHeight)
					return;
				m_TexWidth = width;
				m_TexHeight = height;
				const bool fractions = width <= 0.0f && height <= 0.0f;
				Line("TEX_SIZE " + (fractions ? std::string("1 1") : N(width > 0.0f ? width : 1.0f) + " " + N(height > 0.0f ? height : 1.0f)));
			}

			void WriteType1Roof()
			{
				const FacadeAsset &a = m_Asset;
				if (a.roofUv.empty())
					return;
				TextureSize(a.roofTextureWidth, a.roofTextureHeight);
				const float w = a.roofTextureWidth, h = a.roofTextureHeight;
				if (a.roofScaled && a.roofUv.size() == 4)
				{
					// ROOF_SCALE s1 t1 s_center t_center s2 t2 size_x size_y (the parser derives meters per repeat from the sizes)
					const float s1 = a.roofUv[0].first, t1 = a.roofUv[0].second, s2 = a.roofUv[2].first, t2 = a.roofUv[2].second;
					const float spanS = s2 - s1, spanT = t2 - t1;
					const float sizeX = FindPrintable(static_cast<double>(a.roofScaleS) * spanS, [&](const float c) { return c / spanS == a.roofScaleS; });
					const float sizeY = FindPrintable(static_cast<double>(a.roofScaleT) * spanT, [&](const float c) { return c / spanT == a.roofScaleT; });
					Line("ROOF_SCALE " + N(ToPixels(s1, w)) + " " + N(ToPixels(t1, h)) + " " + N(ToPixels(a.roofCenter.first, w)) + " " +
						N(ToPixels(a.roofCenter.second, h)) + " " + N(ToPixels(s2, w)) + " " + N(ToPixels(t2, h)) + " " + N(sizeX) + " " + N(sizeY));
					return;
				}
				for (const auto &[s, t] : a.roofUv)
					Line("ROOF " + N(ToPixels(s, w)) + " " + N(ToPixels(t, h)));
			}

			static std::string Filter(const FacadeAsset::WallFilter &filter)
			{
				return N(filter.minWidth) + " " + N(filter.maxWidth) + " " + N(filter.minHeading) + " " + N(filter.maxHeading);
			}

			void WriteFilters(const std::vector<FacadeAsset::WallFilter> &filters, const FacadeAsset::WallFilter &own, const std::string &name)
			{
				Line("WALL " + Filter(filters.empty() ? own : filters.front()) + (name.empty() ? std::string() : " " + name));
				for (size_t i = 1; i < filters.size(); ++i)
					Line("WALL_RULE " + Filter(filters[i]));
			}

			void WriteType1Wall(const FacadeAsset::Wall &wall)
			{
				Line("");
				TextureSize(wall.textureWidth, wall.textureHeight);
				WriteFilters(wall.filters, { wall.minWidth, wall.maxWidth, 0.0f, 0.0f }, wall.name);
				Line("SCALE " + N(wall.scaleS) + " " + N(wall.scaleT));
				if (wall.roofSlope != 0.0f || wall.slant)
					Line("ROOF_SLOPE " + N(wall.roofSlope) + (wall.slant ? " SLANT" : ""));
				if (wall.basement != 0.0f)
					Line("BASEMENT_DEPTH " + N(ToPixels(wall.basement, wall.textureHeight)));
				const auto ranges = [&](const char *key, const std::vector<std::pair<float, float>> &list, const float size)
				{
					for (const auto &[from, to] : list)
						Line(std::string(key) + " " + N(ToPixels(from, size)) + " " + N(ToPixels(to, size)));
				};
				ranges("BOTTOM", wall.bottomFloors, wall.textureHeight);
				ranges("MIDDLE", wall.middleFloors, wall.textureHeight);
				ranges("TOP", wall.topFloors, wall.textureHeight);
				ranges("LEFT", wall.leftPanels, wall.textureWidth);
				ranges("CENTER", wall.centerPanels, wall.textureWidth);
				ranges("RIGHT", wall.rightPanels, wall.textureWidth);
			}

			static std::string ShowRange(const int lo, const int hi)
			{
				return lo == 0 && hi == 0 ? std::string() : " " + std::to_string(lo) + " " + std::to_string(hi);
			}

			void WriteSegment(const char *key, const size_t index, const FacadeAsset::Segment &segment)
			{
				Line(std::string(key) + " " + std::to_string(index));
				for (const FacadeAsset::Mesh &mesh : segment.meshes)
				{
					const size_t vertices = mesh.positions.size() / 3;
					Line("MESH " + std::to_string(mesh.group) + " " + N(mesh.lodFar) + " " + std::to_string(mesh.cuts) + " " + std::to_string(vertices) + " " +
						std::to_string(mesh.indices.size()));
					// Meshes are most of a type 2 facade: numbers go straight into the text
					const auto at = [](const std::vector<float> &list, const size_t i, const float fallback) { return i < list.size() ? list[i] : fallback; };
					for (size_t v = 0; v < vertices; ++v)
					{
						m_Out += "VERTEX";
						for (const float value : { mesh.positions[v * 3], mesh.positions[v * 3 + 1], mesh.positions[v * 3 + 2], at(mesh.normals, v * 3, 0.0f),
							at(mesh.normals, v * 3 + 1, 1.0f), at(mesh.normals, v * 3 + 2, 0.0f), at(mesh.uvs, v * 2, 0.0f), at(mesh.uvs, v * 2 + 1, 0.0f) })
						{
							m_Out += ' ';
							AppendFacadeNumber(m_Out, value);
						}
						m_Out += '\n';
					}
					// Two triangles per row, as X-Plane's exporters write them
					for (size_t i = 0; i < mesh.indices.size(); i += 6)
					{
						m_Out += "IDX";
						for (size_t j = i; j < std::min(i + 6, mesh.indices.size()); ++j)
						{
							char buffer[16];
							const auto [end, ec] = std::to_chars(buffer, buffer + sizeof(buffer), mesh.indices[j]);
							m_Out += ' ';
							m_Out.append(buffer, end);
						}
						m_Out += '\n';
					}
				}
				for (const FacadeAsset::Attachment &attachment : segment.attachments)
				{
					Line(std::string(attachment.draped ? "ATTACH_DRAPED " : "ATTACH_GRADED ") + std::to_string(attachment.object) + " " + N(attachment.x) + " " +
						N(attachment.y) + " " + N(attachment.z) + " " + N(attachment.heading) + ShowRange(attachment.showLo, attachment.showHi));
				}
			}

			void WriteFloor(const FacadeAsset::Floor &floor)
			{
				Line("");
				Line(floor.name.empty() ? std::string("FLOOR") : "FLOOR " + floor.name);
				for (const FacadeAsset::Roof &roof : floor.roofs)
				{
					// One height per row: TWO_SIDED_ROOF and the roof objects apply to the roof above them
					Line("ROOF_HEIGHT " + N(roof.height));
					if (roof.twoSided)
						Line("TWO_SIDED_ROOF");
					for (const FacadeAsset::RoofObject &object : roof.objects)
					{
						if (object.heading != 0.0f)
							Line("ROOF_OBJ_HEADING " + std::to_string(object.object) + " " + N(object.s) + " " + N(object.t) + " " + N(object.heading) + ShowRange(object.showLo, object.showHi));
						else
							Line("ROOF_OBJ " + std::to_string(object.object) + " " + N(object.s) + " " + N(object.t) + ShowRange(object.showLo, object.showHi));
					}
				}
				if (!floor.hardRoof.empty())
					Line("HARD_ROOF " + floor.hardRoof);
				for (size_t i = 0; i < floor.segments.size(); ++i)
					WriteSegment("SEGMENT", i, floor.segments[i]);
				for (size_t i = 0; i < floor.curvedSegments.size(); ++i)
					WriteSegment("SEGMENT_CURVED", i, floor.curvedSegments[i]);
				for (const FacadeAsset::WallRule &wall : floor.walls)
				{
					WriteFilters(wall.filters, { wall.minWidth, wall.maxWidth, wall.minHeading, wall.maxHeading }, wall.name);
					for (const std::vector<int> &spelling : wall.spellings)
					{
						std::string row = "SPELLING";
						for (const int segment : spelling)
							row += " " + std::to_string(segment);
						Line(row);
					}
				}
			}

			const FacadeAsset &m_Asset;
			std::string m_Out;
			float m_TexWidth = 0.0f;	// The TEX_SIZE in effect (0: fractions)
			float m_TexHeight = 0.0f;
		};
	}

	// -------------------------------------------------------

	void AppendFacadeNumber(std::string &out, const float value)
	{
		char buffer[64];
		const auto [end, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::fixed);
		if (!std::isfinite(value) || value == 0.0f || ec != std::errc())
			out += '0';
		else
			out.append(buffer, end);
	}

	std::string FormatFacadeNumber(const float value)
	{
		std::string out;
		AppendFacadeNumber(out, value);
		return out;
	}

	std::string WriteFacadeAsset(const FacadeAsset &asset)
	{
		return FacadeWriter(asset).Write();
	}

	bool SaveFacadeAsset(const std::filesystem::path &path, const FacadeAsset &asset, std::string *error)
	{
		return VFS::WriteFileAtomic(path, WriteFacadeAsset(asset), error);
	}
}

// -------------------------------------------------------

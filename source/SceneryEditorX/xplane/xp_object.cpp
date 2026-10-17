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
 * xp_object.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "xp_object.h"
#include <chrono>
#include <cmath>
#include <unordered_map>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/vulkan/enums.h>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		constexpr const char *LOG_TAG = "XPlaneObject";

		/*
		 * OBJ8 triangles are front-facing when their vertices run clockwise; the renderer's pipelines use
		 * counter-clockwise front faces (see the renderer GPU conventions), so every triangle is re-wound.
		 */
		constexpr bool REVERSE_WINDING = true;

		std::unordered_map<std::string, Ref<ObjectAsset>> s_Cache;
		bool s_LitVisible = false;

		std::string CacheKey(const std::filesystem::path &path)
		{
			std::string key = std::filesystem::absolute(path).lexically_normal().string();
			for (char &c : key)
			{
				if (c >= 'A' && c <= 'Z')
					c = static_cast<char>(c - 'A' + 'a');
			}
			return key;
		}

		/* @brief Per-vertex tangents from UV derivatives (s right, t up), orthogonalized against the normal. */
		void ComputeTangents(std::vector<Vertex_PosTexNorTan> &vertices, const std::vector<uint32_t> &indices)
		{
			std::vector<float> accum(vertices.size() * 3, 0.0f);
			for (size_t i = 0; i + 2 < indices.size(); i += 3)
			{
				const Vertex_PosTexNorTan &v0 = vertices[indices[i]];
				const Vertex_PosTexNorTan &v1 = vertices[indices[i + 1]];
				const Vertex_PosTexNorTan &v2 = vertices[indices[i + 2]];

				const float e1[3] = { v1.pos[0] - v0.pos[0], v1.pos[1] - v0.pos[1], v1.pos[2] - v0.pos[2] };
				const float e2[3] = { v2.pos[0] - v0.pos[0], v2.pos[1] - v0.pos[1], v2.pos[2] - v0.pos[2] };
				const float du1 = v1.tex[0] - v0.tex[0], dv1 = v1.tex[1] - v0.tex[1];
				const float du2 = v2.tex[0] - v0.tex[0], dv2 = v2.tex[1] - v0.tex[1];
				const float det = du1 * dv2 - du2 * dv1;
				if (std::abs(det) < 1e-12f)
					continue;

				const float r = 1.0f / det;
				const float t[3] = { (e1[0] * dv2 - e2[0] * dv1) * r, (e1[1] * dv2 - e2[1] * dv1) * r, (e1[2] * dv2 - e2[2] * dv1) * r };
				for (int k = 0; k < 3; ++k)
				{
					for (int c = 0; c < 3; ++c)
						accum[static_cast<size_t>(indices[i + k]) * 3 + c] += t[c];
				}
			}

			for (size_t v = 0; v < vertices.size(); ++v)
			{
				const float *n = vertices[v].nor;
				float t[3] = { accum[v * 3], accum[v * 3 + 1], accum[v * 3 + 2] };
				const float d = t[0] * n[0] + t[1] * n[1] + t[2] * n[2];
				for (int c = 0; c < 3; ++c)
					t[c] -= n[c] * d;

				float length = std::sqrt(t[0] * t[0] + t[1] * t[1] + t[2] * t[2]);
				if (length < 1e-8f)
				{
					// No usable UV gradient: any vector perpendicular to the normal.
					const float axis[3] = { std::abs(n[0]) < 0.9f ? 1.0f : 0.0f, std::abs(n[0]) < 0.9f ? 0.0f : 1.0f, 0.0f };
					const float dd = axis[0] * n[0] + axis[1] * n[1] + axis[2] * n[2];
					for (int c = 0; c < 3; ++c)
						t[c] = axis[c] - n[c] * dd;
					length = std::sqrt(t[0] * t[0] + t[1] * t[1] + t[2] * t[2]);
				}

				for (int c = 0; c < 3; ++c)
					vertices[v].tan[c] = length > 1e-8f ? t[c] / length : (c == 0 ? 1.0f : 0.0f);
			}
		}
	}

	// -------------------------------------------------------

	Ref<ObjectAsset> ObjectAsset::Load(const std::filesystem::path &path, std::string *error)
	{
		const std::string key = CacheKey(path);
		if (const auto it = s_Cache.find(key); it != s_Cache.end())
			return it->second;

		Ref<ObjectAsset> asset = CreateRef<ObjectAsset>();
		if (!asset->Build(path, error))
			return nullptr;

		s_Cache.emplace(key, asset);
		return asset;
	}

	std::vector<Ref<ObjectAsset>> ObjectAsset::GetLoaded()
	{
		std::vector<Ref<ObjectAsset>> assets;
		assets.reserve(s_Cache.size());
		for (const auto &[key, asset] : s_Cache)
			assets.push_back(asset);
		return assets;
	}

	void ObjectAsset::ClearCache()
	{
		s_Cache.clear();
	}

	void ObjectAsset::SetLitTexturesVisible(const bool visible)
	{
		if (s_LitVisible == visible)
			return;

		s_LitVisible = visible;
		for (const auto &[key, asset] : s_Cache)
			asset->ApplyLit(visible);
	}

	bool ObjectAsset::AreLitTexturesVisible()
	{
		return s_LitVisible;
	}

	ObjectAsset::~ObjectAsset() = default;

	// -------------------------------------------------------

	bool ObjectAsset::Build(const std::filesystem::path &path, std::string *error)
	{
		const auto start = std::chrono::steady_clock::now();
		m_Path = path;

		std::string parseError;
		if (!LoadObj8(path, m_Data, &parseError))
		{
			SEDX_CORE_ERROR_TAG(LOG_TAG, "Failed to load '{}': {}", path.string(), parseError);
			if (error)
				*error = parseError;
			return false;
		}

		for (const std::string &warning : m_Data.warnings)
			SEDX_CORE_WARN_TAG(LOG_TAG, "{}: {}", path.filename().string(), warning);

		if (m_Data.batches.empty())
		{
			const std::string message = "The object has no triangles to draw";
			SEDX_CORE_WARN_TAG(LOG_TAG, "{}: {}", path.string(), message);
			if (error)
				*error = message;
			return false;
		}

		LoadTextures();

		m_Mesh = CreateRef<Mesh>();
		m_Mesh->SetFlags(0); // keep X-Plane's units and geometry: no normalization, optimization or generated LODs
		m_Mesh->SetResourceName(path.filename().string());
		m_Mesh->SetResourceFilePath(path.string());

		std::vector<int32_t> remap(m_Data.vertices.size(), -1);
		std::vector<uint32_t> touched;
		bool firstVertex = true;

		for (const Obj8Batch &batch : m_Data.batches)
		{
			std::vector<Vertex_PosTexNorTan> vertices;
			std::vector<uint32_t> indices;
			indices.reserve(batch.GetIndexCount());

			for (const Obj8Batch::Range &range : batch.ranges)
			{
				for (uint32_t i = range.offset; i + 2 < range.offset + range.count; i += 3)
				{
					uint32_t triangle[3] = { m_Data.indices[i], m_Data.indices[i + 1], m_Data.indices[i + 2] };
					if constexpr (REVERSE_WINDING)
						std::swap(triangle[1], triangle[2]);

					for (const uint32_t source : triangle)
					{
						if (remap[source] < 0)
						{
							const Obj8Vertex &v = m_Data.vertices[source];
							Vertex_PosTexNorTan out;
							for (int c = 0; c < 3; ++c)
							{
								out.pos[c] = v.position[c];
								out.nor[c] = v.normal[c];

								m_BoundsMin[c] = firstVertex ? v.position[c] : std::min(m_BoundsMin[c], v.position[c]);
								m_BoundsMax[c] = firstVertex ? v.position[c] : std::max(m_BoundsMax[c], v.position[c]);
							}
							firstVertex = false;

							const float length = std::sqrt(out.nor[0] * out.nor[0] + out.nor[1] * out.nor[1] + out.nor[2] * out.nor[2]);
							if (length > 1e-8f)
							{
								for (float &component : out.nor)
									component /= length;
							}
							else
							{
								out.nor[0] = 0.0f;
								out.nor[1] = 1.0f;
								out.nor[2] = 0.0f;
							}

							// (s, t) stay as written: textures are stored with row 0 at t = 0 (see xp_texture_loader.h)
							out.tex[0] = v.uv[0];
							out.tex[1] = v.uv[1];

							remap[source] = static_cast<int32_t>(vertices.size());
							vertices.push_back(out);
							touched.push_back(source);
						}
						indices.push_back(static_cast<uint32_t>(remap[source]));
					}
				}
			}

			for (const uint32_t source : touched)
				remap[source] = -1;
			touched.clear();

			if (indices.empty())
				continue;

			ComputeTangents(vertices, indices);

			ObjectBatch out;
			out.node = batch.node;
			out.lod = batch.lod;
			out.material = GetOrCreateMaterial(batch.state);
			out.visibility = batch.visibility;
			out.triangleCount = static_cast<uint32_t>(indices.size() / 3);
			out.castShadow = batch.state.castShadow && !m_Data.globalNoShadow;
			out.draped = batch.state.draped;
			m_Mesh->AddGeometry(vertices, indices, false, &out.subMesh);
			m_Batches.push_back(std::move(out));
		}

		if (m_Batches.empty())
		{
			if (error)
				*error = "The object has no triangles to draw";
			return false;
		}

		// Appends the geometry to the renderer's global vertex / index buffers (uploaded at the start of the next frame).
		m_Mesh->CreateGpuBuffers();
		ApplyLit(s_LitVisible);

		m_LoadMilliseconds = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
		SEDX_CORE_INFO_TAG(LOG_TAG, "Loaded '{}': {} triangles, {} batches, {} materials, {} animation groups, {} LODs ({:.1f} ms)",
			path.filename().string(), m_Data.GetTriangleCount(), m_Batches.size(), m_Materials.size(), m_Data.nodes.size(),
			m_Data.lods.size(), m_LoadMilliseconds);
		return true;
	}

	void ObjectAsset::LoadTextures()
	{
		const std::filesystem::path folder = m_Path.parent_path();

		const auto load = [&folder](const std::string &reference, const TextureUsage usage, TextureInfo &info) -> Ref<ImageResource>
		{
			if (reference.empty())
				return nullptr;

			const std::filesystem::path file = TextureLoader::ResolveTexturePath(folder, reference);
			if (file.empty())
			{
				info.error = "File not found";
				SEDX_CORE_WARN_TAG(LOG_TAG, "Texture '{}' not found next to '{}'", reference, folder.string());
				return nullptr;
			}
			return TextureLoader::Load(file, usage, &info);
		};

		m_Textures.albedo = load(m_Data.texture, TextureUsage::Albedo, m_Textures.albedoInfo);
		m_Textures.normal = load(m_Data.textureNormal, TextureUsage::Normal, m_Textures.normalInfo);
		m_Textures.lit = load(m_Data.textureLit, TextureUsage::Lit, m_Textures.litInfo);

		TextureInfo drapedNormalInfo;
		m_Textures.drapedAlbedo = load(m_Data.textureDraped, TextureUsage::Albedo, m_Textures.drapedInfo);
		m_Textures.drapedNormal = load(m_Data.textureDrapedNormal, TextureUsage::Normal, drapedNormalInfo);

		// With a normal map the specular level defaults to 1 (standard shading); without one surfaces are matte.
		if (m_Textures.normal && !m_Textures.normalInfo.path.empty())
		{
			const float specular = m_Data.hasGlobalSpecular ? m_Data.globalSpecular : 1.0f;
			m_Textures.packed = TextureLoader::LoadPackedMaterial(m_Textures.normalInfo.path, m_Data.normalMetalness, specular);
		}
	}

	uint32_t ObjectAsset::GetOrCreateMaterial(const Obj8DrawState &state)
	{
		// Only the parts of the draw state the renderer can express split materials.
		const auto same_material = [](const Obj8DrawState &a, const Obj8DrawState &b)
		{
			return a.cull == b.cull && a.draped == b.draped && a.blend == b.blend;
		};

		for (uint32_t i = 0; i < static_cast<uint32_t>(m_MaterialStates.size()); ++i)
		{
			if (same_material(m_MaterialStates[i], state))
				return i;
		}

		const bool useDraped = state.draped && m_Textures.drapedAlbedo;
		ImageResource *albedo = useDraped ? m_Textures.drapedAlbedo.Get() : m_Textures.albedo.Get();
		ImageResource *normal = useDraped ? m_Textures.drapedNormal.Get() : m_Textures.normal.Get();
		ImageResource *packed = useDraped ? nullptr : m_Textures.packed.Get();

		Ref<MaterialAsset> material = CreateRef<MaterialAsset>();
		material->SetName(m_Path.stem().string() + "_" + std::to_string(m_Materials.size()));

		// Without a texture X-Plane draws a gray proxy.
		const float tone = albedo ? 1.0f : 0.6f;
		material->SetProperty(MaterialClass::ColorR, tone);
		material->SetProperty(MaterialClass::ColorG, tone);
		material->SetProperty(MaterialClass::ColorB, tone);
		material->SetProperty(MaterialClass::ColorA, 1.0f);
		material->SetProperty(MaterialClass::TextureTilingX, 1.0f);
		material->SetProperty(MaterialClass::TextureTilingY, 1.0f);

		if (packed)
		{
			// The packed map carries roughness (G) and metalness (B); the scalars become multipliers.
			material->SetProperty(MaterialClass::Roughness, 1.0f);
			material->SetProperty(MaterialClass::Metalness, 1.0f);
		}
		else
		{
			const float specular = m_Data.hasGlobalSpecular ? m_Data.globalSpecular : (normal ? 1.0f : 0.0f);
			material->SetProperty(MaterialClass::Roughness, std::clamp(1.0f - 0.6f * specular, 0.3f, 1.0f));
			material->SetProperty(MaterialClass::Metalness, 0.0f);
		}

		material->SetProperty(MaterialClass::Normal, normal ? 1.0f : 0.0f);
		material->SetProperty(MaterialProperty::CullMode, static_cast<float>(state.cull ? CullMode::Back : CullMode::None));

		material->SetTexture(MaterialTextureType::Color, albedo);
		material->SetTexture(MaterialTextureType::Normal, normal);
		if (packed)
		{
			// The shader samples the packed map for both; the typed slots only raise the "has texture" flags.
			material->SetTexture(MaterialTextureType::None, packed);
			material->SetTexture(MaterialTextureType::Roughness, packed);
			material->SetTexture(MaterialTextureType::Metalness, packed);
		}

		m_Materials.push_back(material);
		m_MaterialStates.push_back(state);
		return static_cast<uint32_t>(m_Materials.size()) - 1;
	}

	void ObjectAsset::ApplyLit(const bool visible)
	{
		for (size_t i = 0; i < m_Materials.size(); ++i)
		{
			// Draped geometry has no LIT texture of its own in OBJ8.
			const bool draped = m_MaterialStates[i].draped && m_Textures.drapedAlbedo;
			m_Materials[i]->SetTexture(MaterialTextureType::Emission, visible && !draped ? m_Textures.lit.Get() : nullptr);
		}
	}

}

// -------------------------------------------------------

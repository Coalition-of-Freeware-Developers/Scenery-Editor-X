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
 * material.cpp
 * -------------------------------------------------------
 * Created: 16/4/2025
 * -------------------------------------------------------
 */
#include "material.h"
#include "SceneryEditorX/asset/asset_manager.h"
#include <SceneryEditorX/renderer/renderer.h>
#include <nlohmann/json.hpp>

// -------------------------------------------------------


namespace SceneryEditorX
{

	// Use nlohmann json for parsing material files
	typedef nlohmann::json json;

	// -------------------------------------------------------

	MaterialAsset::MaterialAsset(const std::string &path)
	{
		MaterialAsset::Load(path);
	}

	MaterialAsset::~MaterialAsset()
	{
		MaterialAsset::Unload();
	}

	// -------------------------------------------------------

	static const std::string ALBEDO_COLOR_UNIFORM	= "u_MaterialUniforms.Albedo";
	static const std::string USE_NORMAL_MAP_UNIFORM	= "u_MaterialUniforms.UseNormalMap";
	static const std::string METALNESS_UNIFORM		= "u_MaterialUniforms.Metalness";
	static const std::string ROUGHNESS_UNIFORM		= "u_MaterialUniforms.Roughness";
	static const std::string EMISSION_UNIFORM		= "u_MaterialUniforms.Emission";
	static const std::string TRANSPARENCY_UNIFORM	= "u_MaterialUniforms.Transparency";
	static const std::string ALBEDO_MAP_UNIFORM		= "u_AlbedoTexture";
	static const std::string NORMAL_MAP_UNIFORM		= "u_NormalTexture";
	static const std::string METALNESS_MAP_UNIFORM	= "u_MetalnessTexture";
	static const std::string ROUGHNESS_MAP_UNIFORM	= "u_RoughnessTexture";

	/*
	void MaterialAsset::Serialize(Serializer &ser)
	{
		/// TODO: Implement serialization
		/// This would store material properties in a specific format
	}
	*/

	void MaterialAsset::Load(const std::string &path)
	{
		// Store the path
		materialPath = path;
		materialName = path.substr(path.find_last_of("/\\") + 1);
		
		// Default values
		color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
		emission = Vec3(0.0f);
		metallic = 0.0f;
		roughness = 1.0f;
		
		/*
		// Clear any existing texture references
		aoMap = nullptr;
		colorMap = nullptr;
		normalMap = nullptr;
		emissionMap = nullptr;
		metallicRoughnessMap = nullptr;
		*/
		
		SEDX_CORE_TRACE("Material initialized with default values: {}", path);
	}

	/*
	void MaterialAsset::Unload()
	{
		// Release texture references
		aoMap = nullptr;
		colorMap = nullptr;
		normalMap = nullptr;
		emissionMap = nullptr;
		metallicRoughnessMap = nullptr;
	}*/

	void MaterialAsset::SetName(const std::string &name) { materialName = name; }
	const std::string &MaterialAsset::GetPath() const { return materialPath; }
	const std::string &MaterialAsset::GetName() const { return materialName; }

	void MaterialAsset::OnDependencyUpdated(const AssetHandle &handle)
	{
		// TODO: Evaluate if this can be changed to an all "if" statement
		if (handle == m_Maps.AlbedoMap)
		{
			// TODO: Add back when asset manager is implemented
			//AssetManager::RemoveAsset(handle);
			SetAlbedoMap(handle);
		}
		else if (handle == m_Maps.NormalMap)
		{
			SetNormalMap(handle);
		}
		else if (handle == m_Maps.MetalnessMap)
		{
			SetMetalnessMap(handle);

		}
		else if (handle == m_Maps.RoughnessMap)
		{
			SetRoughnessMap(handle);
		}
	}

	Vec3& MaterialAsset::GetAlbedoColor() const
	{
		return m_Material->GetVector3(ALBEDO_COLOR_UNIFORM);
	}

	void MaterialAsset::SetAlbedoColor(const Vec3& color) const
	{
		m_Material->Set(ALBEDO_COLOR_UNIFORM, color);
	}

	float& MaterialAsset::GetMetalness() const
	{
		return m_Material->GetFloat(METALNESS_UNIFORM);
	}

	void MaterialAsset::SetMetalness(float value) const
	{
		m_Material->Set(METALNESS_UNIFORM, value);
	}

	float& MaterialAsset::GetRoughness() const
	{
		return m_Material->GetFloat(ROUGHNESS_UNIFORM);
	}

	void MaterialAsset::SetRoughness(float value) const
	{
		m_Material->Set(ROUGHNESS_UNIFORM, value);
	}

	float& MaterialAsset::GetEmission() const
	{
		return m_Material->GetFloat(EMISSION_UNIFORM);
	}

	void MaterialAsset::SetEmission(float value) const
	{
		m_Material->Set(EMISSION_UNIFORM, value);
	}

	/*
	Ref<Texture2D> MaterialAsset::GetAlbedoMap()
	{
		// QUESTION: Is there a reason we need to go to the material here?
		//           Don't we already have the texture handle in m_Maps.AlbedoMap?
		auto texture = m_Material->TryGetTexture2D(ALBEDO_MAP_UNIFORM);
		if (!texture.EqualsObject(Renderer::GetWhiteTexture()))
		{
			if (texture->handle)
			{
				// Return sRGB version of the albedo texture, which is at Handle-1  (see SetAlbedoMap())
				texture = AssetManager::GetAsset<Texture2D>(texture->handle - 1);
				SEDX_CORE_ASSERT(texture);
			}
		}
		return texture;
	}
	*/

	/*
	void MaterialAsset::SetAlbedoMap(AssetHandle handle)
	{
		m_Maps.AlbedoMap = handle;
		if (handle)
		{
			// Handle + 1 is the linear version of the texture
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle + 1);
			if (!texture)
			{
				auto textureSRGB = AssetManager::GetAsset<Texture2D>(handle);
				SEDX_CORE_ASSERT(textureSRGB, "Could not find texture with handle {}", handle); // if this fires, you've passed the wrong handle.  Probably somewhere you retrieved the handle directly from shader.  You need to go through MaterialAsset::GetAlbedoMap()
				if (textureSRGB)
				{
					texture = Texture2D::CreateFromSRGB(textureSRGB);
					texture->handle = handle + 1;
					AssetManager::AddMemoryOnlyAsset(texture);
				}
			}
			m_Material->Set(ALBEDO_MAP_UNIFORM, texture);
			AssetManager::RegisterDependency(handle, pHandle);
		}
		else
		{
			ClearAlbedoMap();
		}
	}
	*/

	/*
	void MaterialAsset::ClearAlbedoMap() const
	{
		AssetManager::DeregisterDependency(m_Maps.AlbedoMap, pHandle);
		m_Material->Set(ALBEDO_MAP_UNIFORM, Renderer::GetWhiteTexture());
	}
	*/

	/*
	Ref<Texture2D> MaterialAsset::GetNormalMap() const
	{
		return m_Material->TryGetTexture2D(NORMAL_MAP_UNIFORM);
	}
	*/

	/*
	bool MaterialAsset::IsUsingNormalMap() const
	{
		return m_Material->GetBool(USE_NORMAL_MAP_UNIFORM);
	}
	*/

	void MaterialAsset::SetUseNormalMap(bool value) const
	{
		m_Material->Set(USE_NORMAL_MAP_UNIFORM, value);
	}

	/*
	void MaterialAsset::ClearNormalMap() const
	{
		//AssetManager::DeregisterDependency(m_Maps.NormalMap, Handle);
		m_Material->Set(NORMAL_MAP_UNIFORM, Renderer::GetWhiteTexture());
	}
	*/

	/*
	Ref<Texture2D> MaterialAsset::GetMetalnessMap() const
	{
		return m_Material->TryGetTexture2D(METALNESS_MAP_UNIFORM);
	}
	*/

	/*
	void MaterialAsset::SetMetalnessMap(const AssetHandle &handle)
	{
		m_Maps.MetalnessMap = handle;

		if (handle)
		{
			const Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);
			m_Material->Set(METALNESS_MAP_UNIFORM, texture);
			AssetManager::RegisterDependency(handle, pHandle);
		}
		else
		{
			ClearMetalnessMap();
		}
	}
	*/

	/*
	void MaterialAsset::ClearMetalnessMap() const
	{
		AssetManager::DeregisterDependency(m_Maps.MetalnessMap, pHandle);
		m_Material->Set(METALNESS_MAP_UNIFORM, Renderer::GetWhiteTexture());
	}
	*/

	/*
	Ref<Texture2D> MaterialAsset::GetRoughnessMap() const
	{
		return m_Material->TryGetTexture2D(ROUGHNESS_MAP_UNIFORM);
	}
	*/

	/*
	void MaterialAsset::SetRoughnessMap(const AssetHandle &handle)
	{
		m_Maps.RoughnessMap = handle;

		if (handle)
		{
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);
			m_Material->Set(ROUGHNESS_MAP_UNIFORM, texture);
			AssetManager::RegisterDependency(handle, pHandle);
		}
		else
		{
			ClearRoughnessMap();
		}
	}
	*/

	/*
	void MaterialAsset::ClearRoughnessMap() const
	{
		AssetManager::DeregisterDependency(m_Maps.RoughnessMap, pHandle);
		m_Material->Set(ROUGHNESS_MAP_UNIFORM, Renderer::GetWhiteTexture());
	}
	*/

	float& MaterialAsset::GetTransparency() const
	{
		return m_Material->GetFloat(TRANSPARENCY_UNIFORM);
	}

	void MaterialAsset::SetTransparency(float transparency) const
	{
		m_Material->Set(TRANSPARENCY_UNIFORM, transparency);
	}


	void MaterialAsset::SetDefaults() const
	{
		if (m_Transparent)
		{
			// Set defaults
			SetAlbedoColor(Vec3(0.8f));

			// Maps
			ClearAlbedoMap();
		}
		else
		{
			// Set defaults
			SetAlbedoColor(Vec3(0.8f));
			SetEmission(0.0f);
			SetUseNormalMap(false);
			SetMetalness(0.0f);
			SetRoughness(0.4f);

			// Maps
			ClearAlbedoMap();
			ClearNormalMap();
			ClearMetalnessMap();
			ClearRoughnessMap();
		}
	}

	// -------------------------------------------------------

	MaterialTable::MaterialTable(const uint32_t materialCount) : m_MaterialCount(materialCount) {}

	MaterialTable::MaterialTable(const Ref<MaterialTable> &other) : m_MaterialCount(other->m_MaterialCount)
	{
		for (const auto& meshMaterials = other->GetMaterials(); const auto &[index, materialAsset] : meshMaterials)
			SetMaterial(index, materialAsset);
	}

	/*
	void MaterialTable::SetMaterial(const uint32_t index, const AssetHandle &material)
	{
		m_Materials[index] = material;
		if (index >= m_MaterialCount)
			m_MaterialCount = index + 1;
	}
	*/

	void MaterialTable::ClearMaterial(const uint32_t index)
	{
		SEDX_CORE_ASSERT(HasMaterial(index));
		m_Materials.erase(index);
		if (index >= m_MaterialCount)
			m_MaterialCount = index + 1;
	}

	void MaterialTable::Clear()
	{
		m_Materials.clear();
	}

}

// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* material.cpp
* -------------------------------------------------------
* Created: 16/4/2025
* -------------------------------------------------------
*/
#include <nlohmann/json.hpp>
#include <SceneryEditorX/asset/texture_manager.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/platform/file_manager.hpp>
#include <SceneryEditorX/scene/material.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /// Use nlohmann json for parsing material files
    using json = nlohmann::json;

    /// -------------------------------------------------------

    MaterialAsset::MaterialAsset(const std::string &path)
    {
        MaterialAsset::Load(path);
    }

    MaterialAsset::~MaterialAsset()
    {
        MaterialAsset::Unload();
    }

    /// -------------------------------------------------------

    /*
    void MaterialAsset::Serialize(Serializer &ser)
    {
        /// TODO: Implement serialization
        /// This would store material properties in a specific format
    }
    */

    void MaterialAsset::Load(const std::string &path)
    {
        /// Store the path
        materialPath = path;
        materialName = path.substr(path.find_last_of("/\\") + 1);
        
        /// Default values
        color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        emission = Vec3(0.0f);
        metallic = 0.0f;
        roughness = 1.0f;
        
        /// Clear any existing texture references
        aoMap = nullptr;
        colorMap = nullptr;
        normalMap = nullptr;
        emissionMap = nullptr;
        metallicRoughnessMap = nullptr;
        
        SEDX_CORE_INFO("Material initialized with default values: {}", path);
    }

    void MaterialAsset::Unload()
    {
        /// Release texture references
        aoMap = nullptr;
        colorMap = nullptr;
        normalMap = nullptr;
        emissionMap = nullptr;
        metallicRoughnessMap = nullptr;
    }

    void MaterialAsset::SetName(const std::string &name) { materialName = name; }
    const std::string &MaterialAsset::GetPath() const { return materialPath; }
    const std::string &MaterialAsset::GetName() const { return materialName; }

    void MaterialAsset::OnDependencyUpdated(AssetHandle handle)
	{
		if (handle == m_Maps.AlbedoMap)
		{
			AssetManager::RemoveAsset(handle + 1);
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

	Vec3& MaterialAsset::GetAlbedoColor()
	{
		return m_Material->GetVector3(s_AlbedoColorUniform);
	}

	void MaterialAsset::SetAlbedoColor(const Vec3& color)
	{
		m_Material->Set(s_AlbedoColorUniform, color);
	}

	float& MaterialAsset::GetMetalness()
	{
		return m_Material->GetFloat(s_MetalnessUniform);
	}

	void MaterialAsset::SetMetalness(float value)
	{
		m_Material->Set(s_MetalnessUniform, value);
	}

	float& MaterialAsset::GetRoughness()
	{
		return m_Material->GetFloat(s_RoughnessUniform);
	}

	void MaterialAsset::SetRoughness(float value)
	{
		m_Material->Set(s_RoughnessUniform, value);
	}

	float& MaterialAsset::GetEmission()
	{
		return m_Material->GetFloat(s_EmissionUniform);
	}

	void MaterialAsset::SetEmission(float value)
	{
		m_Material->Set(s_EmissionUniform, value);
	}

	Ref<Texture2D> MaterialAsset::GetAlbedoMap()
	{
		// QUESTION: Is there a reason we need to go to the material here?
		//           Don't we already have the texture handle in m_Maps.AlbedoMap?
		auto texture = m_Material->TryGetTexture2D(s_AlbedoMapUniform);
		if (!texture.EqualsObject(Renderer::GetWhiteTexture()))
		{
			if (texture->Handle)
			{
				// Return sRGB version of the albedo texture, which is at Handle-1  (see SetAlbedoMap())
				texture = AssetManager::GetAsset<Texture2D>(texture->Handle - 1);
				SEDX_CORE_ASSERT(texture);
			}
		}
		return texture;
	}

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
					texture->Handle = handle + 1;
					AssetManager::AddMemoryOnlyAsset(texture);
				}
			}
			m_Material->Set(s_AlbedoMapUniform, texture);
			AssetManager::RegisterDependency(handle, Handle);
		}
		else
		{
			ClearAlbedoMap();
		}
	}

	void MaterialAsset::ClearAlbedoMap()
	{
		AssetManager::DeregisterDependency(m_Maps.AlbedoMap, Handle);
		m_Material->Set(s_AlbedoMapUniform, Renderer::GetWhiteTexture());
	}

	Ref<Texture2D> MaterialAsset::GetNormalMap()
	{
		return m_Material->TryGetTexture2D(s_NormalMapUniform);
	}

	void MaterialAsset::SetNormalMap(AssetHandle handle)
	{
		m_Maps.NormalMap = handle;

		if (handle)
		{
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);
			m_Material->Set(s_NormalMapUniform, texture);
			AssetManager::RegisterDependency(handle, Handle);
		}
		else
		{
			ClearNormalMap();
		}
	}

	bool MaterialAsset::IsUsingNormalMap()
	{
		return m_Material->GetBool(s_UseNormalMapUniform);
	}

	void MaterialAsset::SetUseNormalMap(bool value)
	{
		m_Material->Set(s_UseNormalMapUniform, value);
	}

	void MaterialAsset::ClearNormalMap()
	{
		AssetManager::DeregisterDependency(m_Maps.NormalMap, Handle);
		m_Material->Set(s_NormalMapUniform, Renderer::GetWhiteTexture());
	}

	Ref<Texture2D> MaterialAsset::GetMetalnessMap()
	{
		return m_Material->TryGetTexture2D(s_MetalnessMapUniform);
	}

	void MaterialAsset::SetMetalnessMap(AssetHandle handle)
	{
		m_Maps.MetalnessMap = handle;

		if (handle)
		{
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);
			m_Material->Set(s_MetalnessMapUniform, texture);
			AssetManager::RegisterDependency(handle, Handle);
		}
		else
		{
			ClearMetalnessMap();
		}
	}

	void MaterialAsset::ClearMetalnessMap()
	{
		AssetManager::DeregisterDependency(m_Maps.MetalnessMap, Handle);
		m_Material->Set(s_MetalnessMapUniform, Renderer::GetWhiteTexture());
	}

	Ref<Texture2D> MaterialAsset::GetRoughnessMap()
	{
		return m_Material->TryGetTexture2D(s_RoughnessMapUniform);
	}

	void MaterialAsset::SetRoughnessMap(AssetHandle handle)
	{
		m_Maps.RoughnessMap = handle;

		if (handle)
		{
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);
			m_Material->Set(s_RoughnessMapUniform, texture);
			AssetManager::RegisterDependency(handle, Handle);
		}
		else
		{
			ClearRoughnessMap();
		}
	}

	void MaterialAsset::ClearRoughnessMap()
	{
		AssetManager::DeregisterDependency(m_Maps.RoughnessMap, Handle);
		m_Material->Set(s_RoughnessMapUniform, Renderer::GetWhiteTexture());
	}

	float& MaterialAsset::GetTransparency()
	{
		return m_Material->GetFloat(s_TransparencyUniform);
	}

	void MaterialAsset::SetTransparency(float transparency)
	{
		m_Material->Set(s_TransparencyUniform, transparency);
	}

	void MaterialAsset::SetDefaults()
	{
		if (m_Transparent)
		{
			///< Set defaults
			SetAlbedoColor(Vec3(0.8f));

			///< Maps
			ClearAlbedoMap();
		}
		else
		{
			///< Set defaults
			SetAlbedoColor(Vec3(0.8f));
			SetEmission(0.0f);
			SetUseNormalMap(false);
			SetMetalness(0.0f);
			SetRoughness(0.4f);

			///< Maps
			ClearAlbedoMap();
			ClearNormalMap();
			ClearMetalnessMap();
			ClearRoughnessMap();
		}
	}

    /// -------------------------------------------------------

	
	MaterialTable::MaterialTable(const uint32_t materialCount) : m_MaterialCount(materialCount) {}

	MaterialTable::MaterialTable(const Ref<MaterialTable> &other) : m_MaterialCount(other->m_MaterialCount)
	{
        for (const auto& meshMaterials = other->GetMaterials(); const auto &[index, materialAsset] : meshMaterials)
			SetMaterial(index, materialAsset);
	}

	void MaterialTable::SetMaterial(const uint32_t index, const AssetHandle &material)
	{
		m_Materials[index] = material;
		if (index >= m_MaterialCount)
			m_MaterialCount = index + 1;
	}

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

/// -------------------------------------------------------

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
#include <SceneryEditorX/asset/manager/asset_manager.h>
#include <SceneryEditorX/core/threading/thread_pool.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <nlohmann/json.hpp>

// -------------------------------------------------------


namespace SceneryEditorX
{

	// Use nlohmann json for parsing material files
	typedef nlohmann::json json;

	// -------------------------------------------------------

	MaterialAsset::MaterialAsset(const std::string &path) : IResource(ResourceType::Material)
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
		// TODO: Implement serialization
		// This would store material properties in a specific format
	}
	*/

	void MaterialAsset::Load(const std::string &path)
	{
		// Store the path
		m_MaterialPath = path;
		m_MaterialName = path.substr(path.find_last_of("/\\") + 1);
		
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

	void MaterialAsset::Unload()
	{
		m_Material.Reset();
		m_Maps = MapAssets{};
	}

	bool MaterialAsset::IsLoaded() const
	{
		return !m_MaterialPath.empty();
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

	void MaterialAsset::SetName(const std::string &name) { m_MaterialName = name; }
	const std::string &MaterialAsset::GetPath() const { return m_MaterialPath; }
	const std::string &MaterialAsset::GetName() const { return m_MaterialName; }

	void MaterialAsset::OnDependencyUpdated(const AssetHandle &handle)
	{
		// TODO: Evaluate if this can be changed to an all "if" statement
		if (handle == m_Maps.m_AlbedoMap)
		{
			// TODO: Add back when asset manager is implemented
			//AssetManager::RemoveAsset(handle);
			SetAlbedoMap(handle);
		}
		else if (handle == m_Maps.m_NormalMap)
		{
			SetNormalMap(handle);
		}
		else if (handle == m_Maps.m_MetalnessMap)
		{
			SetMetalnessMap(handle);

		}
		else if (handle == m_Maps.m_RoughnessMap)
		{
			SetRoughnessMap(handle);
		}
	}

	Vec3& MaterialAsset::GetAlbedoColor() const
	{
		if (!m_Material) // or if (m_Material.m_Ptr == nullptr) if accessible
		{
			SEDX_CORE_ERROR("MaterialAsset::GetAlbedoColor called with null underlying material");
			static Vec3 dummy = Vec3(0.0f);
			return dummy;
		}

		return m_Material->GetVector3(ALBEDO_COLOR_UNIFORM);
	}

	void MaterialAsset::SetAlbedoColor(const Vec3& color) const
	{
		if (!m_Material) // or if (m_Material.m_Ptr == nullptr) if accessible
		{
			SEDX_CORE_ERROR("MaterialAsset::SetAlbedoColor called with null underlying material");
			return;
		}

		m_Material->Set(ALBEDO_COLOR_UNIFORM, color);
	}

	float& MaterialAsset::GetMetalness() const
	{
		if (!m_Material) // or if (m_Material.m_Ptr == nullptr) if accessible
		{
			SEDX_CORE_ERROR("MaterialAsset::GetMetalness called with null underlying material");
			static float dummy = 0.0f;
			return dummy;
		}

		return m_Material->GetFloat(METALNESS_UNIFORM);
	}

	void MaterialAsset::SetMetalness(float value) const
	{
		if (!m_Material) // or if (m_Material.m_Ptr == nullptr) if accessible
		{
			SEDX_CORE_ERROR("MaterialAsset::SetMetalness called with null underlying material");
			return;
		}

		m_Material->Set(METALNESS_UNIFORM, value);
	}

	float& MaterialAsset::GetRoughness() const
	{
		if (!m_Material) // or if (m_Material.m_Ptr == nullptr) if accessible
		{
			SEDX_CORE_ERROR("MaterialAsset::GetRoughness called with null underlying material");
			static float dummy = 0.0f;
			return dummy;
		}

		return m_Material->GetFloat(ROUGHNESS_UNIFORM);
	}

	void MaterialAsset::SetRoughness(float value) const
	{
		if (!m_Material) // or if (m_Material.m_Ptr == nullptr) if accessible
		{
			SEDX_CORE_ERROR("MaterialAsset::SetRoughness called with null underlying material");
			return;
		}

		m_Material->Set(ROUGHNESS_UNIFORM, value);
	}

	float& MaterialAsset::GetEmission() const
	{
		if (!m_Material) // or if (m_Material.m_Ptr == nullptr) if accessible
		{
			SEDX_CORE_ERROR("MaterialAsset::GetEmission called with null underlying material");
			static float dummy = 0.0f;
			return dummy;
		}

		return m_Material->GetFloat(EMISSION_UNIFORM);
	}

	void MaterialAsset::SetEmission(float value) const
	{
		if (!m_Material) // or if (m_Material.m_Ptr == nullptr) if accessible
		{
			SEDX_CORE_ERROR("MaterialAsset::SetEmission called with null underlying material");
			return;
		}

		m_Material->Set(EMISSION_UNIFORM, value);
	}

	uint32_t MaterialAsset::GetUsedSlotCount() const
	{
		// array to track highest used slot for each texture type
		uint32_t max_used_slot[static_cast<size_t>(MaterialTextureType::MaxEnum)] = { 0 };
	
		// iterate through each texture type
		for (size_t type = 0; type < static_cast<size_t>(MaterialTextureType::MaxEnum); type++)
		{
			// check each slot for this type
			for (uint32_t slot = 0; slot < SLOTS_PER_TEXTURE; ++slot)
			{
				// calculate array index using the helper function
				uint32_t index = (static_cast<uint32_t>(type) * SLOTS_PER_TEXTURE) + slot;
				
				// if this slot has a texture, update the max used slot for this type
				if (m_Textures[index])
				{
					max_used_slot[type] = slot + 1; // +1 because we want count, not index
				}
			}
		}
	
		// return the maximum used slot count across all texture types (minimum of 1)
		return xMath::Max<uint32_t>(*std::ranges::max_element(std::begin(max_used_slot), std::end(max_used_slot)), 1);
	}

	Ref<ImageResource> MaterialAsset::GetAlbedoMap() const
	{
	    SEDX_CORE_ERROR_TAG("Material", "GetAlbedoMap called with no underlying material");
		SEDX_CORE_TRACE_TAG("Material", "GetAlbedoMap if not yet implemented");

		return nullptr;
	}

	void MaterialAsset::SetAlbedoMap(const AssetHandle &handle)
	{
		m_Maps.m_AlbedoMap = handle;
		if (handle == AssetHandle{})
		{
			ClearAlbedoMap();
		}

		SEDX_CORE_ERROR_TAG("Material", "SetAlbedoMap called with null underlying material");
	}

	void MaterialAsset::SetProperty(MaterialClass materialClass, const float value)
	{
		if (m_Properties[static_cast<uint32_t>(materialClass)] == value)
			return;

		if (materialClass == MaterialClass::ColorA)
		{
			// if an object switches from opaque to transparent or vice versa, make the world update so that the renderer
			// goes through the entities and makes the ones that use this material, render in the correct mode.
			float current_alpha = m_Properties[static_cast<uint32_t>(materialClass)];
			if ((current_alpha != 1.0f && value == 1.0f) || (current_alpha == 1.0f && value != 1.0f))
			{
				CullMode cull_mode = value < 1.0f ? CullMode::None : CullMode::Back;
				m_Properties[static_cast<uint32_t>(MaterialProperty::CullMode)] = static_cast<float>(cull_mode);
			}

			// transparent objects are typically see-through (low roughness) so use the alpha as the roughness multiplier.
			m_Properties[static_cast<uint32_t>(MaterialClass::Roughness)] = value * 0.5f;
		}

		m_Properties[static_cast<uint32_t>(materialClass)] = value;

		// save on change
		SaveToFile(GetResourceFilePath());
	}

	void MaterialAsset::ClearAlbedoMap() const
	{
		const_cast<MapAssets&>(m_Maps).m_AlbedoMap = AssetHandle{};
	}

	Ref<ImageResource> MaterialAsset::GetNormalMap() const
	{
		SEDX_CORE_ERROR_TAG("Material", "GetNormalMap called with no underlying material");
		SEDX_CORE_TRACE_TAG("Material", "GetNormalMap if not yet implemented");
		return nullptr;
	}

	bool MaterialAsset::IsUsingNormalMap() const
	{
		if (!m_Material) // or if (m_Material.m_Ptr == nullptr) if accessible
		{
			SEDX_CORE_ERROR_TAG("Material", "IsUsingNormalMap called with no underlying material");
		    SEDX_CORE_TRACE_TAG("Material", "IsUsingNormalMap if not yet implemented");
			return false;
		}

		return m_Maps.m_NormalMap != AssetHandle{};
	}

	void MaterialAsset::SetUseNormalMap(bool value) const
	{
		if (!m_Material) // or if (m_Material.m_Ptr == nullptr) if accessible
		{
			SEDX_CORE_ERROR_TAG("Material", "SetUseNormalMap called with no underlying material");
			return;
		}

		m_Material->Set(USE_NORMAL_MAP_UNIFORM, value);
	}

	void MaterialAsset::PrepareForGPU()
	{
		{
			std::scoped_lock lock(m_Mutex);

			// skip if already preparing or if no repack is needed for already-prepared materials
			if (m_ResourceState == ResourceState::PreparingForGpu)
				return;

			bool isRepack = m_ResourceState == ResourceState::PreparedForGpu;
			if (isRepack && !m_NeedsRepack)
				return;

			m_ResourceState = ResourceState::PreparingForGpu;
			// pack textures (this happens synchronously to ensure data is ready)
			for (uint8_t slot = 0; slot < GetUsedSlotCount(); slot++)
			{
				//texture_processing::pack_textures(this, slot);
			}

			m_NeedsRepack = false;
		}

		ThreadPool::Submit([this]()
		{
			{
				std::scoped_lock lock(m_Mutex);

				// prepare any textures that haven't been prepared yet
				for (ImageResource* texture : m_Textures)
				{
					if (texture && texture->GetResourceState() == ResourceState::MaxEnum)
					{
						texture->PrepareForGpu();
					}
				}

				m_ResourceState = ResourceState::PreparedForGpu;
			}

			// check if textures were set during preparation (async texture loading race condition)
			// if so, trigger another preparation cycle to repack with the new textures
			if (m_NeedsRepack)
			{
				PrepareForGPU();
			}
		});
	}

	void MaterialAsset::ClearNormalMap() const
	{
		const_cast<MapAssets&>(m_Maps).m_NormalMap = AssetHandle{};
		SetUseNormalMap(false);
	}

	Ref<ImageResource> MaterialAsset::GetMetalnessMap() const
	{
		return nullptr;
	}

	void MaterialAsset::SetMetalnessMap(const AssetHandle &handle)
	{
		m_Maps.m_MetalnessMap = handle;
		if (handle == AssetHandle{})
		{
			ClearMetalnessMap();
		}
	}

	void MaterialAsset::ClearMetalnessMap() const
	{
		const_cast<MapAssets&>(m_Maps).m_MetalnessMap = AssetHandle{};
	}

	Ref<ImageResource> MaterialAsset::GetRoughnessMap() const
	{
		return nullptr;
	}

	void MaterialAsset::SetRoughnessMap(const AssetHandle &handle)
	{
		m_Maps.m_RoughnessMap = handle;
		if (handle == AssetHandle{})
		{
			ClearRoughnessMap();
		}
	}

	void MaterialAsset::ClearRoughnessMap() const
	{
		const_cast<MapAssets&>(m_Maps).m_RoughnessMap = AssetHandle{};
	}

	void MaterialAsset::SetNormalMap(const AssetHandle &handle)
	{
		m_Maps.m_NormalMap = handle;
		SetUseNormalMap(handle != AssetHandle{});
		if (handle == AssetHandle{})
		{
			ClearNormalMap();
		}
	}

	float& MaterialAsset::GetTransparency() const
	{
		return m_Material->GetFloat(TRANSPARENCY_UNIFORM);
	}

	void MaterialAsset::SetTransparency(float transparency) const
	{
		m_Material->Set(TRANSPARENCY_UNIFORM, transparency);
	}

	bool MaterialAsset::IsAlphaTested() const
	{
		bool albedoMask = false;
		if (ImageResource* texture = GetTexture(MaterialTextureType::Color))
		{
			albedoMask = texture->IsSemiTransparent();
		}

		return HasTextureOfType(MaterialTextureType::AlphaMask) || albedoMask;
	}

	ImageResource *MaterialAsset::GetTexture(MaterialTextureType materialTexture, uint32_t slot) const
	{
		//return m_Material[(static_cast<uint32_t>(materialTexture) * SLOTS_PER_TEXTURE) + slot];
		switch (materialTexture)
		{
			case MaterialTextureType::Color: return GetAlbedoMap().Get();
			case MaterialTextureType::Normal: return GetNormalMap().Get();
			default:
				SEDX_CORE_ERROR("Unsupported material texture type: {}", static_cast<uint32_t>(materialTexture));
				return nullptr;
		}
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

// -------------------------------------------------------

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
 * material.h
 * -------------------------------------------------------
 * Created: 15/4/2025
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/renderer/vulkan/image_view.h>
#include <SceneryEditorX/renderer/vulkan/shader/shader.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class ImageResource;

	/**
	 * @enum MaterialProperty
	 * @brief Material scalar property identifiers used by renderer passes.
	 */
	enum class MaterialProperty : uint8_t
	{
		Tessellation = 0,
		CullMode     = 1,
		MaxEnum		 = 255
	};

	/**
	 * @enum MaterialTextureType
	 * @brief Material texture slot identifiers used by renderer passes.
	 */
	enum class MaterialTextureType : uint8_t
	{
		Color		= 0,
		Normal		= 1,
		Greyscale	= 2,
		MaxEnum		= 255
	};

	/**
	 * @enum MapBits
	 * @brief Material map bit identifiers.
	 */
	enum class MapBits : uint8_t
	{
		Null				= 0,
		Albedo				= BIT(0),
		Normal				= BIT(1),
		MRAO				= BIT(2), // Metalness in R, Roughness in G, Ambient Occlusion in B
		Emission			= BIT(3),
		Metalness			= BIT(4),
		Roughness			= BIT(5),
		AlphaMask			= BIT(6),
		AmbientOcclusion	= BIT(7),
		Specular			= BIT(8),
		Glossiness          = BIT(9),
		DetailMap			= BIT(10),
		BentNormal			= BIT(11),
		Height				= BIT(12),
		Mask0				= BIT(13),
		Mask1				= BIT(14),
		Mask2				= BIT(15),
		Mask3				= BIT(16),
		MaxEnum				= 255
	};

	/**
	 * @enum MaterialFlag
	 * @brief Material rendering flags that can be set to control various aspects of how the material is rendered.
	 */
	enum class MaterialFlag  // NOLINT(performance-enum-size)
	{
		None					= BIT(0),
		DepthTest				= BIT(1),
		Blend					= BIT(2),
		TwoSided				= BIT(3),
		DisableShadowCasting	= BIT(4)
	};

	// -------------------------------------------------------

	/**
	 * @class Material
	 * @brief Represents a material used for rendering, containing properties and textures that define the appearance of a surface.
	 */
	class Material : public IResource, public Asset
	{
	public:
		explicit Material(bool transparent = false);
		explicit Material(Ref<Material> material);
		virtual ~Material();

		static Ref<Material> Create(const Ref<Shader>& shader, const std::string& name = "");
		static Ref<Material> Copy(const Ref<Material>& other, const std::string& name = "");

		Vec3& GetAlbedoColor();
		void SetAlbedoColor(const Vec3& color);

		float& GetMetalness();
		void SetMetalness(float value);

		float& GetRoughness();
		void SetRoughness(float value);

		float& GetEmission();
		void SetEmission(float value);

		virtual bool  IsTransparent() const                               { return false; }
		virtual bool  IsAlphaTested() const                               { return false; }
		virtual float GetProperty(MaterialProperty /*prop*/) const        { return 0.0f;  }
		virtual uint32_t GetIndex() const                                 { return 0;     }
		virtual bool  HasTextureOfType(MaterialTextureType /*t*/) const   { return false; }

		virtual void Invalidate() = 0;
		virtual void OnShaderReloaded() = 0;

		virtual void Set(const std::string& name, float value) = 0;
		virtual void Set(const std::string& name, int value) = 0;
		virtual void Set(const std::string& name, uint32_t value) = 0;
		virtual void Set(const std::string& name, bool value) = 0;
		virtual void Set(const std::string& name, const Vec2& value) = 0;
		virtual void Set(const std::string& name, const Vec3& value) = 0;
		virtual void Set(const std::string& name, const Vec4& value) = 0;
		virtual void Set(const std::string& name, const iVec2& value) = 0;
		virtual void Set(const std::string& name, const iVec3& value) = 0;
		virtual void Set(const std::string& name, const iVec4& value) = 0;
		virtual void Set(const std::string& name, const Mat3& value) = 0;
		virtual void Set(const std::string& name, const Mat4& value) = 0;
		/*virtual void Set(const std::string& name, const Ref<Texture2D>& texture) = 0;
		virtual void Set(const std::string& name, const Ref<Texture2D>& texture, uint32_t arrayIndex) = 0;
		virtual void Set(const std::string& name, const Ref<TextureCube>& texture) = 0;
		virtual void Set(const std::string& name, const Ref<Image2D>& image) = 0;*/
		virtual void Set(const std::string& name, const Ref<ImageView>& image) = 0;

		virtual float& GetFloat(const std::string& name) = 0;
		virtual int32_t& GetInt(const std::string& name) = 0;
		virtual uint32_t& GetUInt(const std::string& name) = 0;
		virtual bool& GetBool(const std::string& name) = 0;
		virtual Vec2& GetVector2(const std::string& name) = 0;
		virtual Vec3& GetVector3(const std::string& name) = 0;
		virtual Vec4& GetVector4(const std::string& name) = 0;
		virtual Mat3& GetMatrix3(const std::string& name) = 0;
		virtual Mat4& GetMatrix4(const std::string& name) = 0;

		virtual Ref<ImageResource> GetTexture2D(const std::string& name) = 0;
		virtual Ref<ImageResource> GetTextureCube(const std::string& name) = 0;
		virtual Ref<ImageResource> TryGetTexture2D(const std::string& name) = 0;
		virtual Ref<ImageResource> TryGetTextureCube(const std::string& name) = 0;

#if 0
		template<typename T>
		T& Get(const std::string& name)
		{
			auto decl = m_Material->FindUniformDeclaration(name);
			SEDX_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			auto& buffer = m_UniformStorageBuffer;
			return buffer.Read<T>(decl->GetOffset());
		}

		template<typename T>
		Ref<T> GetResource(const std::string& name)
		{
			auto decl = m_Material->FindResourceDeclaration(name);
			SEDX_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			uint32_t slot = decl->GetRegister();
			SEDX_CORE_ASSERT(slot < m_Textures.size(), "Texture slot is invalid!");
			return Ref<T>(m_Textures[slot]);
		}

		template<typename T>
		Ref<T> TryGetResource(const std::string& name)
		{
			auto decl = m_Material->FindResourceDeclaration(name);
			if (!decl)
				return nullptr;

			uint32_t slot = decl->GetRegister();
			if (slot >= m_Textures.size())
				return nullptr;

			return Ref<T>(m_Textures[slot]);
		}
#endif

		virtual uint32_t GetFlags() const = 0;
		virtual void SetFlags(uint32_t flags) = 0;

		virtual bool GetFlag(MaterialFlag flag) const = 0;
		virtual void SetFlag(MaterialFlag flag, bool value = true) = 0;

		virtual Ref<Shader> GetShader() = 0;
		virtual const std::string& GetName() const = 0;
	};

	// -------------------------------------------------------

	class MaterialAsset : public IResource, public Asset
	{
	public:
		MaterialAsset() : IResource(ResourceType::Material), color(1.0f), emission(0.0f), metallic(0.0f), roughness(1.0f) {}
		explicit MaterialAsset(const std::string & path);
		virtual ~MaterialAsset() override;
		//virtual void Serialize(Serializer &ser);

		static const uint32_t SLOTS_PER_TEXTURE = 4;

		// -------------------------------------------------------

		virtual void Load(const std::string &path);
		virtual void Unload();
		virtual void SetName(const std::string &name);
		[[nodiscard]] virtual bool IsLoaded() const;
		[[nodiscard]] virtual const std::string &GetPath() const;
		[[nodiscard]] virtual const std::string &GetName() const;
		void OnDependencyUpdated(const AssetHandle &handle);
		bool IsUsingNormalMap() const;
		void SetUseNormalMap(bool value) const;

		// -------------------------------------------------------

		Vec3 &GetAlbedoColor() const;
		float &GetMetalness() const;
		float &GetTransparency() const;
		float &GetRoughness() const;
		float &GetEmission() const;

		void SetAlbedoColor(const Vec3 &color) const;
		void SetAlbedoMap(AssetHandle handle);
		void SetNormalMap(const AssetHandle &handle);
		void SetMetalness(float value) const;
		void SetMetalnessMap(const AssetHandle &handle);
		void SetRoughness(float value) const;
		void SetRoughnessMap(const AssetHandle &handle);
		void SetTransparency(float transparency) const;
		void SetEmission(float value) const;

		void ClearAlbedoMap() const;
		void ClearRoughnessMap() const;
		void ClearMetalnessMap() const;
		void ClearNormalMap() const;

		// -------------------------------------------------------

		Ref<ImageResource> GetAlbedoMap();
		Ref<ImageResource> GetRoughnessMap() const;
		Ref<ImageResource> GetMetalnessMap() const;
		Ref<ImageResource> GetNormalMap() const;

		Ref<Material> GetMaterial() const { return m_Material; }
		void SetMaterial(const Ref<Material> &material) { m_Material = material; }
		bool IsTransparent() const { return m_Transparent; }

		ImageResource *GetTexture(MaterialTextureType materialTexture, uint32_t slot);

		// -------------------------------------------------------

		Vec4 color = Vec4(1.0f);
		Vec3 emission = Vec3(0.0f);
		float metallic = 0.0f;
		float roughness = 1.0f;

		//Ref<ImageResource> aoMap;
		//Ref<ImageResource> colorMap;
		//Ref<ImageResource> normalMap;
		//Ref<ImageResource> emissionMap;
		//Ref<ImageResource> metallicRoughnessMap;

	private:
		void SetDefaults() const;
		std::string materialPath;
		std::string materialName;
		Ref<Material> m_Material;

		struct MapAssets
		{
			AssetHandle AlbedoMap;
			AssetHandle NormalMap;
			AssetHandle MetalnessMap;
			AssetHandle RoughnessMap;
		} m_Maps;

		bool m_Transparent = false;

		friend class AssetManager;
	};

	// -------------------------------------------------------

	class MaterialTable : public RefCounted
	{
	public:
		explicit MaterialTable(uint32_t materialCount = 1);
		explicit MaterialTable(const Ref<MaterialTable> &other);
		virtual ~MaterialTable() override = default;

		bool HasMaterial(const uint32_t materialIndex) const { return m_Materials.contains(materialIndex); }
		void SetMaterial(uint32_t index, const AssetHandle &material);
		void ClearMaterial(uint32_t index);

		AssetHandle GetMaterial(const uint32_t materialIndex) const
		{
			SEDX_CORE_VERIFY(HasMaterial(materialIndex));
			return m_Materials.at(materialIndex);
		}
		std::map<uint32_t, AssetHandle> &GetMaterials() { return m_Materials; }
		const std::map<uint32_t, AssetHandle> &GetMaterials() const { return m_Materials; }

		uint32_t GetMaterialCount() const { return m_MaterialCount; }
		void SetMaterialCount(const uint32_t materialCount) { m_MaterialCount = materialCount; }
		void Clear();

	private:
		std::map<uint32_t, AssetHandle> m_Materials;
		uint32_t m_MaterialCount;
	};

}


// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* material.h
* -------------------------------------------------------
* Created: 15/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/renderer/shaders/shader.h>
#include <SceneryEditorX/renderer/texture.h>
#include <SceneryEditorX/scene/texture.h>
#include <SceneryEditorX/serialization/serializer.hpp>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/// -------------------------------------------------------

    enum class MaterialFlag  // NOLINT(performance-enum-size)
	{
		None					= BIT(0),
		DepthTest				= BIT(1),
		Blend					= BIT(2),
		TwoSided				= BIT(3),
		DisableShadowCasting	= BIT(4)
	};

    /// -------------------------------------------------------

	class Material : public RefCounted
	{
	public:
		static Ref<Material> Create(const Ref<Shader>& shader, const std::string& name = "");
		static Ref<Material> Copy(const Ref<Material>& other, const std::string& name = "");
        virtual ~Material() override;

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
		virtual void Set(const std::string& name, const Ref<Texture2D>& texture) = 0;
		virtual void Set(const std::string& name, const Ref<Texture2D>& texture, uint32_t arrayIndex) = 0;
		virtual void Set(const std::string& name, const Ref<TextureCube>& texture) = 0;
		virtual void Set(const std::string& name, const Ref<Image2D>& image) = 0;
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

		virtual Ref<Texture2D> GetTexture2D(const std::string& name) = 0;
		virtual Ref<TextureCube> GetTextureCube(const std::string& name) = 0;
		virtual Ref<Texture2D> TryGetTexture2D(const std::string& name) = 0;
		virtual Ref<TextureCube> TryGetTextureCube(const std::string& name) = 0;

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
    
    class MaterialAsset : public Asset
    {
    public:
        MaterialAsset() : color(1.0f), emission(0.0f), metallic(0.0f), roughness(1.0f) {}
        explicit MaterialAsset(const std::string & path);
        virtual ~MaterialAsset() override;
        virtual void Serialize(Serializer &ser);

        /// -------------------------------------------------------

        virtual void Load(const std::string &path) override;
        virtual void Unload() override;
        virtual void SetName(const std::string &name) override;
        [[nodiscard]] virtual bool IsLoaded() const;
        [[nodiscard]] virtual const std::string &GetPath() const;
        [[nodiscard]] virtual const std::string &GetName() const;
        void OnDependencyUpdated(AssetHandle handle);
        bool IsUsingNormalMap();
        void SetUseNormalMap(bool value);

        /// -------------------------------------------------------

        Vec3 &GetAlbedoColor();
        float &GetMetalness();
        float &GetTransparency();
        float &GetRoughness();
        float &GetEmission();

        void SetAlbedoColor(const Vec3 &color);
        void SetAlbedoMap(AssetHandle handle);
        void SetNormalMap(AssetHandle handle);
        void SetMetalness(float value);
        void SetMetalnessMap(AssetHandle handle);
        void SetRoughness(float value);
        void SetRoughnessMap(AssetHandle handle);
        void SetTransparency(float transparency);
        void SetEmission(float value);

        void ClearAlbedoMap();
        void ClearRoughnessMap();
        void ClearMetalnessMap();
        void ClearNormalMap();

        /// -------------------------------------------------------

        Ref<Texture2D> GetAlbedoMap();
        Ref<Texture2D> GetRoughnessMap();
        Ref<Texture2D> GetMetalnessMap();
        Ref<Texture2D> GetNormalMap();

        Ref<Material> GetMaterial() const { return m_Material; }
        void SetMaterial(const Ref<Material> &material) { m_Material = material; }
        bool IsTransparent() const { return m_Transparent; }

        /// -------------------------------------------------------

        Vec4 color = Vec4(1.0f);
        Vec3 emission = Vec3(0.0f);
        float metallic = 0.0f;
        float roughness = 1.0f;
        Ref<TextureAsset> aoMap;
        Ref<TextureAsset> colorMap;
        Ref<TextureAsset> normalMap;
        Ref<TextureAsset> emissionMap;
        Ref<TextureAsset> metallicRoughnessMap;

    private:
        void SetDefaults();
        std::string materialPath;
        std::string materialName;
        Ref<Material> m_Material;

        struct MapAssets
		{
			AssetHandle AlbedoMap = 0;
			AssetHandle NormalMap = 0;
			AssetHandle MetalnessMap = 0;
			AssetHandle RoughnessMap = 0;
		} m_Maps;

        bool m_Transparent = false;

        friend class AssetManager;
    };


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
		std::map<uint32_t, AssetHandle>& GetMaterials() { return m_Materials; }
		const std::map<uint32_t, AssetHandle>& GetMaterials() const { return m_Materials; }

		uint32_t GetMaterialCount() const { return m_MaterialCount; }
		void SetMaterialCount(const uint32_t materialCount) { m_MaterialCount = materialCount; }

		void Clear();
	private:
		std::map<uint32_t, AssetHandle> m_Materials;
		uint32_t m_MaterialCount;
	};

}

/// -------------------------------------------------------

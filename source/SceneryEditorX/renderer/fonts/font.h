/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* font.h
* -------------------------------------------------------
* Created: 12/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/asset/asset_types.h>
#include <SceneryEditorX/renderer/texture.h>
#include <SceneryEditorX/utils/pointers.h>
#include <SceneryEditorX/core/memory/buffer.h>
#undef INFINITE
#include <msdf-atlas-gen/msdf-atlas-gen/msdf-atlas-gen.h>
#include <SceneryEditorX/asset/ecs.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	struct MSDFData
	{
	    msdf_atlas::FontGeometry FontGeometry;
	    std::vector<msdf_atlas::GlyphGeometry> Glyphs;
	};

    /// -------------------------------------------------------

    class Font : public Asset
	{
	public:
        explicit Font(const std::filesystem::path& filepath);
        Font(std::string name, Memory::Buffer buffer);
		virtual ~Font() override;

		Ref<Texture2D> GetFontAtlas() const { return m_TextureAtlas; }
		const MSDFData* GetMSDFData() const { return m_MSDFData; }

		static void Init();
		static void Shutdown();
		static Ref<Font> GetDefaultFont();
		static Ref<Font> GetDefaultMonoSpacedFont();
		static Ref<Font> GetFontAssetForTextComponent(const TextComponent& textComponent);

		static ObjectType GetStaticType() { return ObjectType::Font; }
		virtual ObjectType GetAssetType() const override { return GetStaticType(); }

		const std::string& GetName() const { return m_Name; }
	private:
		void CreateAtlas(Memory::Buffer buffer);
		std::string m_Name;
		Ref<Texture2D> m_TextureAtlas;
		MSDFData* m_MSDFData = nullptr;
        Memory::Buffer buffer;

		inline static Ref<Font> s_DefaultFont, s_DefaultMonoSpacedFont;
	};

}

/// -------------------------------------------------------

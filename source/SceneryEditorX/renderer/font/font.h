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
 * font.h
 * -------------------------------------------------------
 * Created: 19/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include "glyph.h"
#include "SceneryEditorX/renderer/vulkan/vertex.h"
#include "SceneryEditorX/utils/inheritance.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Buffer;
	class CommandList;
	class ImageResource;

	/**
	 * @enum FontHintingType
	 * @brief 
	 */
	enum FontHintingType
	{
		Font_Hinting_None,
		Font_Hinting_Light,
		Font_Hinting_Normal
	};

	/**
	 * @enum FontOutlineType
	 * @brief 
	 */
	enum FontOutlineType
	{
		Font_Outline_None,
		Font_Outline_Edge,
		Font_Outline_Positive,
		Font_Outline_Negative
	};

	/**
	 * @class Font
	 * @brief 
	 */
	class Font : public SharedResource
	{
	public:
		Font(const std::string& file_path, const uint32_t font_size, const Color& color);
		~Font() = default;

		// iresource
		void SaveToFile(const std::string& file_path) override;
		void LoadFromFile(const std::string& file_path) override;

		// text
		void AddText(const char* text, const Vec2& positionScreenPercentage);
		bool HasText() const;

		// color
		const Color& GetColor() const     { return m_Color; }
		void SetColor(const Color& color) { m_Color = color; }

		// color outline
		const Color& GetColorOutline() const     { return m_Color_Outline; }
		void SetColorOutline(const Color& color) { m_Color_Outline = color; }

		// outline
		void SetOutline(const FontOutlineType outline) { m_Outline = outline; }
		const FontOutlineType GetOutline() const       { return m_Outline; }

		// outline size
		void SetOutlineSize(const uint32_t outline_size) { m_OutlineSize = outline_size; }
		const uint32_t GetOutlineSize() const            { return m_OutlineSize; }

		// atlas
		const auto& GetAtlas() const         { return m_Atlas; }
		void SetAtlas(const Ref<ImageResource>& atlas)				{ m_Atlas = atlas; }
		const auto& GetAtlasOutline() const  { return m_AtlasOutline; }
		void SetAtlasOutline(const Ref<ImageResource>& atlas)		{ m_AtlasOutline = atlas; }

		// misc
		void UpdateVertexAndIndexBuffers(CommandList* cmd_list);
		uint32_t GetIndexCount();

		// properties
		void SetSize(uint32_t size);
		Buffer* GetIndexBuffer() const                          { return m_IndexBuffers[m_BufferIndex].Get(); }
		Buffer* GetVertexBuffer() const                         { return m_VertexBuffers[m_BufferIndex].Get(); }
		uint32_t GetSize() const                                { return m_FontSize; }
		FontHintingType GetHinting() const						{ return m_Hinting; }
		auto GetForceAutohint() const                      { return m_ForceAutoHint; }
		void SetGlyph(const uint32_t charCode, const Glyph& glyph) { m_Glyphs[charCode] = glyph; }

	private:
		uint32_t m_FontSize         = 14;
		uint32_t m_OutlineSize      = 2;
		bool m_ForceAutoHint        = false;
		FontHintingType m_Hinting	= Font_Hinting_Normal;
		FontOutlineType m_Outline	= Font_Outline_Positive;
		Color m_Color               = Color(1.0f, 1.0f, 1.0f, 1.0f);
		Color m_Color_Outline       = Color(0.0f, 0.0f, 0.0f, 1.0f);
		uint32_t m_CharMax_Width    = 0;
		uint32_t m_CharMax_Height   = 0;
		std::unordered_map<uint32_t, Glyph> m_Glyphs;
		Ref<ImageResource> m_Atlas;
		Ref<ImageResource> m_AtlasOutline;
		std::vector<Vertex_PosTex> m_Vertices;
		std::vector<uint32_t> m_Indices;

		static const uint32_t BUFFER_COUNT              = 8;
		uint32_t m_BufferIndex                          = 0;
		std::array<uint32_t, BUFFER_COUNT> m_IndexCount = { 0 };
		std::array<uint64_t, BUFFER_COUNT> m_VertexBufferCapacity = { 0 };
		std::array<uint64_t, BUFFER_COUNT> m_IndexBufferCapacity  = { 0 };
		std::array<Ref<Buffer>, BUFFER_COUNT> m_IndexBuffers;
		std::array<Ref<Buffer>, BUFFER_COUNT> m_VertexBuffers;
	};

}

// -------------------------------------------------------

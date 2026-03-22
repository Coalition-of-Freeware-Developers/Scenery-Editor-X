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
 * font.cpp
 * -------------------------------------------------------
 * Created: 19/03/2026
 * -------------------------------------------------------
 */
#include "font.h"
#include "glyph.h"
#include <SceneryEditorX/asset/import/font_importer.h>
#include <SceneryEditorX/core/time/timer.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/buffer.h>
#include <SceneryEditorX/renderer/vulkan/command_list.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

    const uint8_t ASCII_TAB      = 9;
    const uint8_t ASCII_NEW_LINE = 10;
    const uint8_t ASCII_SPACE    = 32;

    Font::Font(const std::string & file_path, const uint32_t font_size, const Color& color) : InheritanceBundle<RefCounted, IResource>(ResourceType::Font)
    {
        for (uint32_t i = 0; i < BUFFER_COUNT; i++)
        {
            m_VertexBuffers[i] = CreateRef<Buffer>();
            m_IndexBuffers[i] = CreateRef<Buffer>();
        }
        m_Color = color;

        SetSize(font_size);
        LoadFromFile(file_path);
    }

    void Font::SaveToFile(const std::string & file_path)
    {

    }

    void Font::LoadFromFile(const std::string & file_path)
    {
        const Timer timer;

        // load
        if (!FontImporter::LoadFromFile(this, file_path))
        {
            SEDX_CORE_ERROR_TAG("Font","Failed to load font \"%s\"", file_path.c_str());
            return;
        }

        // find max character height (todo, actually get spacing from FreeType)
        for (const auto& charInfo : m_Glyphs)
        {
            m_CharMax_Width  = xMath::Max(charInfo.second.width, m_CharMax_Width);
            m_CharMax_Height = xMath::Max(charInfo.second.height, m_CharMax_Height);
        }

        SEDX_CORE_INFO_TAG("Font","Loading \"%s\" took %d ms", IO::FileSystem::GetFileNameFromFilePath(file_path).c_str(), static_cast<int>(timer.ElapsedMillis()));
    }

    void Font::AddText(const char* text, const Vec2& positionScreenPercentage)
    {
        // define a maximum vertex limit
        const uint32_t maxVertices = 100'000;
        uint32_t vertex_offset      = static_cast<uint32_t>(m_Vertices.size());

        const float viewport_width  = Renderer::GetViewport().width;
        const float viewport_height = Renderer::GetViewport().height;

        // convert screen percentage to pixel coordinates
        Vec2 position;
        position.x = viewport_width  * positionScreenPercentage.x;
        position.y = viewport_height * (-positionScreenPercentage.y); // flip y-axis to match the screen space coordinates (y is positive downwards in screen space)

        // make the origin be the top left corner
        position.x -= 0.5f * viewport_width;
        position.y += 0.5f * viewport_height;
    
        // generate vertices - draw each letter onto a quad
        xMath::Vec2 cursor = position;
        for (const char* p = text; *p != '\0'; ++p)
        {
            char character = *p;

            // check if adding this character would exceed the vertex limit
            if (m_Vertices.size() + 6 > maxVertices)
                return;
    
            Glyph& glyph = m_Glyphs[character];
    
            if (character == ASCII_TAB)
            {
                // use max character width for consistent tab stops (works reliably across all resolutions)
                const float tabSpacing  = static_cast<float>(m_CharMax_Width) * 4.0f;
                float relativeX         = cursor.x - position.x;
                float nextTabStop       = (floor(relativeX / tabSpacing) + 1.0f) * tabSpacing;
                cursor.x                = position.x + nextTabStop;
            }
            else if (character == ASCII_NEW_LINE)
            {
                cursor.x  = position.x;
                cursor.y -= m_CharMax_Height;
            }
            else if (character == ASCII_SPACE)
            {
                cursor.x += glyph.horizontal_advance;
            }
            else
            {
                // first triangle in quad
                m_Vertices.emplace_back(cursor.x + glyph.offset_x,               cursor.y + glyph.offset_y,                0.0f, glyph.uv_x_left,  glyph.uv_y_top);
                m_Vertices.emplace_back(cursor.x + glyph.offset_x + glyph.width, cursor.y + glyph.offset_y - glyph.height, 0.0f, glyph.uv_x_right, glyph.uv_y_bottom);
                m_Vertices.emplace_back(cursor.x + glyph.offset_x,               cursor.y + glyph.offset_y - glyph.height, 0.0f, glyph.uv_x_left,  glyph.uv_y_bottom);
    
                // second triangle in quad
                m_Vertices.emplace_back(cursor.x + glyph.offset_x,               cursor.y + glyph.offset_y,                0.0f, glyph.uv_x_left,  glyph.uv_y_top);
                m_Vertices.emplace_back(cursor.x + glyph.offset_x + glyph.width, cursor.y + glyph.offset_y,                0.0f, glyph.uv_x_right, glyph.uv_y_top);
                m_Vertices.emplace_back(cursor.x + glyph.offset_x + glyph.width, cursor.y + glyph.offset_y - glyph.height, 0.0f, glyph.uv_x_right, glyph.uv_y_bottom);
    
                // add indices for the two triangles
                for (uint32_t i = 0; i < 6; ++i)
                {
                    m_Indices.push_back(vertex_offset + i);
                }
    
                // advance the cursor and vertex offset
                cursor.x      += glyph.horizontal_advance;
                vertex_offset += 6;
            }
        }
    }

    bool Font::HasText() const
    {
        return !m_Vertices.empty() && !m_Indices.empty();
    }

    void Font::SetSize(const uint32_t size)
    {
        m_FontSize = xMath::Clamp<uint32_t>(size, 8, 50);
    }

    void Font::UpdateVertexAndIndexBuffers(CommandList* cmd_list)
    {
        m_BufferIndex = (m_BufferIndex + 1) % BUFFER_COUNT;
    
        // grow gpu buffers if needed
        {
            // compute how many bytes we need
            uint64_t vertex_data_size = static_cast<uint64_t>(m_Vertices.size()) * sizeof(m_Vertices[0]);
            uint64_t index_data_size  = static_cast<uint64_t>(m_Indices.size())  * sizeof(m_Indices[0]);

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            
            // grow vertex buffer if needed
            if (vertex_data_size > m_VertexBufferCapacity[m_BufferIndex])
            {
                m_VertexBuffers[m_BufferIndex] = CreateRef<Buffer>(
                    VK_NULL_HANDLE,
                    static_cast<VkDeviceSize>(vertex_data_size),
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    allocInfo
                );
               m_VertexBufferCapacity[m_BufferIndex] = vertex_data_size;
            }
            
            // grow index buffer if needed
            if (index_data_size > m_IndexBufferCapacity[m_BufferIndex])
            {
                m_IndexBuffers[m_BufferIndex] = CreateRef<Buffer>(
                 VK_NULL_HANDLE,
                    static_cast<VkDeviceSize>(index_data_size),
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    allocInfo);
               m_IndexBufferCapacity[m_BufferIndex] = index_data_size;
            }
        }
    
        // map vertices and indices to gpu buffers
        uint64_t vertexDataSize = static_cast<uint64_t>(m_Vertices.size()) * sizeof(m_Vertices[0]);
        cmd_list->UpdateBuffer(m_VertexBuffers[m_BufferIndex].Get(), 0, vertexDataSize, m_Vertices.data());
        
        uint64_t indexDataSize = static_cast<uint64_t>(m_Indices.size()) * sizeof(m_Indices[0]);
        cmd_list->UpdateBuffer(m_IndexBuffers[m_BufferIndex].Get(), 0, indexDataSize, m_Indices.data());
    
        // store the used index count
        m_IndexCount[m_BufferIndex] = static_cast<uint32_t>(m_Indices.size());
    
        // clear vertices and indices
        m_Vertices.clear();
        m_Indices.clear();
    }

    uint32_t Font::GetIndexCount()
    {
        return m_IndexCount[m_BufferIndex];
    }

}

// -------------------------------------------------------

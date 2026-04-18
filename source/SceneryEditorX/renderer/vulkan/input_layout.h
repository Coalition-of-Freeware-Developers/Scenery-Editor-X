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
 * input_layout.h
 * -------------------------------------------------------
 * Created: 02/04/2026
 * -------------------------------------------------------
 */
#pragma once
#include "vertex.h"
#include <SceneryEditorX/utils/inheritance.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @struct VertexAttribute
	 * @brief Represents a vertex attribute in the input layout.
	 */
	struct VertexAttribute 
	{
		/**
		 * @brief Constructs a VertexAttribute with the specified parameters.
		 * @param name The name of the vertex attribute.
		 * @param location The location of the vertex attribute.
		 * @param binding The binding index of the vertex attribute.
		 * @param format The format of the vertex attribute.
		 * @param offset The offset of the vertex attribute within the vertex structure.
		 */
		VertexAttribute(const std::string& name, const uint32_t location, const uint32_t binding, const VkFormat format, const uint32_t offset)
		{
			this->name     = name;
			this->location = location;
			this->binding  = binding;
			this->format   = format;
			this->offset   = offset;
		}

		std::string name;	// For informational purposes; not used in Vulkan pipeline creation but can be helpful for debugging and reflection.
		uint32_t location;	// The location of the vertex attribute, used in shader input declarations (e.g., layout(location = 0) in GLSL).
		uint32_t binding;	// The binding index of the vertex attribute, used in Vulkan pipeline creation to specify which vertex buffer binding this attribute comes from.
		VkFormat format;	// The format of the vertex attribute, specified as a Vulkan format (e.g., VK_FORMAT_R32G32B32_SFLOAT for a vec3).
		uint32_t offset;	// The byte offset of the vertex attribute within the vertex structure, used in Vulkan pipeline creation to specify where this attribute is located in the vertex data.
	};

	/**
	 * @class InputLayout
	 * @brief Defines the layout of vertex input data for a shader, including the vertex attributes and their formats.
	 */
	class InputLayout : public SharedObject
	{
	public:
		InputLayout() = default;
		~InputLayout();

		/**
		 * @brief Creates an input layout based on the specified vertex type, which determines the vertex attributes and their formats.
		 * @param type The vertex type to create the input layout for.
		 */
		void Create(const VertexType type)
		{
			const uint32_t binding = 0;

			if (type == VertexType::MaxEnum)
			{
				// The full-screen triangle vertex shaders generates its own vertices.
				// Therefore, it doesn't need to define a vertex type for an input layout.
				m_VertexSize = 0;
			}
			else if (type == VertexType::Position)
			{
				m_VertexAttributes =
				{
					{ "POSITION", 0, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_Pos, pos) }
				};

				m_VertexSize = sizeof(Vertex_Pos);
			}
			else if (type == VertexType::PositionUv)
			{
				m_VertexAttributes =
				{
					{ "POSITION", 0, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PosTex, pos) },
					{ "TEXCOORD", 1, binding, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex_PosTex, tex) }
				};

				m_VertexSize = sizeof(Vertex_PosTex);
			}
			else if (type == VertexType::PositionColor)
			{
				m_VertexAttributes =
				{
					{ "POSITION", 0, binding, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex_PosCol, pos) },
					{ "COLOR",    1, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex_PosCol, col) }
				};

				m_VertexSize = sizeof(Vertex_PosCol);
			}
			else if (type == VertexType::Position2dUvColor8)
			{
				m_VertexAttributes =
				{
					{ "POSITION", 0, binding, VK_FORMAT_R32G32_SFLOAT,   offsetof(Vertex_Pos2dTexCol8, pos) },
					{ "TEXCOORD", 1, binding, VK_FORMAT_R32G32_SFLOAT,   offsetof(Vertex_Pos2dTexCol8, tex) },
					{ "COLOR",    2, binding, VK_FORMAT_R8G8B8A8_UNORM, offsetof(Vertex_Pos2dTexCol8, col) }
				};

				m_VertexSize = sizeof(Vertex_Pos2dTexCol8);
			}
			else if (type == VertexType::PositionUvNormalTangent)
			{
				m_VertexAttributes =
				{
					{ "POSITION", 0, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PosTexNorTan, pos) },
					{ "TEXCOORD", 1, binding, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex_PosTexNorTan, tex) },
					{ "NORMAL",   2, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PosTexNorTan, nor) },
					{ "TANGENT",  3, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_PosTexNorTan, tan) }
				};

				m_VertexSize = sizeof(Vertex_PosTexNorTan);
			}
		}

		/**
		 * @brief Get the vertex type of the input layout.
		 * @return The vertex type.
		 */
		VertexType GetVertexType() const { return m_VertexType; }

		/**
		 * @brief Get the size of the vertex in bytes.
		 * @return The size of the vertex in bytes.
		 */
		const uint32_t GetVertexSize() const { return m_VertexSize; }

		/**
		 * @brief Get the vertex attribute descriptions.
		 * @return The vertex attribute descriptions.
		 */
		const std::vector<VertexAttribute>& GetAttributeDescriptions() const { return m_VertexAttributes; }

		/**
		 * @brief Get the number of vertex attributes.
		 * @return The number of vertex attributes.
		 */
		uint32_t GetAttributeCount() const { return static_cast<uint32_t>(m_VertexAttributes.size()); }

		/**
		 * @brief Compare two input layouts for equality.
		 * @param rhs The input layout to compare with.
		 * @return True if the input layouts are equal, false otherwise.
		 */
		bool operator==(const InputLayout& rhs) const { return m_VertexType == rhs.GetVertexType(); }

	private:
		VertexType m_VertexType = VertexType::MaxEnum;
		uint32_t m_VertexSize = 0;
		bool CreateLayout();
		std::vector<VertexAttribute> m_VertexAttributes;
	};

}

// -------------------------------------------------------

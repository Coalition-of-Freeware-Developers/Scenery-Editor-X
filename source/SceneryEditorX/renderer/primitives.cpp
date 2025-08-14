/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* primitives.cpp
* -------------------------------------------------------
* Created: 20/5/2025
* -------------------------------------------------------
*/
#include <numbers>

#include <SceneryEditorX/renderer/primitives.h>
#include <SceneryEditorX/scene/model_asset.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	ObjectType Primitives::CreateBox(const Vec3 &size)
	{
        SEDX_CORE_INFO_TAG("PRIMITIVES", "Creating box primitive with size: ({}, {}, {})", size.x, size.y, size.z);

        std::vector<MeshVertex> vertices;
		vertices.resize(24); // 6 faces * 4 vertices per face

        // Define the 8 corner positions of the box
        const Vec3 corners[8] = {
            { -size.x / 2.0f, -size.y / 2.0f,  size.z / 2.0f }, // 0: front-bottom-left
            {  size.x / 2.0f, -size.y / 2.0f,  size.z / 2.0f }, // 1: front-bottom-right
            {  size.x / 2.0f,  size.y / 2.0f,  size.z / 2.0f }, // 2: front-top-right
            { -size.x / 2.0f,  size.y / 2.0f,  size.z / 2.0f }, // 3: front-top-left
            { -size.x / 2.0f, -size.y / 2.0f, -size.z / 2.0f }, // 4: back-bottom-left
            {  size.x / 2.0f, -size.y / 2.0f, -size.z / 2.0f }, // 5: back-bottom-right
            {  size.x / 2.0f,  size.y / 2.0f, -size.z / 2.0f }, // 6: back-top-right
            { -size.x / 2.0f,  size.y / 2.0f, -size.z / 2.0f }  // 7: back-top-left
        };

        // Define face normals
        const Vec3 normals[6] = {
            { 0.0f,  0.0f,  1.0f}, // Front face
            { 0.0f,  0.0f, -1.0f}, // Back face
            {-1.0f,  0.0f,  0.0f}, // Left face
            { 1.0f,  0.0f,  0.0f}, // Right face
            { 0.0f, -1.0f,  0.0f}, // Bottom face
            { 0.0f,  1.0f,  0.0f}  // Top face
        };

        // Define texture coordinates for each vertex of a face
        const Vec2 texCoords[4] = {
            {0.0f, 0.0f}, // bottom-left
            {1.0f, 0.0f}, // bottom-right
            {1.0f, 1.0f}, // top-right
            {0.0f, 1.0f}  // top-left
        };

        // Define faces using corner indices
        const uint32_t faces[6][4] = {
            {0, 1, 2, 3}, // Front face
            {5, 4, 7, 6}, // Back face
            {4, 0, 3, 7}, // Left face
            {1, 5, 6, 2}, // Right face
            {4, 5, 1, 0}, // Bottom face
            {3, 2, 6, 7}  // Top face
        };

        // Generate vertices for each face
        for (int face = 0; face < 6; ++face)
        {
            for (int vertex = 0; vertex < 4; ++vertex)
            {
                const int vertexIndex = face * 4 + vertex;
                const int cornerIndex = faces[face][vertex];

                vertices[vertexIndex].position = corners[cornerIndex];
                vertices[vertexIndex].normal = normals[face];
                vertices[vertexIndex].texCoord = texCoords[vertex];
                vertices[vertexIndex].color = Vec3(1.0f, 1.0f, 1.0f); // White color
                vertices[vertexIndex].tangent = Vec4(1.0f, 0.0f, 0.0f, 1.0f); // Default tangent
            }
        }

        // Generate indices for triangulated faces (2 triangles per face)
		std::vector<ModelAsset::Index> indices;
        indices.reserve(36); // 6 faces * 2 triangles * 3 vertices

        for (int face = 0; face < 6; ++face)
        {
            const uint32_t baseIndex = face * 4;

            // First triangle (0, 1, 2)
            indices.push_back({.V1 = baseIndex + 0, .V2 = baseIndex + 1, .V3 = baseIndex + 2});
            // Second triangle (2, 3, 0)
            indices.push_back({.V1 = baseIndex + 2, .V2 = baseIndex + 3, .V3 = baseIndex + 0});
        }

        //TODO: Create and return actual ObjectType/ModelAsset from vertices and indices
        SEDX_CORE_INFO_TAG("PRIMITIVES", "Box primitive created successfully with {} vertices and {} indices",
                           vertices.size(), indices.size());
	    return {};
	}

    /// -------------------------------------------------------

	ObjectType Primitives::CreateSphere(const float radius)
	{
        SEDX_CORE_INFO_TAG("PRIMITIVES", "Creating sphere primitive with radius: {}", radius);

        std::vector<MeshVertex> vertices;
        std::vector<ModelAsset::Index> indices;

		constexpr uint32_t latitudeBands = 30;
		constexpr uint32_t longitudeBands = 30;

        // Generate vertices
        for (uint32_t latitude = 0; latitude <= latitudeBands; latitude++)
        {
            const float theta = static_cast<float>(latitude) * SceneryEditorX::PI / static_cast<float>(latitudeBands);
            const float sinTheta = std::sin(theta);
            const float cosTheta = std::cos(theta);

            for (uint32_t longitude = 0; longitude <= longitudeBands; longitude++)
            {
                const float phi = static_cast<float>(longitude) * 2.0f * SceneryEditorX::PI / static_cast<float>(longitudeBands);
                const float sinPhi = std::sin(phi);
                const float cosPhi = std::cos(phi);

				MeshVertex vertex;

                // Calculate normal (points outward from center)
				vertex.normal = Vec3(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta);

                // Position is normal scaled by radius
				vertex.position = radius * vertex.normal;

                // Texture coordinates
                vertex.texCoord = Vec2(
                    static_cast<float>(longitude) / static_cast<float>(longitudeBands),
                    static_cast<float>(latitude) / static_cast<float>(latitudeBands)
                );

                // Color and tangent
                vertex.color = Vec3(1.0f, 1.0f, 1.0f);

                // Calculate tangent for sphere (derivative with respect to longitude)
                vertex.tangent = Vec4(-sinPhi, 0.0f, cosPhi, 1.0f);

				vertices.push_back(vertex);
			}
		}

        // Generate indices
		for (uint32_t latitude = 0; latitude < latitudeBands; latitude++)
		{
			for (uint32_t longitude = 0; longitude < longitudeBands; longitude++)
			{
				const uint32_t first = (latitude * (longitudeBands + 1)) + longitude;
				const uint32_t second = first + longitudeBands + 1;

                // First triangle
				indices.push_back({.V1 = first, .V2 = second, .V3 = first + 1});
                // Second triangle
				indices.push_back({.V1 = second, .V2 = second + 1, .V3 = first + 1});
			}
		}

        //TODO: Create and return actual ObjectType/ModelAsset from vertices and indices
        SEDX_CORE_INFO_TAG("PRIMITIVES", "Sphere primitive created successfully with {} vertices and {} indices",
                           vertices.size(), indices.size());
        return {};
	}

    /// -------------------------------------------------------

	ObjectType Primitives::CreateCylinder(float radius, float height)
	{
        SEDX_CORE_INFO_TAG("PRIMITIVES", "Creating cylinder primitive with radius: {}, height: {}", radius, height);

		std::vector<MeshVertex> vertices;
		std::vector<ModelAsset::Index> indices;

		/// TODO: Allow a user to modify the number of loop segments, height, and height segments.
        constexpr int segments = 30;
        const float halfHeight = height / 2.0f;

        /// Create center vertices for top and bottom caps
		MeshVertex topCenter = {
		    .position = {0.0f, halfHeight, 0.0f},
		    .color = {1.0f, 1.0f, 1.0f},
            .normal = {0.0f, 1.0f, 0.0f},
            .tangent = {1.0f, 0.0f, 0.0f, 1.0f},
			.texCoord = {0.5f, 0.5f}
		};

		MeshVertex bottomCenter = {
			.position = {0.0f, -halfHeight, 0.0f},
		    .color = {1.0f, 1.0f, 1.0f},
			.normal = {0.0f, -1.0f, 0.0f},
            .tangent = {1.0f, 0.0f, 0.0f, 1.0f},
			.texCoord = {0.5f, 0.5f}
		};

		vertices.push_back(topCenter);    // Index 0
		vertices.push_back(bottomCenter); // Index 1

        /// Create side vertices
        for (int i = 0; i <= segments; ++i)
        {
            const float theta = static_cast<float>(i) / static_cast<float>(segments) * 2.0f * SceneryEditorX::PI;
            const float x = radius * std::cos(theta);
            const float z = radius * std::sin(theta);

            const Vec3 normal = Normalize(Vec3(x, 0.0f, z));
            const float u = static_cast<float>(i) / static_cast<float>(segments);

            /// Top ring vertex
			vertices.push_back({
				.position = {x, halfHeight, z},
				.color = {1.0f, 1.0f, 1.0f},
			    .normal = normal,
				.tangent = {-sin(theta), 0.0f, cos(theta), 1.0f},
				.texCoord = {u, 1.0f}
			});

            // Bottom ring vertex
            vertices.push_back({
                .position = {x, -halfHeight, z},
                .color = {1.0f, 1.0f, 1.0f},
                .normal = normal,
                .tangent = {-sin(theta), 0.0f, cos(theta), 1.0f},
                .texCoord = {u, 0.0f}
            });
		}

        // Generate indices for caps and sides
		for (int i = 0; i < segments; ++i)
		{
            const uint32_t topIndex1 = 2 + i * 2;
            const uint32_t topIndex2 = 2 + ((i + 1) % (segments + 1)) * 2;
            const uint32_t bottomIndex1 = 3 + i * 2;
            const uint32_t bottomIndex2 = 3 + ((i + 1) % (segments + 1)) * 2;

            // Top cap (center is index 0)
			indices.push_back({.V1 = 0, .V2 = topIndex1, .V3 = topIndex2});

            // Bottom cap (center is index 1)
			indices.push_back({.V1 = 1, .V2 = bottomIndex2, .V3 = bottomIndex1});

            // Side faces (2 triangles per segment)
			indices.push_back({.V1 = topIndex1, .V2 = bottomIndex1, .V3 = bottomIndex2});
			indices.push_back({.V1 = topIndex1, .V2 = bottomIndex2, .V3 = topIndex2});
		}

        //TODO: Create and return actual ObjectType/ModelAsset from vertices and indices
        SEDX_CORE_INFO_TAG("PRIMITIVES", "Cylinder primitive created successfully with {} vertices and {} indices",
                           vertices.size(), indices.size());
	    return {};
	}

    /// -------------------------------------------------------

	ObjectType Primitives::CreatePlane(const Vec2 &size)
	{
        SEDX_CORE_INFO_TAG("PRIMITIVES", "Creating plane primitive with size: ({}, {})", size.x, size.y);

        std::vector<MeshVertex> vertices;
        std::vector<ModelAsset::Index> indices;

        const float halfWidth = size.x / 2.0f;
        const float halfHeight = size.y / 2.0f;

        // Create the 4 vertices of the plane (lying in XY plane, facing +Z)
        vertices.reserve(4);

        // Bottom-left
        vertices.push_back({
            .position = {-halfWidth, -halfHeight, 0.0f},
            .color = {1.0f, 1.0f, 1.0f},
            .normal = {0.0f, 0.0f, 1.0f},
            .tangent = {1.0f, 0.0f, 0.0f, 1.0f},
            .texCoord = {0.0f, 0.0f}
        });

        // Bottom-right
        vertices.push_back({
            .position = {halfWidth, -halfHeight, 0.0f},
            .color = {1.0f, 1.0f, 1.0f},
            .normal = {0.0f, 0.0f, 1.0f},
            .tangent = {1.0f, 0.0f, 0.0f, 1.0f},
            .texCoord = {1.0f, 0.0f}
        });

        // Top-right
        vertices.push_back({
            .position = {halfWidth, halfHeight, 0.0f},
            .color = {1.0f, 1.0f, 1.0f},
            .normal = {0.0f, 0.0f, 1.0f},
            .tangent = {1.0f, 0.0f, 0.0f, 1.0f},
            .texCoord = {1.0f, 1.0f}
        });

        // Top-left
        vertices.push_back({
            .position = {-halfWidth, halfHeight, 0.0f},
            .color = {1.0f, 1.0f, 1.0f},
            .normal = {0.0f, 0.0f, 1.0f},
            .tangent = {1.0f, 0.0f, 0.0f, 1.0f},
            .texCoord = {0.0f, 1.0f}
        });

        // Create indices for 2 triangles
        indices.reserve(2);

        // First triangle (bottom-left, bottom-right, top-right)
        indices.push_back({.V1 = 0, .V2 = 1, .V3 = 2});

        // Second triangle (top-right, top-left, bottom-left)
        indices.push_back({.V1 = 2, .V2 = 3, .V3 = 0});

        //TODO: Create and return actual ObjectType/ModelAsset from vertices and indices
        SEDX_CORE_INFO_TAG("PRIMITIVES", "Plane primitive created successfully with {} vertices and {} indices",
                           vertices.size(), indices.size());
	    return {};
	}

    /// -------------------------------------------------------

    /*
    ObjectType Primitives::CreatePrimitiveType(PrimitiveType type, const Vec3& size)
    {
        SEDX_CORE_INFO_TAG("PRIMITIVES", "Creating primitive of type: {} with size: ({}, {}, {})",
                          static_cast<int>(type), size.x, size.y, size.z);

        // Validate input parameters
        if (!ValidatePrimitiveDimensions(type, size))
        {
            SEDX_CORE_ERROR_TAG("PRIMITIVES", "Invalid dimensions for primitive type: {}", static_cast<int>(type));
            return {};
        }

        switch (type)
        {
            using enum PrimitiveType;

            case Cube:		return CreateBox(size);
            case Sphere:	return CreateSphere(size.x); /// For sphere, only use the x component as radius
            case Cylinder:	return CreateCylinder(size.x, size.y); /// For cylinder, x = radius, y = height
            case Plane:		return CreatePlane(Vec2(size.x, size.y)); /// For plane, use x and y components for width and height

            case None:		SEDX_CORE_WARN_TAG("PRIMITIVES", "Attempted to create primitive of type None");
                            break;
            default:
                SEDX_CORE_ERROR_TAG("PRIMITIVES", "Unknown primitive type: {}", static_cast<int>(type));
                break;
        }

        SEDX_CORE_ASSERT(false, "Unknown PrimitiveType!");
        return {};
    }
    */

    /// -------------------------------------------------------

    bool Primitives::ValidatePrimitiveDimensions(PrimitiveType type, const Vec3& size)
    {
        switch (type)
        {
            using enum PrimitiveType;

            case Cube:		return size.x > 0.0f && size.y > 0.0f && size.z > 0.0f; /// All dimensions must be positive
            case Sphere:	return size.x > 0.0f; /// Radius (x component) must be positive
            case Cylinder:	return size.x > 0.0f && size.y > 0.0f; /// Radius (x component) and height (y component) must be positive
            case Plane:		return size.x > 0.0f && size.y > 0.0f; /// Width (x component) and height (y component) must be positive
            case None:		return false;
            default:		return false;
        }
    }

    /// -------------------------------------------------------

    /// UI Placeholder Methods - To be implemented when UI system is ready

    /*
    bool Primitives::ShowPrimitiveCreationUI(bool& isOpen)
    {
        // TODO: Implement ImGui interface with ImGuizmo integration
        // This will include:
        // - Dropdown for primitive type selection
        // - Sliders/input fields for dimensions
        // - ImGuizmo for visual manipulation
        // - Preview rendering
        // - Create button

        SEDX_CORE_WARN_TAG("PRIMITIVES", "ShowPrimitiveCreationUI not yet implemented");
        isOpen = false;
        return false;
    }
    */

    bool Primitives::ShowTextInputInterface(PrimitiveType& primitiveType, Vec3& dimensions)
    {
        // TODO: Implement text-based input interface
        // This will include:
        // - Simple dropdown for primitive type
        // - Text input fields for dimensions
        // - Validation feedback
        // - OK/Cancel buttons

        SEDX_CORE_WARN_TAG("PRIMITIVES", "ShowTextInputInterface not yet implemented");
        return false;
    }

    /// -------------------------------------------------------

}

/// -------------------------------------------------------

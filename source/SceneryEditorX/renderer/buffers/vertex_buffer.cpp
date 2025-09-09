/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vertex_buffer.cpp
* -------------------------------------------------------
* Created: 18/5/2025
* -------------------------------------------------------
*/
#include "vertex_buffer.h"
#include <numbers>
#include "SceneryEditorX/logging/profiler.hpp"
#include "SceneryEditorX/renderer/primitives.h"
#include "SceneryEditorX/renderer/renderer.h"
#include "SceneryEditorX/renderer/vulkan/vk_buffers.h"

/// ---------------------------------------------------------

namespace SceneryEditorX
{

    VertexBuffer::VertexBuffer(uint64_t size)
        : m_BufferType(VertexBufferType::Dynamic),
		m_Format(VertexFormat::Position3D_Color3), /// or whatever your default format is
		m_Capacity(0), m_Size(size), m_IsInitialized(false)
    {
		SEDX_CORE_INFO_TAG("VERTEX_BUFFER", "Creating VertexBuffer with size: {} bytes", size);

		/// Initialize the buffer with the specified size
		CreateVertexBuffer();

		m_IsInitialized = true;
    }

    /**
     * @brief Constructor for VertexBuffer with buffer type and format.
     *
     * @param type The type of vertex buffer (Static, Dynamic, etc.)
     * @param vertexFormat The format of vertices to be stored.
     * @param initialCapacity Initial buffer capacity in vertices (optional).
     */
    VertexBuffer::VertexBuffer(VertexBufferType type, VertexFormat vertexFormat, uint32_t initialCapacity) : m_BufferType(type), m_Format(vertexFormat), m_Capacity(initialCapacity)
    {
        SEDX_CORE_INFO_TAG("VERTEX_BUFFER", "Creating vertex buffer: type={}, format={}, capacity={}", static_cast<int>(type), static_cast<int>(vertexFormat), initialCapacity);

        // Initialize MemoryAllocator
        m_MemoryAllocator = CreateRef<MemoryAllocator>("VertexBuffer");

        if (initialCapacity > 0)
        {
            m_Vertices.reserve(initialCapacity);
            m_Size = initialCapacity * sizeof(Vertex);
            CreateVertexBuffer();
        }
    }

    /**
     * @brief Constructor for VertexBuffer with initial vertex data.
     *
     * @param initialVertices Vector of vertices to initialize the buffer with.
     * @param type The type of vertex buffer (Static, Dynamic, etc.).
     */
    VertexBuffer::VertexBuffer(const std::vector<Vertex>& initialVertices, VertexBufferType type) : m_Vertices(initialVertices), m_BufferType(type), m_Format(VertexFormat::Position3D_Color3)
    {
        SEDX_CORE_INFO_TAG("VERTEX_BUFFER", "Creating vertex buffer with {} vertices", initialVertices.size());

        /// Initialize MemoryAllocator
        m_MemoryAllocator = CreateRef<MemoryAllocator>("VertexBuffer");

        m_Size = m_Vertices.size() * sizeof(Vertex);
        m_Capacity = static_cast<uint32_t>(m_Vertices.size());

        /// Create local data buffer
        m_LocalData.Allocate(m_Size);
        if (!m_Vertices.empty())
            m_LocalData.Write(m_Vertices.data(), m_Size, 0);

        /// Create GPU buffer using MemoryAllocator
        CreateVertexBuffer();
        UploadVertexData();
    }

    /**
     * @brief Constructor for VertexBuffer with raw data.
     *
     * @param data Pointer to vertex data.
     * @param size Size of the data in bytes.
     * @param usage Buffer usage type (default: Static).
     */
    VertexBuffer::VertexBuffer(const void *data, uint64_t size, VertexBufferType usage) : m_BufferType(usage), m_Format(VertexFormat::Custom), m_Size(size)
    {
        SEDX_CORE_INFO_TAG("VERTEX_BUFFER", "Creating vertex buffer with raw data: {} bytes", size);

        // Initialize MemoryAllocator
        m_MemoryAllocator = CreateRef<MemoryAllocator>("VertexBuffer");

        /// Create local data buffer
        m_LocalData.Allocate(size);
        if (data && size > 0)
            m_LocalData.Write(data, size, 0);

        /// Calculate vertex count based on standard vertex size
        m_Capacity = static_cast<uint32_t>(size / sizeof(Vertex));

        /// Create GPU buffer using MemoryAllocator
        CreateVertexBuffer();
        UploadVertexData();
    }

    /**
     * @brief Destructor for VertexBuffer
     *
     * Releases all resources including the Vulkan buffer through MemoryAllocator.
     * The Buffer destructor automatically handles cleanup through RAII.
     */
    VertexBuffer::~VertexBuffer()
    {
        SEDX_CORE_INFO_TAG("VERTEX_BUFFER", "Destroying vertex buffer with {} vertices", GetVertexCount());

        /// Release local data
        m_LocalData.Release();

        /// GPU buffer cleanup is handled automatically by Buffer destructor
        m_IsInitialized = false;

        /// MemoryAllocator is automatically cleaned up by smart pointer
    }

    /**
     * @brief Sets new vertex data, replacing existing data
     *
     * @param newVertices Vector of new vertices
     * @param recreateBuffer Whether to recreate the buffer immediately
     */
    void VertexBuffer::SetData(const std::vector<Vertex> &newVertices, bool recreateBuffer)
    {
        SEDX_PROFILE_SCOPE("VertexBuffer::SetData");
        SEDX_CORE_TRACE_TAG("VERTEX_BUFFER", "Setting data: {} vertices", newVertices.size());

        m_Vertices = newVertices;
        m_Size = m_Vertices.size() * sizeof(Vertex);
        m_Capacity = static_cast<uint32_t>(m_Vertices.size());

        /// Update local data buffer
        m_LocalData.Release();
        m_LocalData.Allocate(m_Size);

        if (!m_Vertices.empty())
            m_LocalData.Write(m_Vertices.data(), m_Size, 0);

        if (recreateBuffer)
        {
            CreateVertexBuffer();
            UploadVertexData();
        }
    }

    /**
     * @brief Adds vertices to the buffer
     *
     * @param additionalVertices Vector of vertices to add
     * @param recreateBuffer Whether to recreate the buffer immediately
     */
    void VertexBuffer::AppendData(const std::vector<Vertex> &additionalVertices, bool recreateBuffer)
    {
        SEDX_PROFILE_SCOPE("VertexBuffer::AppendData");
        SEDX_CORE_TRACE_TAG("VERTEX_BUFFER", "Appending {} vertices to {} existing", additionalVertices.size(), m_Vertices.size());

        /// Reserve space for better performance
        m_Vertices.reserve(m_Vertices.size() + additionalVertices.size());
        m_Vertices.insert(m_Vertices.end(), additionalVertices.begin(), additionalVertices.end());

        m_Size = m_Vertices.size() * sizeof(Vertex);
        m_Capacity = static_cast<uint32_t>(m_Vertices.size());

        /// Update local data buffer
        m_LocalData.Release();
        m_LocalData.Allocate(m_Size);
        if (!m_Vertices.empty())
            m_LocalData.Write(m_Vertices.data(), m_Size, 0);

        if (recreateBuffer)
        {
            CreateVertexBuffer();
            UploadVertexData();
        }
    }

    /**
     * @brief Updates a subset of vertices in the buffer
     *
     * @param startIndex Starting index to update
     * @param updatedVertices Vector of vertices with new data
     */
    void VertexBuffer::UpdateData(uint32_t startIndex, const std::vector<Vertex> &updatedVertices)
    {
        SEDX_PROFILE_SCOPE("VertexBuffer::UpdateData");
        SEDX_CORE_ASSERT(startIndex + updatedVertices.size() <= m_Vertices.size(), "Update range exceeds buffer size");
        SEDX_CORE_TRACE_TAG("VERTEX_BUFFER", "Updating {} vertices starting at index {}", updatedVertices.size(), startIndex);

        /// Update local vertex data
        for (size_t i = 0; i < updatedVertices.size(); ++i)
            m_Vertices[startIndex + i] = updatedVertices[i];

        /// Update local data buffer
        const uint64_t offset = startIndex * sizeof(Vertex);
        const uint64_t updateSize = updatedVertices.size() * sizeof(Vertex);
        m_LocalData.Write(updatedVertices.data(), updateSize, offset);

        /// For dynamic buffers, update GPU memory directly
        if (m_BufferType == VertexBufferType::Dynamic && m_VertexBuffer.memory & MemoryType::CPU)
        {
            if (void* mapped = MapBuffer(m_VertexBuffer))
            {
                memcpy(static_cast<uint8_t*>(mapped) + offset, updatedVertices.data(), updateSize);
                UnmapBuffer(m_VertexBuffer);
            }
        }
        else
        {
            /// For static buffers, use staging buffer to update
            UploadPartialVertexData(offset, updateSize);
        }
    }

    /**
     * @brief Clears all vertex data
     *
     * @param releaseBuffer Whether to also release the GPU buffer
     */
    void VertexBuffer::ClearData(bool releaseBuffer)
    {
        SEDX_CORE_TRACE_TAG("VERTEX_BUFFER", "Clearing vertex data (release buffer: {})", releaseBuffer);

        m_Vertices.clear();
        m_Size = 0;
        m_LocalData.Release();

        if (releaseBuffer)
        {
            /// Reset GPU buffer
            m_VertexBuffer = Buffer{};
            m_IsInitialized = false;
        }
    }

    /**
     * @brief Resizes the buffer to accommodate a specific number of vertices
     *
     * @param newCapacity The new capacity in vertices
     * @param preserveData Whether to preserve existing vertex data
     */
    void VertexBuffer::Reserve(uint32_t newCapacity, bool preserveData)
    {
        SEDX_CORE_TRACE_TAG("VERTEX_BUFFER", "Reserving capacity for {} vertices (preserve: {})", newCapacity, preserveData);

        if (newCapacity <= m_Capacity)
            return;

        if (preserveData)
        {
            m_Vertices.reserve(newCapacity);
        }
        else
        {
            m_Vertices.clear();
            m_Vertices.reserve(newCapacity);
            m_Size = 0;
        }

        m_Capacity = newCapacity;

        /// Update size for new capacity
        m_Size = preserveData ? m_Vertices.size() * sizeof(Vertex) : newCapacity * sizeof(Vertex);

        /// Recreate buffer with new capacity
        CreateVertexBuffer();
        if (preserveData && !m_Vertices.empty())
            UploadVertexData();
    }

    /**
     * @brief Gets the binding description for this vertex buffer
     *
     * @param binding Binding index to use
     * @param inputRate Vertex input rate (vertex or instance)
     * @return VkVertexInputBindingDescription structure
     */
    VkVertexInputBindingDescription VertexBuffer::GetBindingDescription(uint32_t binding, VkVertexInputRate inputRate) const
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = binding;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = inputRate;
        return bindingDescription;
    }

    /**
     * @brief Gets attribute descriptions for this vertex buffer
     *
     * @param binding The binding index these attributes are associated with
     * @return Array of VkVertexInputAttributeDescription structures
     */
    std::vector<VkVertexInputAttributeDescription> VertexBuffer::GetAttributeDescriptions(uint32_t binding) const
    {
        auto attributeArray = Vertex::GetAttributeDescriptions(binding);
        return {attributeArray.begin(), attributeArray.end()};
    }

    /**
     * @brief Creates a vertex buffer representing a primitive geometry
     *
     * This method creates a vertex buffer containing the geometric data for the specified
     * primitive type. It leverages the Primitives class to generate the mesh data, then
     * converts it to the VertexBuffer's internal Vertex format and creates a GPU buffer.
     *
     * @param type Type of primitive shape to create (Cube, Sphere, Cylinder, Plane)
     * @param size Size of the primitive - interpretation depends on primitive type:
     *             - Cube: Vec3(width, height, depth)
     *             - Sphere: Vec3(radius, 0, 0) - only x component used
     *             - Cylinder: Vec3(radius, height, 0) - x=radius, y=height
     *             - Plane: Vec3(width, height, 0) - only x,y components used
     * @param color Color to apply to all primitive vertices (default: white)
     * @return Ref<VertexBuffer> Reference to the created vertex buffer, or nullptr if creation failed
     */
    /*
    Ref<VertexBuffer> VertexBuffer::CreatePrimitive(PrimitiveType type, const Vec3 &size, const Vec3 &color)
    {
        SEDX_PROFILE_SCOPE("VertexBuffer::CreatePrimitive");
        SEDX_CORE_INFO_TAG("VERTEX_BUFFER", "Creating primitive vertex buffer: type={}, size=({}, {}, {}), color=({}, {}, {})",
                          static_cast<int>(type), size.x, size.y, size.z, color.r, color.g, color.b);

        /// Validate input parameters
        if (type == PrimitiveType::None)
        {
            SEDX_CORE_WARN_TAG("VERTEX_BUFFER", "Cannot create vertex buffer for PrimitiveType::None");
            return nullptr;
        }

        try
        {
            /// Generate primitive mesh data
            std::vector<Vertex> vertices;
            switch (type)
            {
                using enum PrimitiveType;

                case Cube:		vertices = GenerateCubeVertices(size, color);		break;
                case Sphere:	vertices = GenerateSphereVertices(size.x, color);	break;
                case Cylinder:	vertices = GenerateCylinderVertices(size.x, size.y, color); break;
                case Plane:		vertices = GeneratePlaneVertices(Vec2(size.x, size.y), color);	break;
                default:
                    SEDX_CORE_ERROR_TAG("VERTEX_BUFFER", "Unsupported primitive type: {}", static_cast<int>(type));
                    return nullptr;
            }

            if (vertices.empty())
            {
                SEDX_CORE_ERROR_TAG("VERTEX_BUFFER", "Failed to generate vertices for primitive type: {}", static_cast<int>(type));
                return nullptr;
            }

            /// Create vertex buffer with the generated vertices
            auto vertexBuffer = CreateRef<VertexBuffer>(vertices, VertexBufferType::Static);

            SEDX_CORE_INFO_TAG("VERTEX_BUFFER", "Primitive vertex buffer created successfully with {} vertices", vertices.size());
            return vertexBuffer;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("VERTEX_BUFFER", "Exception creating primitive vertex buffer: {}", e.what());
            return nullptr;
        }
    }
    */

    /**
     * @brief Creates vertex attribute descriptions based on the vertex format
     */
    std::vector<VkVertexInputAttributeDescription> VertexBuffer::CreateAttributeDescriptions(uint32_t binding) const
    {
        return GetAttributeDescriptions(binding);
    }

    /**
     * @brief Static factory method to create a VertexBuffer from raw data.
     *
     * @param data Pointer to vertex data.
     * @param size Size of the data in bytes.
     * @param usage Buffer usage type.
     * @param debugName Optional debug name for the buffer.
     * @return Ref<VertexBuffer> Reference-counted pointer to the created VertexBuffer.
     */
    /*
    Ref<VertexBuffer> VertexBuffer::Create(const void* data, uint64_t size, VertexBufferType usage, const std::string& debugName)
    {
        Ref<VertexBuffer> buffer = CreateRef<VertexBuffer>(data, size, usage);
        if (!debugName.empty())
            buffer->m_VertexBuffer.resource->debugName = debugName;

        return buffer;
    }
    */

    /// ---- Private Implementation Methods ----

    /**
     * @brief Creates the internal GPU buffer using the MemoryAllocator system
     */
    void VertexBuffer::CreateVertexBuffer()
    {
        SEDX_PROFILE_SCOPE("VertexBuffer::CreateVertexBuffer");

        if (m_Size == 0)
        {
            SEDX_CORE_WARN_TAG("VERTEX_BUFFER", "Attempting to create buffer with zero size");
            return;
        }

        /// Initialize memory allocator if not already done
        if (!m_MemoryAllocator)
            m_MemoryAllocator = CreateRef<MemoryAllocator>("VertexBuffer");

        try
        {
            /// Determine memory type based on buffer type
            MemoryFlags memoryType = MemoryType::GPU;
            if (m_BufferType == VertexBufferType::Dynamic || m_BufferType == VertexBufferType::Streaming)
                memoryType = MemoryType::CPU;

            /// Create buffer using existing Buffer system
            m_VertexBuffer = CreateBuffer(m_Size, BufferUsage::Vertex, memoryType);

            if (!m_VertexBuffer.resource || m_VertexBuffer.resource->buffer == VK_NULL_HANDLE)
            {
                SEDX_CORE_ERROR_TAG("VERTEX_BUFFER", "Failed to create vertex buffer");
                return;
            }

            m_IsInitialized = true;

            SEDX_CORE_TRACE_TAG("VERTEX_BUFFER", "Created vertex buffer: {} bytes, Type: {}",
                               m_Size, static_cast<int>(m_BufferType));
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("VERTEX_BUFFER", "Failed to create vertex buffer: {}", e.what());
            m_IsInitialized = false;
            throw;
        }
    }

    /**
     * @brief Uploads vertex data to the GPU buffer using the Buffer system
     */
    void VertexBuffer::UploadVertexData()
    {
        SEDX_PROFILE_SCOPE("VertexBuffer::UploadVertexData");

        if (!m_IsInitialized || m_Size == 0 || !m_LocalData.data)
            return;

        try
        {
            if (m_VertexBuffer.memory & MemoryType::CPU)
            {
                /// Direct memory mapping for CPU-accessible buffers
                if (void* mapped = MapBuffer(m_VertexBuffer))
                {
                    memcpy(mapped, m_LocalData.data, m_Size);
                    UnmapBuffer(m_VertexBuffer);
                    SEDX_CORE_TRACE_TAG("VERTEX_BUFFER", "Uploaded {} bytes via direct mapping", m_Size);
                }
                else
                {
                    SEDX_CORE_ERROR_TAG("VERTEX_BUFFER", "Failed to map vertex buffer memory");
                }
            }
            else
            {
                /// Use staging buffer for GPU-only memory
                Buffer stagingBuffer = CreateBuffer(m_Size, BufferUsage::TransferSrc, MemoryType::CPU);

                if (!stagingBuffer.resource || stagingBuffer.resource->buffer == VK_NULL_HANDLE)
                {
                    SEDX_CORE_ERROR_TAG("VERTEX_BUFFER", "Failed to create staging buffer");
                    return;
                }

                /// Upload to staging buffer
                if (void* mapped = MapBuffer(stagingBuffer))
                {
                    memcpy(mapped, m_LocalData.data, m_Size);
                    UnmapBuffer(stagingBuffer);

                    /// Copy from staging to GPU buffer
                    CopyBuffer(stagingBuffer.resource->buffer, m_VertexBuffer.resource->buffer, m_Size);

                    SEDX_CORE_TRACE_TAG("VERTEX_BUFFER", "Uploaded {} bytes via staging buffer", m_Size);
                }
                else
                {
                    SEDX_CORE_ERROR_TAG("VERTEX_BUFFER", "Failed to map staging buffer memory");
                }

                /// Staging buffer is automatically cleaned up by Buffer destructor
            }
        }
        catch (const std::exception& exept)
        {
            SEDX_CORE_ERROR_TAG("VERTEX_BUFFER", "Failed to upload vertex data: {}", exept.what());
            throw;
        }
    }

    /**
     * @brief Uploads partial vertex data to the GPU buffer using the Buffer system with offset support
     */
    void VertexBuffer::UploadPartialVertexData(uint64_t offset, uint64_t size) const
    {
        SEDX_PROFILE_SCOPE("VertexBuffer::UploadPartialVertexData");

        if (!m_IsInitialized || size == 0 || !m_LocalData.data)
            return;

        SEDX_CORE_ASSERT(offset + size <= m_Size, "Partial update exceeds buffer size");

        try
        {
            if (m_VertexBuffer.memory & MemoryType::CPU)
            {
                /// Direct memory mapping for CPU-accessible buffers with offset
                if (void* mapped = MapBuffer(m_VertexBuffer))
                {
                    memcpy(static_cast<uint8_t*>(mapped) + offset, static_cast<uint8_t*>(m_LocalData.data) + offset, size);
                    UnmapBuffer(m_VertexBuffer);
                    SEDX_CORE_TRACE_TAG("VERTEX_BUFFER", "Uploaded {} bytes at offset {} via direct mapping", size, offset);
                }
                else
                {
                    SEDX_CORE_ERROR_TAG("VERTEX_BUFFER", "Failed to map vertex buffer memory");
                }
            }
            else
            {
                /// Use staging buffer for partial updates with offset support
                Buffer stagingBuffer = CreateBuffer(size, BufferUsage::TransferSrc, MemoryType::CPU);

                if (!stagingBuffer.resource || stagingBuffer.resource->buffer == VK_NULL_HANDLE)
                {
                    SEDX_CORE_ERROR_TAG("VERTEX_BUFFER", "Failed to create staging buffer for partial update");
                    return;
                }

                /// Upload to staging buffer
                if (void* mapped = MapBuffer(stagingBuffer))
                {
                    memcpy(mapped, static_cast<uint8_t*>(m_LocalData.data) + offset, size);
                    UnmapBuffer(stagingBuffer);

                    /// Copy from staging to GPU buffer with offset using CopyBufferRegion
                    CopyBufferRegion(stagingBuffer.resource->buffer, m_VertexBuffer.resource->buffer, size, 0, offset);

                    SEDX_CORE_TRACE_TAG("VERTEX_BUFFER", "Uploaded {} bytes at offset {} via staging buffer", size, offset);
                }
                else
                {
                    SEDX_CORE_ERROR_TAG("VERTEX_BUFFER", "Failed to map staging buffer memory");
                }

                /// Staging buffer is automatically cleaned up by Buffer destructor
            }
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("VERTEX_BUFFER", "Failed to upload partial vertex data: {}", e.what());
            throw;
        }
    }

    /// ---- Primitive Generation Helper Methods ----

    /**
     * @brief Generates vertices for a cube primitive
     *
     * This method generates vertices for a cube centered at the origin.
     *
     * @param size Size of the cube in Vec3 format (width, height, depth).
     * @param color Color to apply to all vertices of the cube.
     */
    std::vector<VertexBuffer::Vertex> VertexBuffer::GenerateCubeVertices(const Vec3& size, const Vec3& color)
    {
        std::vector<Vertex> vertices;
        vertices.reserve(24); /// 6 faces * 4 vertices per face

        /// Define the 8 corner positions of the box
        const Vec3 corners[8] = {
            { -size.x / 2.0f, -size.y / 2.0f,  size.z / 2.0f }, /// 0: front-bottom-left
            {  size.x / 2.0f, -size.y / 2.0f,  size.z / 2.0f }, /// 1: front-bottom-right
            {  size.x / 2.0f,  size.y / 2.0f,  size.z / 2.0f }, /// 2: front-top-right
            { -size.x / 2.0f,  size.y / 2.0f,  size.z / 2.0f }, /// 3: front-top-left
            { -size.x / 2.0f, -size.y / 2.0f, -size.z / 2.0f }, /// 4: back-bottom-left
            {  size.x / 2.0f, -size.y / 2.0f, -size.z / 2.0f }, /// 5: back-bottom-right
            {  size.x / 2.0f,  size.y / 2.0f, -size.z / 2.0f }, /// 6: back-top-right
            { -size.x / 2.0f,  size.y / 2.0f, -size.z / 2.0f }  /// 7: back-top-left
        };

        /// Define texture coordinates for each vertex of a face
        Vec2 texCoords[4] = {
            {0.0f, 0.0f}, /// bottom-left
            {1.0f, 0.0f}, /// bottom-right
            {1.0f, 1.0f}, /// top-right
            {0.0f, 1.0f}  /// top-left
        };

        /// Define faces using corner indices
        const uint32_t faces[6][4] = {
            {0, 1, 2, 3}, /// Front face
            {5, 4, 7, 6}, /// Back face
            {4, 0, 3, 7}, /// Left face
            {1, 5, 6, 2}, /// Right face
            {4, 5, 1, 0}, /// Bottom face
            {3, 2, 6, 7}  /// Top face
        };

        /// Generate vertices for each face
        for (auto face : faces)
        {
            for (int vertex = 0; vertex < 4; ++vertex)
            {
                const int cornerIndex = face[vertex];

                /// Use constructor with position, color, and texture coordinates
                vertices.emplace_back(corners[cornerIndex], color, texCoords[vertex]);
            }
        }

        return vertices;
    }

    /**
     * @brief Generates vertices for a sphere primitive
     */
    std::vector<VertexBuffer::Vertex> VertexBuffer::GenerateSphereVertices(float radius, const Vec3& color)
    {
        std::vector<Vertex> vertices;

        constexpr uint32_t latitudeBands = 30;
        constexpr uint32_t longitudeBands = 30;

        /// Generate vertices
        for (uint32_t latitude = 0; latitude <= latitudeBands; latitude++)
        {
            const float theta = static_cast<float>(latitude) * xMath::PI / static_cast<float>(latitudeBands);
            const float sinTheta = std::sin(theta);
            const float cosTheta = std::cos(theta);

            for (uint32_t longitude = 0; longitude <= longitudeBands; longitude++)
            {
                const float phi = static_cast<float>(longitude) * 2.0f * xMath::PI / static_cast<float>(longitudeBands);
                const float sinPhi = std::sin(phi);
                const float cosPhi = std::cos(phi);

                /// Calculate position
                Vec3 position = Vec3(
                    radius * cosPhi * sinTheta,
                    radius * cosTheta,
                    radius * sinPhi * sinTheta
                );

                /// Texture coordinates
                Vec2 texCoord = Vec2(
                    static_cast<float>(longitude) / static_cast<float>(longitudeBands),
                    static_cast<float>(latitude) / static_cast<float>(latitudeBands)
                );

                /// Use constructor with position, color, and texture coordinates
                vertices.emplace_back(position, color, texCoord);
            }
        }

        return vertices;
    }

    /**
     * @brief Generates vertices for a cylinder primitive
     */
    std::vector<VertexBuffer::Vertex> VertexBuffer::GenerateCylinderVertices(float radius, float height, const Vec3& color)
    {
        std::vector<Vertex> vertices;

        constexpr int segments = 30;
        const float halfHeight = height / 2.0f;

        /// Create center vertices for top and bottom caps
        vertices.emplace_back(Vec3(0.0f, halfHeight, 0.0f), color, Vec2(0.5f, 0.5f));   /// Index 0 - top center
        vertices.emplace_back(Vec3(0.0f, -halfHeight, 0.0f), color, Vec2(0.5f, 0.5f));  /// Index 1 - bottom center

        /// Create side vertices
        for (int i = 0; i <= segments; ++i)
        {
            const float theta = static_cast<float>(i) / static_cast<float>(segments) * 2.0f * xMath::PI;
            const float x = radius * std::cos(theta);
            const float z = radius * std::sin(theta);
            const float u = static_cast<float>(i) / static_cast<float>(segments);

            /// Top ring vertex
            vertices.emplace_back(Vec3(x, halfHeight, z), color, Vec2(u, 1.0f));

            /// Bottom ring vertex
            vertices.emplace_back(Vec3(x, -halfHeight, z), color, Vec2(u, 0.0f));
        }

        return vertices;
    }

    /**
     * @brief Generates vertices for a plane primitive
     */
    std::vector<VertexBuffer::Vertex> VertexBuffer::GeneratePlaneVertices(const Vec2& size, const Vec3& color)
    {
        std::vector<Vertex> vertices;
        vertices.reserve(6); /// 2 triangles * 3 vertices per triangle

        const float halfWidth = size.x / 2.0f;
        const float halfHeight = size.y / 2.0f;

        /// First triangle: bottom-left, bottom-right, top-right
        vertices.emplace_back(Vec3(-halfWidth, -halfHeight, 0.0f), color, Vec2(0.0f, 0.0f));
        vertices.emplace_back(Vec3(halfWidth, -halfHeight, 0.0f), color, Vec2(1.0f, 0.0f));
        vertices.emplace_back(Vec3(halfWidth, halfHeight, 0.0f), color, Vec2(1.0f, 1.0f));

        /// Second triangle: top-right, top-left, bottom-left
        vertices.emplace_back(Vec3(halfWidth, halfHeight, 0.0f), color, Vec2(1.0f, 1.0f));
        vertices.emplace_back(Vec3(-halfWidth, halfHeight, 0.0f), color, Vec2(0.0f, 1.0f));
        vertices.emplace_back(Vec3(-halfWidth, -halfHeight, 0.0f), color, Vec2(0.0f, 0.0f));

        return vertices;
    }
}

/// ---------------------------------------------------------

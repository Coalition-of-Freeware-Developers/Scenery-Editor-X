/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vertex_buffer.h
* -------------------------------------------------------
* Created: 18/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <GraphicsEngine/vulkan/vk_core.h>

/// ----------------------------------------------------------

namespace SceneryEditorX
{
    /// Forward Declaration
    enum class PrimitiveType : uint8_t;

	/**
     * @enum VertexBufferType
     * @brief Defines the type and usage pattern of vertex buffer.
     */
    enum class VertexBufferType : uint8_t
    {
        None = 0,
        Static = 1,      ///< Static data, rarely or never updated (GPU optimized).
        Dynamic = 2,     ///< Frequently changed data (CPU-GPU shared memory).
        Transient = 3,   ///< Single-use buffer that will be discarded after rendering.
        Streaming = 4    ///< Continuously streamed data (e.g. particles).
    };

    /**
     * @enum VertexFormat
     * @brief Standard vertex data formats.
     */
    enum class VertexFormat : uint8_t
    {
        None = 0,
        Position2D = 1,							///< Vec2 position
        Position3D = 2,							///< Vec3 position
        Position3D_Color3 = 3,					///< Vec3 position + Vec3 color
        Position3D_Color4 = 4,					///< Vec3 position + Vec4 color
        Position3D_Normal = 5,					///< Vec3 position + Vec3 normal
        Position3D_TexCoord = 6,				///< Vec3 position + Vec2 texcoord
        Position3D_Color4_TexCoord = 7,			///< Vec3 position + Vec4 color + Vec2 texcoord
        Position3D_Normal_TexCoord = 8,			///< Vec3 position + Vec3 normal + Vec2 texcoord
        Position3D_Normal_TexCoord_Tangent = 9, ///< Vec3 position + Vec3 normal + Vec2 texcoord + Vec4 tangent
        Custom = 255                            ///< Custom vertex format defined by user
    };

    /**
     * @class VertexBuffer
     * @brief Manages vertex data storage and configuration in Vulkan.
     *
     * This class handles the creation and management of vertex buffers in Vulkan,
     * including memory allocation and vertex attribute descriptions. It provides
     * functionality for storing and transferring vertex data to the GPU.
     */
    class VertexBuffer
    {
    public:
        /**
         * @struct Vertex
         * @brief Base vertex structure that can be extended for different vertex formats.
         *
         * Contains position, color, and texture coordinate data for each vertex.
         */
        struct Vertex
        {
            Vec3 pos;      ///< 3D position of the vertex.
            Vec3 color;    ///< RGB color of the vertex.
            Vec2 texCoord; ///< Texture coordinates of the vertex.

            /**
             * @brief Default constructor
             */
            Vertex() = default;

            /**
             * @brief Constructor with position.
             *
             * @param position 3D position of the vertex.
             */
            explicit Vertex(const Vec3& position) 
                : pos(position), color(1.0f, 1.0f, 1.0f), texCoord(0.0f, 0.0f) {}

            /**
             * @brief Constructor with position and color.
             *
             * @param position 3D position of the vertex.
             * @param vertexColor RGB color of the vertex.
             */
            Vertex(const Vec3& position, const Vec3& vertexColor)
                : pos(position), color(vertexColor), texCoord(0.0f, 0.0f) {}

            /**
             * @brief Constructor with position, color and texture coordinates.
             *
             * @param position 3D position of the vertex.
             * @param vertexColor RGB color of the vertex.
             * @param uv Texture coordinates of the vertex.
             */
            Vertex(const Vec3& position, const Vec3& vertexColor, const Vec2& uv)
                : pos(position), color(vertexColor), texCoord(uv) {}

            /**
             * @brief Provides the vertex binding description for Vulkan.
             * @param binding Binding index to use.
             * @param inputRate Vertex input rate (vertex or instance).
             * @return VkVertexInputBindingDescription describing how to bind vertex data.
             */
            static VkVertexInputBindingDescription GetBindingDescription( uint32_t binding = 0, VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX)
            {
                VkVertexInputBindingDescription bindingDescription = {};
                bindingDescription.binding = binding;
                bindingDescription.stride = sizeof(Vertex);
                bindingDescription.inputRate = inputRate;
                return bindingDescription;
            }

            /**
             * @brief Provides attribute descriptions for vertex data components.
             * @param binding The binding index these attributes are associated with.
             * @return Array of attribute descriptions for position, color, and texture coordinates.
             */
            static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions(uint32_t binding = 0)
            {
                std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

                attributeDescriptions[0].binding = binding;
                attributeDescriptions[0].location = 0;
                attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributeDescriptions[0].offset = offsetof(Vertex, pos);

                attributeDescriptions[1].binding = binding;
                attributeDescriptions[1].location = 1;
                attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributeDescriptions[1].offset = offsetof(Vertex, color);

                attributeDescriptions[2].binding = binding;
                attributeDescriptions[2].location = 2;
                attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
                attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

                return attributeDescriptions;
            }

            /**
             * @brief Equality comparison operator for vertices.
             * @param other The vertex to compare against.
             * @return true if vertices are equal, false otherwise.
             */
            bool operator==(const Vertex &other) const
            {
                return pos == other.pos && color == other.color && texCoord == other.texCoord;
            }
        };

        /**
         * @brief Constructor for VertexBuffer.
         * 
         * @param type The type of vertex buffer (Static, Dynamic, etc.)
         * @param vertexFormat The format of vertices to be stored.
         * @param initialCapacity Initial buffer capacity in vertices (optional).
         */
        explicit VertexBuffer(
            VertexBufferType type,
            VertexFormat vertexFormat,
            uint32_t initialCapacity = 0
        );

        /**
         * @brief Constructor for VertexBuffer with initial data.
         * 
         * @param initialVertices Vector of vertices to initialize the buffer with.
         * @param type The type of vertex buffer (Static, Dynamic, etc.).
         */
        explicit VertexBuffer(
            const std::vector<Vertex>& initialVertices,
            VertexBufferType type = VertexBufferType::Static
        );

        /**
         * @brief Destructor for VertexBuffer
         */
        virtual ~VertexBuffer();

        /**
         * @brief Creates the vertex buffer with current data
         * @return Buffer structure representing the created vertex buffer
         */
        [[nodiscard]] Buffer Create(); // Renamed from CreateVertexBuffer to Create
        
        /**
         * @brief Gets the Vulkan buffer handle
         * @return VkBuffer handle to the vertex buffer
         */
        [[nodiscard]] VkBuffer GetBuffer() const { return vertexBuffer; }
        
        /**
         * @brief Gets the size of the vertex buffer in bytes
         * @return Size of the buffer in bytes
         */
        [[nodiscard]] VkDeviceSize GetBufferSize() const { return internalBuffer.size; }
        
        /**
         * @brief Gets the number of vertices in the buffer
         * @return Count of vertices
         */
        [[nodiscard]] size_t GetVertexCount() const { return vertices.size(); }
        
        /**
         * @brief Sets new vertex data, replacing existing data
         * 
         * @param newVertices Vector of new vertices
         * @param recreateBuffer Whether to recreate the buffer immediately
         */
        void SetData(const std::vector<Vertex>& newVertices, bool recreateBuffer = true);
        
        /**
         * @brief Adds vertices to the buffer
         * 
         * @param additionalVertices Vector of vertices to add
         * @param recreateBuffer Whether to recreate the buffer immediately
         */
        void AppendData(const std::vector<Vertex>& additionalVertices, bool recreateBuffer = true);
        
        /**
         * @brief Updates a subset of vertices in the buffer
         * 
         * @param startIndex Starting index to update
         * @param updatedVertices Vector of vertices with new data
         */
        void UpdateData(uint32_t startIndex, const std::vector<Vertex>& updatedVertices);
        
        /**
         * @brief Clears all vertex data
         * 
         * @param releaseBuffer Whether to also release the GPU buffer
         */
        void ClearData(bool releaseBuffer = false);
        
        /**
         * @brief Resizes the buffer to accommodate a specific number of vertices
         * 
         * @param newCapacity The new capacity in vertices
         * @param preserveData Whether to preserve existing vertex data
         */
        void Reserve(uint32_t newCapacity, bool preserveData = true);
        
        /**
         * @brief Maps the buffer memory for CPU access (only for dynamic buffers)
         * @return Pointer to mapped memory, or nullptr if mapping fails
         */
        void* MapMemory();
        
        /**
         * @brief Unmaps the buffer memory after CPU access
         */
        void UnmapMemory();
        
        /**
         * @brief Gets the binding description for this vertex buffer
         * 
         * @param binding Binding index to use
         * @param inputRate Vertex input rate (vertex or instance)
         * @return VkVertexInputBindingDescription structure
         */
        [[nodiscard]] VkVertexInputBindingDescription GetBindingDescription(
            uint32_t binding = 0,
            VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX) const;
            
        /**
         * @brief Gets attribute descriptions for this vertex buffer
         * 
         * @param binding The binding index these attributes are associated with
         * @return Array of VkVertexInputAttributeDescription structures
         */
        [[nodiscard]] std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions(uint32_t binding = 0) const;

        /**
         * @brief Creates a vertex buffer representing a primitive geometry
         * 
         * @param type Type of primitive shape to create
         * @param size Size of the primitive
         * @param color Color to apply to the primitive vertices
         * @return Ref<VertexBuffer> Reference to the created vertex buffer
         */
        static Ref<VertexBuffer> CreatePrimitive(
            PrimitiveType type, 
            const Vec3& size = Vec3(1.0f), 
            const Vec3& color = Vec3(1.0f));

    private:
        Ref<GraphicsEngine>* gfxEngine;						///< Pointer to the graphics engine reference
        Ref<MemoryAllocator> allocator;						///< Reference to the memory allocator
        std::vector<Vertex> vertices;						///< Storage for vertex data
        RenderData renderData;								///< Render data reference
        VertexBufferType bufferType;						///< Type of vertex buffer
        VertexFormat format;								///< Format of vertices
        VkBuffer vertexBuffer = VK_NULL_HANDLE;             ///< Handle to the Vulkan vertex buffer
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE; ///< Handle to the allocated memory for the vertex buffer
        VmaAllocation vertexBuffersAllocation;              ///< Allocation handle for uniform buffers
        Buffer internalBuffer;								///< Buffer wrapper 
        uint32_t capacity = 0;								///< Capacity in number of vertices
        bool isInitialized = false;							///< Whether the buffer has been initialized

        /**
         * @brief Initializes the vertex buffer with the given options
         */
        void Initialize();
        
        /**
         * @brief Releases resources associated with the buffer
         */
        void Release();
        
        /**
         * @brief Creates vertex attribute descriptions based on the vertex format
         */
        std::vector<VkVertexInputAttributeDescription> CreateAttributeDescriptions(uint32_t binding) const;
        Buffer CreateVertexBuffer() const;
    };

}

/// ----------------------------------------------------------

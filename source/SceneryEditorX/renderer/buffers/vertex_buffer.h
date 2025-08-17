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
#include <SceneryEditorX/renderer/vulkan/vk_allocator.h>
#include <SceneryEditorX/renderer/vulkan/vk_data.h>

/// ----------------------------------------------------------

namespace SceneryEditorX
{
    /// Forward Declaration
    enum class PrimitiveType : uint8_t;

    /**
     * @class VertexBuffer
     * @brief Manages vertex data storage and configuration in Vulkan.
     *
     * This class handles the creation and management of vertex buffers in Vulkan,
     * including memory allocation and vertex attribute descriptions. It provides
     * functionality for storing and transferring vertex data to the GPU.
     */
    class VertexBuffer : public RefCounted
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
             * @brief Constructor with position.
             *
             * @param position 3D position of the vertex.
             */
            Vertex(const Vec3& position) : pos(position), color(1.0f, 1.0f, 1.0f), texCoord(0.0f, 0.0f) {}

            /**
             * @brief Constructor with position and color.
             *
             * @param position 3D position of the vertex.
             * @param vertexColor RGB color of the vertex.
             */
            Vertex(const Vec3& position, const Vec3& vertexColor) : pos(position), color(vertexColor), texCoord(0.0f, 0.0f) {}

            /**
             * @brief Constructor with position, color and texture coordinates.
             *
             * @param position 3D position of the vertex.
             * @param vertexColor RGB color of the vertex.
             * @param uv Texture coordinates of the vertex.
             */
            Vertex(const Vec3& position, const Vec3& vertexColor, const Vec2& uv) : pos(position), color(vertexColor), texCoord(uv) {}

            /**
             * @brief Provides the vertex binding description for Vulkan.
             *
             * @param binding Binding index to use.
             * @param inputRate Vertex input rate (vertex or instance).
             * @return VkVertexInputBindingDescription describing how to bind vertex data.
             */
            static VkVertexInputBindingDescription GetBindingDescription( uint32_t binding = 0, VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX)
            {
                VkVertexInputBindingDescription bindingDescription{};
                bindingDescription.binding = binding;
                bindingDescription.stride = sizeof(Vertex);
                bindingDescription.inputRate = inputRate;
                return bindingDescription;
            }

            /**
             * @brief Provides attribute descriptions for vertex data components.
             *
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
             *
             * @param other The vertex to compare against.
             * @return true if vertices are equal, false otherwise.
             */
            bool operator==(const Vertex &other) const
            {
                return pos == other.pos && color == other.color && texCoord == other.texCoord;
            }
        };

		/**
		 * @brief Constructor for VertexBuffer with allocation size only.
		 *
		 * Creates a vertex buffer with the specified size in bytes.
		 * Uses default vertex format and dynamic buffer type.
		 *
		 * @param size Size of the buffer in bytes.
		 */
		VertexBuffer(uint64_t size);

        /**
         * @brief Constructor for VertexBuffer.
         *
         * @param type The type of vertex buffer (Static, Dynamic, etc.)
         * @param vertexFormat The format of vertices to be stored.
         * @param initialCapacity Initial buffer capacity in vertices (optional).
         */
        VertexBuffer(VertexBufferType type, VertexFormat vertexFormat, uint32_t initialCapacity = 0);

        /**
         * @brief Constructor for VertexBuffer with initial data.
         *
         * @param initialVertices Vector of vertices to initialize the buffer with.
         * @param type The type of vertex buffer (Static, Dynamic, etc.).
         */
        VertexBuffer(const std::vector<Vertex>& initialVertices,VertexBufferType type = VertexBufferType::Static);

		/**
		 * @brief Constructor for VertexBuffer with raw data.
		 *
		 * @param data Pointer to vertex data.
		 * @param size Size of the data in bytes.
		 * @param usage Buffer usage type (default: Static).
		 */
        VertexBuffer(const void *data, uint64_t size, VertexBufferType usage = VertexBufferType::Static);

        /**
         * @brief Destructor for VertexBuffer
         *
         * Releases the Vulkan buffer and associated memory.
         * This is a virtual destructor to ensure proper cleanup in derived classes.
         *
         */
        virtual ~VertexBuffer() override;

        /**
         * @brief Sets the vertex data in the buffer.
         *
         * @param buffer Pointer to the vertex data buffer.
         * @param size Size of the data in bytes.
         * @param offset Offset in bytes where the data should be written (default: 0).
         */
        virtual void SetData(void* buffer, uint64_t size, uint64_t offset = 0);

        /**
         * @brief Sets the render thread data for the vertex buffer.
         *
         * @param buffer Pointer to the data buffer.
         * @param size Size of the data in bytes.
         * @param offset Offset in bytes where the data should be written (default: 0).
         */
        virtual void Set_RenderThreadData(void* buffer, uint64_t size, uint64_t offset = 0);

        /**
         * @brief Binds the vertex buffer for rendering.
         */
        virtual void Bind() const {}

        /**
         * @brief Unbinds the vertex buffer.
         * @return True if the buffer was successfully unbound, false otherwise.
         */
        virtual uint64_t GetSize() const { return m_Size; }

        /**
         * @brief Gets the renderer ID of the vertex buffer.
         *
         * @return Renderer ID of the vertex buffer.
         */
        virtual uint32_t GetRendererID() const { return 0; }

        /**
         * @brief Creates vertex attribute descriptions based on the vertex format
         */
        std::vector<VkVertexInputAttributeDescription> CreateAttributeDescriptions(uint32_t binding) const;

        /**
         * @brief Static factory method to create a vertex buffer with raw data
         *
         * This method creates a vertex buffer from raw data, allowing for immediate GPU upload.
         *
         * @param data Pointer to vertex data
         * @param size Size of the data in bytes
         * @param usage Buffer usage type (default: Static)
         * @param debugName Optional debug name for the vertex buffer
         * @return Ref<VertexBuffer> Smart pointer to the created vertex buffer
         */
        static Ref<VertexBuffer> Create(const void* data, uint64_t size, VertexBufferType usage = VertexBufferType::Static, const std::string& debugName = "");

        /**
         * @brief Gets the Vulkan buffer handle
         *
         * @return VkBuffer handle to the vertex buffer
         */
        [[nodiscard]] VkBuffer GetBuffer() const { return m_VertexBuffer.resource->buffer; }

        /**
         * @brief Gets the size of the vertex buffer in bytes
         * @return Size of the buffer in bytes
         */
        [[nodiscard]] uint64_t GetBufferSize() const { return m_VertexBuffer.size; }

        /**
         * @brief Gets the number of vertices in the buffer
         *
         * @return Count of vertices
         */
        [[nodiscard]] size_t GetVertexCount() const { return m_Vertices.size(); }

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
         * @brief Gets the binding description for this vertex buffer
         *
         * @param binding Binding index to use
         * @param inputRate Vertex input rate (vertex or instance)
         * @return VkVertexInputBindingDescription structure
         */
        [[nodiscard]] VkVertexInputBindingDescription GetBindingDescription(uint32_t binding = 0, VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX) const;

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
         * This method creates a vertex buffer containing the geometric data for the specified
         * primitive type. It leverages helper functions to generate the mesh data in the correct
         * VertexBuffer::Vertex format and creates a GPU-ready buffer.
         *
         * The method provides a convenient way to generate common geometric primitives that can
         * be used for 3D scene objects, UI elements, or debugging visualization.
         *
         * @param type Type of primitive shape to create (Cube, Sphere, Cylinder, Plane)
         * @param size Size of the primitive - interpretation depends on primitive type:
         *             - Cube: Vec3(width, height, depth) - all components used for box dimensions
         *             - Sphere: Vec3(radius, 0, 0) - only x component used as radius
         *             - Cylinder: Vec3(radius, height, 0) - x=radius, y=height
         *             - Plane: Vec3(width, height, 0) - x=width, y=height (z ignored)
         * @param color Color to apply to all primitive vertices (default: white Vec3(1,1,1))
         * @return Ref<VertexBuffer> Reference to the created vertex buffer, or nullptr if creation failed
         *
         * @note - The generated vertices include position, color, and texture coordinates.
         *       All primitives are centered at the origin with the specified dimensions.
         * @note - For best performance, consider caching returned vertex buffers for reuse.
         *
         * @example Usage:
         * @code
         * // Create a cube with dimensions 2x3x4
         * auto cubeBuffer = VertexBuffer::CreatePrimitive(PrimitiveType::Cube, Vec3(2.0f, 3.0f, 4.0f));
         * 
         * // Create a red sphere with radius 1.5
         * auto sphereBuffer = VertexBuffer::CreatePrimitive(PrimitiveType::Sphere, Vec3(1.5f), Vec3(1.0f, 0.0f, 0.0f));
         * 
         * // Create a plane for ground representation
         * auto groundBuffer = VertexBuffer::CreatePrimitive(PrimitiveType::Plane, Vec3(10.0f, 10.0f, 0.0f));
         * @endcode
         */
        static Ref<VertexBuffer> CreatePrimitive(PrimitiveType type, const Vec3& size = Vec3(1.0f), const Vec3& color = Vec3(1.0f));

    private:
        std::vector<Vertex> m_Vertices;						///< Storage for vertex data
        VertexBufferType m_BufferType;						///< Type of vertex buffer
        VertexFormat m_Format;								///< Format of vertices
        Buffer m_VertexBuffer;								///< Vulkan buffer wrapper using project buffer system
        Buffer m_LocalData;									///< Local data buffer for CPU access
        uint32_t m_Capacity = 0;							///< Capacity in number of vertices
        uint64_t m_Size = 0;								///< Size of the vertex buffer in bytes
        bool m_IsInitialized = false;						///< Whether the buffer has been initialized

        /**
         * @brief Creates the internal GPU buffer using the MemoryAllocator system
         */
        void CreateVertexBuffer();

        /**
         * @brief Uploads vertex data to the GPU buffer using the MemoryAllocator
         */
        void UploadVertexData();

        /**
         * @brief Uploads partial vertex data to the GPU buffer with offset support
         *
         * @param offset Byte offset in the buffer
         * @param size Number of bytes to upload
         */
        void UploadPartialVertexData(uint64_t offset, uint64_t size) const;

        /// ---- Primitive Generation Helper Methods ----

        /**
         * @brief Generates vertices for a cube primitive
         * @param size Dimensions of the cube (width, height, depth)
         * @param color Color to apply to all vertices
         * @return Vector of vertices representing the cube
         */
        static std::vector<Vertex> GenerateCubeVertices(const Vec3& size, const Vec3& color);

        /**
         * @brief Generates vertices for a sphere primitive
         * @param radius Radius of the sphere
         * @param color Color to apply to all vertices
         * @return Vector of vertices representing the sphere
         */
        static std::vector<Vertex> GenerateSphereVertices(float radius, const Vec3& color);

        /**
         * @brief Generates vertices for a cylinder primitive
         * @param radius Radius of the cylinder's circular base
         * @param height Height of the cylinder
         * @param color Color to apply to all vertices
         * @return Vector of vertices representing the cylinder
         */
        static std::vector<Vertex> GenerateCylinderVertices(float radius, float height, const Vec3& color);

        /**
         * @brief Generates vertices for a plane primitive
         * @param size Dimensions of the plane (width, height)
         * @param color Color to apply to all vertices
         * @return Vector of vertices representing the plane
         */
        static std::vector<Vertex> GeneratePlaneVertices(const Vec2& size, const Vec3& color);

        /**
         * @brief Memory allocator instance for this vertex buffer
         */
        mutable Ref<MemoryAllocator> m_MemoryAllocator;
    };

    /// ----------------------------------------------------------

    /**
     * @brief Returns the size in bytes of a given shader data type.
     *
     * @param type The shader data type for which to get the size.
     * @return Size in bytes of the specified shader data type.
     */
    static uint32_t ShaderDataTypeSize(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:		return 4;
			case ShaderDataType::Float2:	return 4 * 2;
			case ShaderDataType::Float3:	return 4 * 3;
			case ShaderDataType::Float4:	return 4 * 4;
			case ShaderDataType::Mat3:		return 4 * 3 * 3;
			case ShaderDataType::Mat4:		return 4 * 4 * 4;
			case ShaderDataType::Int:		return 4;
			case ShaderDataType::Int2:		return 4 * 2;
			case ShaderDataType::Int3:		return 4 * 3;
			case ShaderDataType::Int4:		return 4 * 4;
			case ShaderDataType::Bool:		return 1;
            case ShaderDataType::None:		break;
        }

		SEDX_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}

    /**
     * @brief Represents a single element in a vertex buffer layout.
     */
    struct VertexBufferElement
	{
        std::string name;       ///< Name of the element, typically used in shaders.
        ShaderDataType type;    ///< Shader data type of the element.
        uint32_t size;          ///< Size of the element in bytes, calculated based on the shader data type.
        uint32_t offset;        ///< Offset of the element in the vertex buffer, used for layout calculations.
		bool normalized;        ///< Whether the data should be normalized when accessed in shaders.

        VertexBufferElement() = default;

        /**
         * @brief Constructs a VertexBufferElement with specified type, name, and normalization.
         *
         * @param type The shader data type of the element.
         * @param name The name of the element, typically used in shaders.
         * @param normalized Whether the data should be normalized when accessed in shaders.
         */
        VertexBufferElement(const ShaderDataType type, std::string name, const bool normalized = false) :
            name(std::move(name)), type(type), size(ShaderDataTypeSize(type)), offset(0), normalized(normalized) {}

        /**
         * @brief Returns the number of components in this shader data type.
         *
         * @return Number of components in the shader data type.
         */
        [[nodiscard]] uint32_t GetComponentCount() const
		{
			switch (type)
			{
				case ShaderDataType::Float:   return 1;
				case ShaderDataType::Float2:  return 2;
				case ShaderDataType::Float3:  return 3;
				case ShaderDataType::Float4:  return 4;
				case ShaderDataType::Mat3:    return 3 * 3;
				case ShaderDataType::Mat4:    return 4 * 4;
				case ShaderDataType::Int:     return 1;
				case ShaderDataType::Int2:    return 2;
				case ShaderDataType::Int3:    return 3;
				case ShaderDataType::Int4:    return 4;
				case ShaderDataType::Bool:    return 1;
            case ShaderDataType::None:
                break;
            }

			SEDX_CORE_ASSERT(false, "Unknown ShaderDataType!");
			return 0;
		}
	};

    /**
     * @brief Represents a layout of vertex buffer elements.
     * This class is used to define the structure of vertex data
     * and how it should be interpreted by the graphics pipeline.
     */
    class VertexBufferLayout
	{
	public:
        VertexBufferLayout() = default;

		/**
		 * @brief Constructs a VertexBufferLayout with a list of elements.
		 *
		 * This constructor initializes the layout with the provided elements
		 * and calculates their offsets and total stride.
		 *
		 * @param elements Initializer list of VertexBufferElement objects.
		 * @note - The elements should be defined in the order they will be used in the vertex buffer.
		 * @example
		 * @code
		 * VertexBufferLayout layout = {
		 *	{ShaderDataType::Float3, "a_Position"},
		 *	{ShaderDataType::Float3, "b_Position"}
		 * }
		 * @endcode
		 */
		VertexBufferLayout(const std::initializer_list<VertexBufferElement> &elements) : m_Elements(elements)
		{
			CalculateOffsetsAndStride();
		}

        [[nodiscard]] uint32_t GetStride() const { return m_Stride; }
        [[nodiscard]] const std::vector<VertexBufferElement> &GetElements() const { return m_Elements; }
		[[nodiscard]] uint32_t GetElementCount() const { return (uint32_t)m_Elements.size(); }

		[[nodiscard]] std::vector<VertexBufferElement>::iterator begin() { return m_Elements.begin(); }
		[[nodiscard]] std::vector<VertexBufferElement>::iterator end() { return m_Elements.end(); }
		[[nodiscard]] std::vector<VertexBufferElement>::const_iterator begin() const { return m_Elements.begin(); }
		[[nodiscard]] std::vector<VertexBufferElement>::const_iterator end() const { return m_Elements.end(); }
	private:

		/**
		 * @brief Calculates the offsets and stride of the vertex buffer elements.
		 * This function iterates through the elements and sets their offsets
		 * based on their sizes, also calculating the total stride of the layout.
		 */
		void CalculateOffsetsAndStride()
		{
			uint32_t offset = 0;
			m_Stride = 0;
			for (auto& element : m_Elements)
			{
				element.offset = offset;
				offset += element.size;
				m_Stride += element.size;
			}
		}

		std::vector<VertexBufferElement> m_Elements;
		uint32_t m_Stride = 0;
	};


}

/// ----------------------------------------------------------

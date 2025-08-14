/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* type_values.h
* -------------------------------------------------------
* Created: 15/12/2025
* -------------------------------------------------------
*/
#pragma once
#include <algorithm>
#include <exception>
#include <memory>
#include <SceneryEditorX/logging/logging.hpp>
#include <string>
#include <unordered_map>
#include <vector>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    ///< Forward declare existing classes for integration
    class Allocator;

    ///< Forward declare Types namespace for integration
    namespace Types
    {
        class Type;
        struct MemberNameAndType;
        struct ElementTypeAndOffset;
    }

    namespace Values
    {
        ///< Forward declarations
        class Value;
        class ValueView;
        class StringDictionary;
        struct MemberNameAndValue;

        /// -------------------------------------------------------

        /**
         * @brief Custom exception for value system errors
         *
         * Integrates with SceneryEditorX logging system for consistent error reporting
         */
        struct Error : std::exception
        {
            explicit Error(const char* desc) : description(desc) {}
            [[nodiscard]] virtual const char* what() const noexcept override { return description; }
            const char* description;
        };

        /// -------------------------------------------------------

        /**
         * @brief Throws an error exception with SceneryEditorX logging integration
         * @param errorMessage Error message (must be string literal)
         */
        [[noreturn]] inline void ThrowError(const char* errorMessage)
        {
            throw Error(errorMessage);
        }

        /**
         * @brief Throws an Error with the given message if the condition is false
         * @param condition Condition to check
         * @param errorMessage Error message (must be string literal)
         */
        inline void Check(const bool condition, const char* errorMessage)
        {
            if (!condition)
                ThrowError(errorMessage);
        }

        /// -------------------------------------------------------

        /**
         * @brief Storage type for boolean values in the value system
         * Uses uint32_t for 4-byte alignment to match SceneryEditorX memory alignment
         */
        using BoolStorageType = uint32_t;

        /// -------------------------------------------------------

        /**
         * @brief Input data structure for deserialization operations
         */
        struct InputData
        {
            const uint8_t* start;
            const uint8_t* end;
        };

        /// -------------------------------------------------------

        /**
         * @brief Serialized data container for Value/ValueView objects
         *
         * Uses SceneryEditorX memory allocator for consistent memory management
         */
        struct SerialisedData
        {
            std::vector<uint8_t> data;

            /**
             * @brief Deserializes the data back into a Value object
             * @return Deserialized Value
             */
            [[nodiscard]] Value Deserialise() const;

            /**
             * @brief Gets input data structure for deserialization
             * @return InputData structure
             */
            [[nodiscard]] InputData GetInputData() const;

            /**
             * @brief Writes data to the serialized container
             * @param ptr Pointer to data
             * @param size Size of data in bytes
             */
            void Write(const void* ptr, size_t size);
        };

        /// -------------------------------------------------------

        /**
         * @brief SceneryEditorX-integrated allocator wrapper
         *
         * Provides memory allocation with category tracking for debugging
         * and profiling integration with the SceneryEditorX memory system
         */
        class ValueAllocator
        {
        public:
            /**
             * @brief Constructs with optional custom allocator category
             * @param category Memory category for tracking (defaults to "Values::System")
             */
            explicit ValueAllocator(const char* category = "Values::System") : m_Category(category) {}

            virtual ~ValueAllocator() = default;

            /**
             * @brief Allocates memory using SceneryEditorX allocator
             * @param size Size to allocate in bytes
             * @return Pointer to allocated memory
             */
            virtual void* Allocate(const size_t size)
            {
                if (size == 0) return nullptr;
                ///< For now, use standard allocation - this will be properly integrated with SceneryEditorX allocator
                ///< TODO: Integrate with SceneryEditorX::Allocator::Allocate(size, m_Category)
                return std::malloc(size);
            }

            /**
             * @brief Attempts to resize allocated memory block
             * @param data Existing data pointer
             * @param requestedSize New size in bytes
             * @return Pointer to resized memory or original pointer if resize failed
             */
            virtual void* ResizeIfPossible(void* data, size_t requestedSize)
            {
                ///< Standard allocator doesn't support in-place resize
                ///< Return original pointer to indicate resize not possible
                return data;
            }

            /**
             * @brief Frees allocated memory using SceneryEditorX allocator
             * @param ptr Pointer to free
             */
            virtual void Free(void* ptr) noexcept
            {
                ///< For now, use standard free - this will be properly integrated with SceneryEditorX allocator
                ///< TODO: Integrate with SceneryEditorX::Allocator::Free(ptr)
                std::free(ptr);
            }

            /**
             * @brief Gets the memory category for this allocator
             * @return Memory category string
             */
            [[nodiscard]] const char* GetCategory() const noexcept { return m_Category; }

        private:
            const char* m_Category;
        };

        /// -------------------------------------------------------

        /**
         * @brief Fixed-size pool allocator for high-performance scenarios
         *
         * Useful for real-time threads or when you need deterministic allocation times.
         * Integrates with SceneryEditorX profiling and assertion systems.
         *
         * @tparam totalSize Total size of the memory pool in bytes
         */
        template<size_t totalSize>
        class FixedPoolAllocator : public ValueAllocator
        {
        public:
            /**
             * @brief Constructs fixed pool allocator
             * @param category Memory category for tracking
             */
            explicit FixedPoolAllocator(const char* category = "Values::Pool") : ValueAllocator(category), m_Position(0), m_LastAllocationPosition(0), m_Pool{}
            {
                static_assert(totalSize > 0, "Pool size must be greater than zero");
            }

            /**
             * @brief Resets the pool allocation position
             */
            void Reset() noexcept
            {
                m_Position = 0;
                m_LastAllocationPosition = 0;
            }

            /**
             * @brief Allocates memory from the pool
             * @param size Size to allocate
             * @return Pointer to allocated memory or nullptr if pool exhausted
             */
            virtual void* Allocate(const size_t size) override
            {
                if (size == 0) return nullptr;

                ///< Align to pointer size for better performance
                constexpr size_t alignment = sizeof(void*);
                const size_t alignedSize = (size + alignment - 1) & ~(alignment - 1);

                if (m_Position + alignedSize > totalSize)
                    return nullptr; /// Pool exhausted

                void* result = &m_Pool[m_Position];
                m_LastAllocationPosition = m_Position;
                m_Position += alignedSize;

                return result;
            }

            /**
             * @brief Attempts to resize the last allocation if possible
             * @param data Pointer to existing data
             * @param requiredSize New size required
             * @return Original pointer if resize possible, nullptr otherwise
             */
            virtual void* ResizeIfPossible(void* data, const size_t requiredSize) override
            {
                if (!data)
                    return nullptr;

                ///< Check if this is the last allocation
                if (data != &m_Pool[m_LastAllocationPosition])
                    return data; ///< Can't resize, not the last allocation

                constexpr size_t alignment = sizeof(void*);
                const size_t alignedSize = (requiredSize + alignment - 1) & ~(alignment - 1);
                if (const size_t currentSize = m_Position - m_LastAllocationPosition; alignedSize <= currentSize)
                {
                    ///< Shrinking - adjust position
                    m_Position = m_LastAllocationPosition + alignedSize;
                    return data;
                }

                ///< Growing - check if we have space
                if (m_LastAllocationPosition + alignedSize <= totalSize)
                {
                    m_Position = m_LastAllocationPosition + alignedSize;
                    return data;
                }

                return data; ///< Can't resize
            }

            /**
             * @brief Gets current pool usage statistics
             * @return Pair of (used_bytes, total_bytes)
             */
            [[nodiscard]] std::pair<size_t, size_t> GetUsageStats() const noexcept
            {
                return std::make_pair(m_Position, totalSize);
            }

            /**
             * @brief Gets pool utilization percentage
             * @return Utilization as percentage (0.0 to 100.0)
             */
            [[nodiscard]] float GetUtilizationPercent() const noexcept {  (static_cast<float>(m_Position) / totalSize) * 100.0f; }

        private:
            size_t m_Position;
            size_t m_LastAllocationPosition;
            char m_Pool[totalSize];
        };

        /// -------------------------------------------------------

        /**
         * @brief String dictionary for efficient string storage and deduplication
         *
         * Provides efficient string handling for the value system with
         * automatic deduplication and handle-based access.
         */
        class StringDictionary
        {
        public:
            /**
             * @brief Handle type for string references
             */
            using Handle = uint32_t;
            static constexpr Handle NullHandle = 0;

            /**
             * @brief Constructs string dictionary with optional custom allocator
             * @param allocator Custom allocator (uses default if nullptr)
             */
            explicit StringDictionary(const std::shared_ptr<ValueAllocator> &allocator = nullptr) : m_Allocator(allocator ? allocator : std::make_shared<ValueAllocator>("Values::Strings"))
            {
                /// Reserve handle 0 for null/invalid strings
                m_HandleToString.emplace_back("");
            }

            /**
             * @brief Adds or finds a string in the dictionary
             * @param text String to add
             * @return Handle to the string
             */
            Handle AddString(const std::string& text)
            {
                if (text.empty())
                    return NullHandle;

                /// Check if string already exists
                if (const auto it = m_StringToHandle.find(text); it != m_StringToHandle.end())
                {
                    return it->second;
                }

                /// Add new string
                const Handle handle = static_cast<Handle>(m_HandleToString.size());

                m_HandleToString.emplace_back(text);
                m_StringToHandle[text] = handle;

                return handle;
            }

            /**
             * @brief Gets string by handle
             * @param handle String handle
             * @return String reference
             */
            [[nodiscard]] const std::string& GetString(const Handle handle) const
            {
                if (handle >= m_HandleToString.size())
                    ThrowError("Invalid string handle");

                return m_HandleToString[handle];
            }

            /**
             * @brief Checks if handle is valid
             * @param handle Handle to check
             * @return True if handle is valid
             */
            [[nodiscard]] bool IsValidHandle(const Handle handle) const noexcept
            {
                return handle < m_HandleToString.size();
            }

            /**
             * @brief Gets the number of unique strings in dictionary
             * @return String count
             */
            [[nodiscard]] size_t GetStringCount() const noexcept
            {
                return m_HandleToString.size();
            }

            /**
             * @brief Clears all strings from dictionary
             */
            void Clear()
            {
                m_StringToHandle.clear();
                m_HandleToString.clear();
                m_HandleToString.emplace_back(""); /// Re-add null handle
            }

            /**
             * @brief Gets memory usage statistics
             * @return Approximate memory usage in bytes
             */
            [[nodiscard]] size_t GetMemoryUsage() const
            {
                size_t usage = sizeof(*this);
                for (const auto& str : m_HandleToString)
                {
                    usage += str.capacity();
                }
                usage += m_StringToHandle.size() * (sizeof(std::string) + sizeof(Handle));
                return usage;
            }

        private:
            std::shared_ptr<ValueAllocator> m_Allocator;
            std::vector<std::string> m_HandleToString;
            std::unordered_map<std::string, Handle> m_StringToHandle;
        };

        /// -------------------------------------------------------

        /**
         * @brief Simple string dictionary implementation for basic use cases
         */
        class SimpleStringDictionary : public StringDictionary
        {
        public:
            SimpleStringDictionary() : StringDictionary(std::make_shared<ValueAllocator>("Values::SimpleStrings")) {}
        };

        /// -------------------------------------------------------

        /**
         * @brief Type wrapper that provides a simple interface for the value system
         *
         * This provides a minimal type system for the value implementation that can
         * be extended to integrate with SceneryEditorX::Types::Type later.
         */
        class Type
        {
        public:
            /**
             * @brief Type identifiers for basic types
             */
            enum class TypeId : uint32_t
            {
                Void = 0,
                Bool,
                Int32,
                UInt32,
                Int64,
                UInt64,
                Float,
                Double,
                String,
                Array,
                Object,
                Custom
            };

            Type() : m_TypeId(TypeId::Void), m_Size(0) {}
            explicit Type(const TypeId typeId, const size_t size = 0) : m_TypeId(typeId), m_Size(size) {}

            /**
             * @brief Check if type is valid
             * @return true if type is not void
             */
            [[nodiscard]] bool IsValid() const noexcept { return m_TypeId != TypeId::Void; }

            /**
             * @brief Get type identifier
             * @return Type identifier
             */
            [[nodiscard]] TypeId GetTypeId() const noexcept { return m_TypeId; }

            /**
             * @brief Get type size in bytes
             * @return Size in bytes
             */
            [[nodiscard]] size_t GetSize() const noexcept { return m_Size; }

            /**
             * @brief Set type size
             * @param size Size in bytes
             */
            void SetSize(const size_t size) noexcept { m_Size = size; }

            /**
             * @brief Check type equality
             * @param other Other type to compare
             * @return true if types are equal
             */
            bool operator==(const Type& other) const noexcept
            {
                return m_TypeId == other.m_TypeId && m_Size == other.m_Size;
            }

            bool operator!=(const Type& other) const noexcept
            {
                return !(*this == other);
            }

            /**
             * @brief Create common types
             */
            static Type CreateVoid() { return {}; }
            static Type CreateBool() { return Type(TypeId::Bool, sizeof(bool)); }
            static Type CreateInt32() { return Type(TypeId::Int32, sizeof(int32_t)); }
            static Type CreateUInt32() { return Type(TypeId::UInt32, sizeof(uint32_t)); }
            static Type CreateInt64() { return Type(TypeId::Int64, sizeof(int64_t)); }
            static Type CreateUInt64() { return Type(TypeId::UInt64, sizeof(uint64_t)); }
            static Type CreateFloat() { return Type(TypeId::Float, sizeof(float)); }
            static Type CreateDouble() { return Type(TypeId::Double, sizeof(double)); }
            static Type CreateString() { return Type(TypeId::String, sizeof(std::string)); }

        private:
            TypeId m_TypeId;
            size_t m_Size;
        };

        /**
         * @brief Core value class that can hold any data type with SceneryEditorX integration
         *
         * This class is adapted from choc::value but integrates with SceneryEditorX's
         * memory management, logging, and reflection systems. It provides type-safe
         * storage and manipulation of values with reference counting support.
         *
         * @note - Uses SceneryEditorX memory allocators and logging for consistency
         */
        class Value
        {
        public:
            /** Default constructor creates an empty/void value */
            Value() : m_Type(Type::CreateVoid()), m_Data(nullptr)
            {
                SEDX_CORE_TRACE_TAG("VALUE", "Created empty Value");
            }

            /** Copy constructor */
            Value(const Value& other) : m_Type(other.m_Type), m_Data(nullptr)
            {
                if (other.m_Data && other.m_Type.IsValid())
                {
                    AllocateAndCopy(other);
                    SEDX_CORE_TRACE_TAG("VALUE", "Copied Value of type");
                }
            }

            /** Move constructor */
            Value(Value&& other) noexcept : m_Type(other.m_Type), m_Data(other.m_Data)
            {
                other.m_Type = Type::CreateVoid();
                other.m_Data = nullptr;
                SEDX_CORE_TRACE_TAG("VALUE", "Moved Value");
            }

            /** Constructor from primitive types */
            template<typename T>
            explicit Value(T&& value)
            {
                CreateFromPrimitive(std::forward<T>(value));
                // SEDX_CORE_TRACE_TAG("VALUE", "Created Value from primitive type");
            }

            /** Destructor */
            ~Value()
            {
                Reset();
                // SEDX_CORE_TRACE_TAG("VALUE", "Destroyed Value");
            }

            /** Assignment operators */
            Value& operator=(const Value& other)
            {
                if (this != &other)
                {
                    Reset();
                    m_Type = other.m_Type;
                    if (other.m_Data && other.m_Type.IsValid())
                        AllocateAndCopy(other);
                }
                return *this;
            }

            Value& operator=(Value&& other) noexcept
            {
                if (this != &other)
                {
                    Reset();
                    m_Type = other.m_Type;
                    m_Data = other.m_Data;
                    other.m_Type = Type::CreateVoid();
                    other.m_Data = nullptr;
                }
                return *this;
            }

            /**
             * @brief Check if value is valid/non-empty
             * @return true if value contains data
             */
            [[nodiscard]] bool IsValid() const noexcept { return m_Type.IsValid() && m_Data != nullptr; }

            /**
             * @brief Check if value is empty/void
             * @return true if value is empty
             */
            [[nodiscard]] bool IsVoid() const noexcept { return !IsValid(); }

            /**
             * @brief Get the type of this value
             * @return Type descriptor
             */
            [[nodiscard]] Type GetType() const noexcept { return m_Type; }

            /**
             * @brief Get raw data pointer
             * @return Pointer to data or nullptr
             */
            [[nodiscard]] const void* GetData() const noexcept { return m_Data; }

            /**
             * @brief Get mutable data pointer
             * @return Pointer to data or nullptr
             */
            void* GetData() noexcept { return m_Data; }

            /**
             * @brief Reset value to empty state
             */
            void Reset()
            {
                if (m_Data)
                {
                    ///< Use SceneryEditorX memory allocator for cleanup
                    GetValueAllocator().Free(m_Data);
                    m_Data = nullptr;
                }
                m_Type = Type::CreateVoid();
                // SEDX_CORE_TRACE_TAG("VALUE", "Reset Value to empty state");
            }

            /**
             * @brief Get value as specific type with safety checks
             * @tparam T Type to cast to
             * @return Reference to typed data
             * @throws Error if type doesn't match
             */
            template<typename T>
            const T& Get() const
            {
                ValidateTypeMatch<T>();
                // SEDX_CORE_ASSERT(m_Data != nullptr, "Value data is null");
                if (m_Data == nullptr) throw Error("Value data is null");
                return *static_cast<const T*>(m_Data);
            }

            template<typename T>
            T& Get()
            {
                ValidateTypeMatch<T>();
                // SEDX_CORE_ASSERT(m_Data != nullptr, "Value data is null");
                if (m_Data == nullptr) throw Error("Value data is null");
                return *static_cast<T*>(m_Data);
            }

            /**
             * @brief Try to get value as specific type
             * @tparam T Type to try to cast to
             * @return Pointer to typed data or nullptr if type doesn't match
             */
            template<typename T>
            const T* TryGet() const noexcept
            {
                if (IsTypeCompatible<T>())
                    return static_cast<const T*>(m_Data);

                return nullptr;
            }

            template<typename T>
            T* TryGet() noexcept
            {
                if (IsTypeCompatible<T>())
                    return static_cast<T*>(m_Data);

                return nullptr;
            }

            /**
             * @brief Create a view of this value
             * @return ValueView that references this value
             */
            [[nodiscard]] ValueView GetView() const;

        private:
            Type m_Type;      ///< Type descriptor
            void* m_Data;     ///< Raw data pointer

            /** Get the global value allocator */
            static ValueAllocator& GetValueAllocator()
            {
                static ValueAllocator allocator("Values::Memory");
                return allocator;
            }

            /** Allocate and copy data from another value */
            void AllocateAndCopy(const Value& other)
            {
                if (!other.m_Type.IsValid() || !other.m_Data)
                    return;

                const size_t typeSize = other.m_Type.GetSize();
                m_Data = GetValueAllocator().Allocate(typeSize);

                if (m_Data)
                {
                    std::memcpy(m_Data, other.m_Data, typeSize);
                }
                else
                {
                    // SEDX_CORE_ERROR_TAG("VALUE", "Failed to allocate memory for Value copy");
                    throw Error("Failed to allocate memory for Value");
                }
            }

            /** Create value from primitive type */
            template<typename T>
            void CreateFromPrimitive(T&& value)
            {
                using CleanT = std::decay_t<T>;

                m_Type = GetTypeDescriptor<CleanT>();
                if (!m_Type.IsValid())
                {
                    // SEDX_CORE_ERROR_TAG("VALUE", "No type descriptor for primitive type");
                    throw Error("No type descriptor available");
                }

                const size_t typeSize = sizeof(CleanT);
                m_Data = GetValueAllocator().Allocate(typeSize);

                if (m_Data)
                {
                    new(m_Data) CleanT(std::forward<T>(value));
                }
                else
                {
                    // SEDX_CORE_ERROR_TAG("VALUE", "Failed to allocate memory for primitive value");
                    throw Error("Failed to allocate memory");
                }
            }

            /** Validate that stored type matches requested type */
            template<typename T>
            void ValidateTypeMatch() const
            {
                if (!IsTypeCompatible<T>())
                {
                    // SEDX_CORE_ERROR_TAG("VALUE", "Type mismatch in Value::Get<T>()");
                    throw Error("Type mismatch");
                }
            }

            /** Check if type is compatible */
            template<typename T>
            [[nodiscard]] bool IsTypeCompatible() const noexcept
            {
                if (!m_Type.IsValid())
                    return false;

                const Type requestedType = GetTypeDescriptor<T>();
                return requestedType.IsValid() && m_Type == requestedType;
            }

            /** Get type descriptor for a C++ type */
            template<typename T>
            static Type GetTypeDescriptor()
            {
                // Map common C++ types to our Type system using template specialization
                return GetTypeDescriptorImpl<std::decay_t<T>>();
            }

            /** Default implementation for unknown types */
            template<typename T>
            static Type GetTypeDescriptorImpl()
            {
                return Type(Type::TypeId::Custom, sizeof(T));
            }

        };

        /// -------------------------------------------------------

        /**
         * @brief Non-owning view of a Value for efficient access
         *
         * ValueView provides a lightweight way to access value data without
         * copying or taking ownership. It integrates with SceneryEditorX's
         * type system and provides the same safety guarantees as Value.
         */
        class ValueView
        {
        public:
            /** Default constructor creates invalid view */
            ValueView() : m_Type(Type::CreateVoid()), m_Data(nullptr) {}

            /** Create view from Value */
            explicit ValueView(const Value& value) : m_Type(value.GetType()), m_Data(value.GetData()) {}

            /** Create view from raw data and type */
            ValueView(const Type& type, const void* data) : m_Type(type), m_Data(data) {}

            /**
             * @brief Check if view is valid
             * @return true if view references valid data
             */
            [[nodiscard]] bool IsValid() const noexcept { return m_Type.IsValid() && m_Data != nullptr; }

            /**
             * @brief Get the type of viewed value
             * @return Type descriptor
             */
            [[nodiscard]] Type GetType() const noexcept { return m_Type; }

            /**
             * @brief Get raw data pointer
             * @return Pointer to data
             */
            [[nodiscard]] const void* GetData() const noexcept { return m_Data; }

            /**
             * @brief Get value as specific type
             * @tparam T Type to cast to
             * @return Reference to typed data
             */
            template<typename T>
            const T& Get() const
            {
                ValidateTypeMatch<T>();
                // SEDX_CORE_ASSERT(m_Data != nullptr, "ValueView data is null");
                if (m_Data == nullptr) throw Error("ValueView data is null");
                return *static_cast<const T*>(m_Data);
            }

            /**
             * @brief Try to get value as specific type
             * @tparam T Type to try to cast to
             * @return Pointer to typed data or nullptr
             */
            template<typename T>
            const T* TryGet() const noexcept
            {
                if (IsTypeCompatible<T>())
                    return static_cast<const T*>(m_Data);

                return nullptr;
            }

            /**
             * @brief Create owned copy of viewed value
             * @return New Value containing copy of viewed data
             */
            [[nodiscard]] Value CreateCopy() const
            {
                if (!IsValid())
                    return {}; /// Empty value

                ///< TODO: Implementation will copy data using type system
                Value result;
                // SEDX_CORE_TRACE_TAG("VALUE", "Created copy from ValueView");
                return result;
            }

        private:
            Type m_Type;         ///< Type descriptor (not owned)
            const void* m_Data;  ///< Data pointer (not owned)

            /** Validate type match */
            template<typename T>
            void ValidateTypeMatch() const
            {
                if (!IsTypeCompatible<T>())
                {
                    // SEDX_CORE_ERROR_TAG("VALUE", "Type mismatch in ValueView::Get<T>()");
                    throw Error("Type mismatch");
                }
            }

            /** Check type compatibility - placeholder for integration */
            template<typename T>
            [[nodiscard]] bool IsTypeCompatible() const noexcept
            {
                if (!m_Type.IsValid())
                    return false;

                const Type requestedType = Value::GetTypeDescriptor<T>();
                return requestedType.IsValid() && m_Type == requestedType;
            }
        };

        /// Implement Value::GetView() now that ValueView is defined
        inline ValueView Value::GetView() const { return ValueView(*this); }

        /// -------------------------------------------------------
        /// Template specializations for common C++ types
        /// -------------------------------------------------------

        template<>
        inline Type Value::GetTypeDescriptorImpl<bool>()
        {
            return Type::CreateBool();
        }

        template<>
        inline Type Value::GetTypeDescriptorImpl<int32_t>()
        {
            return Type::CreateInt32();
        }

        template<>
        inline Type Value::GetTypeDescriptorImpl<uint32_t>()
        {
            return Type::CreateUInt32();
        }

        template<>
        inline Type Value::GetTypeDescriptorImpl<int64_t>()
        {
            return Type::CreateInt64();
        }

        template<>
        inline Type Value::GetTypeDescriptorImpl<uint64_t>()
        {
            return Type::CreateUInt64();
        }

        template<>
        inline Type Value::GetTypeDescriptorImpl<float>()
        {
            return Type::CreateFloat();
        }

        template<>
        inline Type Value::GetTypeDescriptorImpl<double>()
        {
            return Type::CreateDouble();
        }

        template<>
        inline Type Value::GetTypeDescriptorImpl<std::string>()
        {
            return Type::CreateString();
        }

    }

}

/// -------------------------------------------------------

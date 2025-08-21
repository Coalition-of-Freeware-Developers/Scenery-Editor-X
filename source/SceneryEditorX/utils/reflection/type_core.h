/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* type_core.h
* -------------------------------------------------------
* Created: 19/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/memory/memory.h>

/// -------------------------------------------------------

namespace SceneryEditorX::Values
{
	class Value;
	class ValueView;
	class StringDictionary;
	struct MemberNameAndType;
	struct MemberNameAndValue;
	struct ElementTypeAndOffset;
}

namespace SceneryEditorX::Types
{

	/**
	 * @brief 
	 */
	class Type
	{
	public:
		Type() = default;
        Type(Type &&) noexcept;
		Type(const Type &);
        Type(Allocator* , const Type &);
        Type &operator=(Type &&) noexcept;
		Type &operator= (const Type &);
        ~Type() noexcept;

		explicit Type(const std::string_view &typeName) : Name(typeName) {}
		std::string Name;  ///< The name of the type
		int ID = -1;       ///< Unique identifier for the type, -1 if not set
        [[nodiscard]] bool IsValid() const { return !Name.empty(); }

	    /**
	     * If the type is an array or vector with a uniform element type, this returns it;
	     * if not, it throws an Error.
	     */
        [[nodiscard]] Type getElementType() const;

	    /**
	     * Returns the type of a given element in this type if it's an array.
	     * If the type isn't an array or the index is out of bounds, it will throw an Error.
	     */
        [[nodiscard]] Type getArrayElementType(uint32_t index) const;

	    /**
	     * For a vector or uniform array type, this allows the number of elements to be directly mutated.
	     * For any other type, this will throw an Error exception.
	     */
        void modifyNumElements(uint32_t newNumElements);

        /**
         * Returns the name and type of one of the members if this type is an object; if not, or the index is out
         * of range, then this will throw an Error exception.
         */
        [[nodiscard]] const Values::MemberNameAndType &getObjectMember(uint32_t index) const;

        /**
         * If this is an object, this returns the index of the member with a given name. If the name isn't found, it
         * will return -1, and if the type isn't an object, it will throw an Error exception.
         */
        [[nodiscard]] int getObjectMemberIndex(std::string_view name) const;

        /**
         * Returns the class-name of this type if it's an object, or throws an Error if it's not.
         */
        [[nodiscard]] std::string_view getObjectClassName() const;

        /**
         * Returns true if this is an object with the given class-name.
         */
        [[nodiscard]] bool isObjectWithClassName(std::string_view name) const;

		/// -------------------------------------------------------

        bool operator==(const Type &) const;
        bool operator!=(const Type &) const;

		/// -------------------------------------------------------

	    /**
	     * Creates a type representing an empty array. Element types can be appended with addArrayElements().
	     */
        static Type createEmptyArray();

        /**
         * Creates a type representing an array containing a set of elements of a fixed type.
         */
        static Type createArray(Type elementType, uint32_t numElements);

        /**
         * Creates a type representing an array of primitives based on the templated type.
         */
        template <typename PrimitiveType>
        static Type createArray(uint32_t numArrayElements);

        /**
         * Creates a type representing an array of vectors based on the templated type.
         */
        template <typename PrimitiveType>
        static Type createArrayOfVectors(uint32_t numArrayElements, uint32_t numVectorElements);

        /**
         * Appends a group of array elements with the given to this type's definition.
         * This will throw an Error if this isn't possible for various reasons.
         */
        void addArrayElements(Type elementType, uint32_t numElements);

	    /// -------------------------------------------------------

		/**
		 * Returns a type representing an empty object, with the given class name.
		 * @note - that the name must be a valid UTF8 string, and may not contain a null character.
		 */
        static Type createObject(std::string_view className, Allocator *allocator = nullptr);

        /**
         * Appends a member to an object type, with the given name and type. This will throw an Error if
         * this isn't possible for some reason.
         * @note - that the name must be a valid UTF8 string, and may not contain a null character.
         */
        void addObjectMember(std::string_view memberName, Type memberType);

	private:

        /// -------------------------------------------------------

        enum class MainType : uint8_t
        {
            void_          = 0,
            int32          = 0x00 + sizeof(int32_t),
            int64          = 0x00 + sizeof(int64_t),
            float32        = 0x10 + sizeof(float),
            float64        = 0x10 + sizeof(double),
            /*boolean        = 0x30 + sizeof(Values::BoolStorageType),*/
            string         = 0x40 + sizeof(uint32_t),
            vector         = 0x50,
            primitiveArray = 0x60,
            object         = 0x80, ///< these two must have the top bit set to make it quick
            complexArray   = 0x90  ///< to decide whether the content references a heap object
        };

	    static constexpr uint32_t maxNumVectorElements = 256;
	    static constexpr uint32_t maxNumArrayElements = 1024 * 1024;
	    static constexpr uint32_t getPrimitiveSize (MainType t)   { return static_cast<uint32_t> (t) & 15; }

	    friend class ValueView;
	    friend class Value;
	    struct SerialisationHelpers;
	    struct ComplexArray;
	    struct Object;

	    template <typename ObjectType>
	    struct AllocatedVector;

	    struct Vector
        {
            MainType elementType;
            uint32_t numElements;

            [[nodiscard]] size_t getElementSize() const;
            [[nodiscard]] size_t getValueDataSize() const;
            [[nodiscard]] Values::ElementTypeAndOffset getElementInfo(uint32_t) const;
            [[nodiscard]] Values::ElementTypeAndOffset getElementRangeInfo(uint32_t start, uint32_t length) const;
            bool operator==(const Vector &) const;
        };

        struct PrimitiveArray
        {
            MainType elementType;
            uint32_t numElements, numVectorElements;

            [[nodiscard]] Type getElementType() const;
            [[nodiscard]] size_t getElementSize() const;
            [[nodiscard]] size_t getValueDataSize() const;
            [[nodiscard]] Values::ElementTypeAndOffset getElementInfo(uint32_t) const;
            [[nodiscard]] Values::ElementTypeAndOffset getElementRangeInfo(uint32_t start, uint32_t length) const;
            bool operator==(const PrimitiveArray &) const;
        };

	    union Content
        {
            Object *object;
            ComplexArray *complexArray;
            Vector vector;
            PrimitiveArray primitiveArray;
        };

	    MainType mainType = MainType::void_;
        Content content = {};
        Allocator *allocator = nullptr;

	    template <typename... Types> bool isType (Types... types) const noexcept   { return ((mainType == types) || ...); }
        template <typename Type> static constexpr MainType selectMainType();

        explicit Type (MainType);
		Type (MainType, Content, Allocator*);
		Type (MainType vectorElementType, uint32_t);
		void allocateCopy (const Type&);
		void deleteAllocatedObjects() noexcept;

	    template <typename Visitor>
	    void visitStringHandles (size_t, const Visitor&) const;

		static Type createArray (Type elementType, uint32_t numElements, Allocator*);
	};

    /// -------------------------------------------------------

    /**
     * This holds the type and location of a sub-element of a Type.
     * @see Type::getElementTypeAndOffset()
	 */
    struct ElementTypeAndOffset
    {
        Type elementType;
        size_t offset; ///< The byte position within its parent value of the data representing this element
    };


}

/// -------------------------------------------------------

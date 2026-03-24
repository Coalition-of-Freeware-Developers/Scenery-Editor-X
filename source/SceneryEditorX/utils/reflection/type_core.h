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
 * type_core.h
 * -------------------------------------------------------
 * Created: 19/7/2025
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/core/memory/memory.h>

// -------------------------------------------------------

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
	 * @class Type
	 * @brief Represents a type in the Scenery Editor X type system, which can be a primitive, array, vector, or object type.
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
		std::string Name;  // The name of the type
		int ID = -1;       // Unique identifier for the type, -1 if not set
		[[nodiscard]] bool IsValid() const { return !Name.empty(); }

		/**
		 * If the type is an array or vector with a uniform element type, this returns it;
		 * if not, it throws an Error.
		 */
		[[nodiscard]] Type GetElementType() const;

		/**
		 * Returns the type of a given element in this type if it's an array.
		 * If the type isn't an array or the index is out of bounds, it will throw an Error.
		 */
		[[nodiscard]] Type GetArrayElementType(uint32_t index) const;

		/**
		 * For a vector or uniform array type, this allows the number of elements to be directly mutated.
		 * For any other type, this will throw an Error exception.
		 */
		void ModifyNumElements(uint32_t newNumElements);

		/**
		 * Returns the name and type of one of the members if this type is an object; if not, or the index is out
		 * of range, then this will throw an Error exception.
		 */
		[[nodiscard]] const Values::MemberNameAndType &GetObjectMember(uint32_t index) const;

		/**
		 * If this is an object, this returns the index of the member with a given name. If the name isn't found, it
		 * will return -1, and if the type isn't an object, it will throw an Error exception.
		 */
		[[nodiscard]] int GetObjectMemberIndex(std::string_view name) const;

		/**
		 * Returns the class-name of this type if it's an object, or throws an Error if it's not.
		 */
		[[nodiscard]] std::string_view GetObjectClassName() const;

		/**
		 * Returns true if this is an object with the given class-name.
		 */
		[[nodiscard]] bool IsObjectWithClassName(std::string_view name) const;

		// -------------------------------------------------------

		bool operator==(const Type &) const;
		bool operator!=(const Type &) const;

		// -------------------------------------------------------

		/**
		 * Creates a type representing an empty array. Element types can be appended with addArrayElements().
		 */
		static Type CreateEmptyArray();

		/**
		 * Creates a type representing an array containing a set of elements of a fixed type.
		 */
		static Type CreateArray(Type elementType, uint32_t numElements);

		/**
		 * Creates a type representing an array of primitives based on the templated type.
		 */
		template <typename PrimitiveType>
		static Type CreateArray(uint32_t numArrayElements);

		/**
		 * Creates a type representing an array of vectors based on the templated type.
		 */
		template <typename PrimitiveType>
		static Type CreateArrayOfVectors(uint32_t numArrayElements, uint32_t numVectorElements);

		/**
		 * Appends a group of array elements with the given to this type's definition.
		 * This will throw an Error if this isn't possible for various reasons.
		 */
		void AddArrayElements(Type elementType, uint32_t numElements);

		// -------------------------------------------------------

		/**
		 * Returns a type representing an empty object, with the given class name.
		 * @note - that the name must be a valid UTF8 string, and may not contain a null character.
		 */
		static Type CreateObject(std::string_view className, Allocator *allocator = nullptr);

		/**
		 * Appends a member to an object type, with the given name and type. This will throw an Error if
		 * this isn't possible for some reason.
		 * @note - that the name must be a valid UTF8 string, and may not contain a null character.
		 */
		void AddObjectMember(std::string_view memberName, Type memberType);

	private:
		/**
		 * @enum MainType
		 * @brief Represents the main types in the type system.
		 */
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
			object         = 0x80, // these two must have the top bit set to make it quick
			complexArray   = 0x90  // to decide whether the content references a heap object
		};

		static constexpr uint32_t MAX_NUM_VECTOR_ELEMENTS = 256;
		static constexpr uint32_t MAX_NUM_ARRAY_ELEMENTS = 1024 * 1024;
		static constexpr uint32_t GetPrimitiveSize (MainType t)   { return static_cast<uint32_t> (t) & 15; }

		friend class ValueView;
		friend class Value;
		struct SerialisationHelpers;
		struct ComplexArray;
		struct Object;

		template <typename ObjectType>
		struct AllocatedVector;

		/**
		 * @struct Vector
		 * @brief Represents a vector type in the type system.
		 */
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

		/**
		 * @struct PrimitiveArray
		 * @brief Represents an array of primitive types in the type system, where all elements have the same type and size.
		 */
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

		/**
		 * @union Content
		 * @brief Represents the content of a type, which can be an object, complex array, vector, or primitive array. 
		 */
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
		template <typename Type> static constexpr MainType SelectMainType();

		explicit Type (MainType);
		Type (MainType, Content, Allocator*);
		Type (MainType vectorElementType, uint32_t);
		void allocateCopy (const Type&);
		void deleteAllocatedObjects() noexcept;

		template <typename Visitor>
		void visitStringHandles (size_t, const Visitor&) const;

		static Type createArray (Type elementType, uint32_t numElements, Allocator*);
	};

	// -------------------------------------------------------

	/**
	 * This holds the type and location of a sub-element of a Type.
	 * @see Type::getElementTypeAndOffset()
	 */
	struct ElementTypeAndOffset
	{
		Type elementType;
		size_t offset; // The byte position within its parent value of the data representing this element
	};


}

// -------------------------------------------------------

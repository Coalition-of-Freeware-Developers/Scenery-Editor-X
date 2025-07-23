/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* type_elements.h
* -------------------------------------------------------
* Created: 19/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/utils/reflection/type_values.h>

/// ----------------------------------------------------------------------------

namespace SceneryEditorX::Values
{
	/**
	 * This holds the type and location of a sub-element of a Type.
	 * @see Type::getElementTypeAndOffset()
	 */
	struct ElementTypeAndOffset
	{
	    Type elementType;
	    size_t offset; ///< The byte position within its parent value of the data representing this element
	};


	inline ElementTypeAndOffset Type::PrimitiveArray::getElementRangeInfo (uint32_t start, uint32_t length) const
	{
	    Check (start < numElements && start + length <= numElements, "Illegal element range");
	
	    Content c;
	    c.primitiveArray = {
	        .elementType = elementType,
	        .numElements = length,
	        .numVectorElements = numVectorElements
	    };
	
	    return {
	        .elementType = Type (MainType::primitiveArray, c, nullptr),
	        .offset = start * getPrimitiveSize (elementType) * (numVectorElements != 0 ? numVectorElements : 1)
	    };
	}


	inline ElementTypeAndOffset Type::PrimitiveArray::getElementInfo (uint32_t index) const
	{
	    Check (index < numElements, "Index out of range");
	    auto primitiveSize = getPrimitiveSize (elementType);
	
	    if (numVectorElements != 0)
	        return { Type (elementType, numVectorElements), primitiveSize * numVectorElements * index };
	
	    return { Type (elementType), primitiveSize * index };
	}


    inline ElementTypeAndOffset getElementRangeInfo (Allocator* a, uint32_t start, uint32_t length)
    {
        ElementTypeAndOffset info { Type (MainType::complexArray), 0 };
        info.elementType.content.complexArray = allocateObject<ComplexArray> (a, a);
        auto& destGroups = info.elementType.content.complexArray->groups;

        for (auto& g : groups)
        {
            auto groupLen = g.repetitions;

            if (start >= groupLen)
            {
                start -= groupLen;
                info.offset += g.repetitions * g.elementType.getValueDataSize();
                continue;
            }

            if (start > 0)
            {
                groupLen -= start;
                info.offset += start * g.elementType.getValueDataSize();
                start = 0;
            }

            if (length <= groupLen)
            {
                destGroups.push_back ({ length, Type (a, g.elementType) });
                return info;
            }

            destGroups.push_back ({ groupLen, Type (a, g.elementType) });
            length -= groupLen;
        }

        check (start == 0 && length == 0, "Illegal element range");
        return info;
    }

    ElementTypeAndOffset getElementInfo(uint32_t index) const
    {
        size_t offset = 0;

        for (auto &g : groups)
        {
            auto elementSize = g.elementType.getValueDataSize();

            if (index < g.repetitions)
                return {g.elementType, offset + elementSize * index};

            index -= g.repetitions;
            offset += elementSize * g.repetitions;
        }

        throwError("Index out of range");
    }

    ElementTypeAndOffset getElementInfo(uint32_t index) const
    {
        size_t offset = 0;

        for (uint32_t i = 0; i < members.size; ++i)
        {
            if (i == index)
                return {members[i].type, offset};

            offset += members[i].type.getValueDataSize();
        }

        throwError("Index out of range");
    }


	inline ElementTypeAndOffset Type::getElementTypeAndOffset (uint32_t index) const
	{
	    if (isType (MainType::vector))          return content.vector.getElementInfo (index);
	    if (isType (MainType::primitiveArray))  return content.primitiveArray.getElementInfo (index);
	    if (isType (MainType::complexArray))    return content.complexArray->getElementInfo (index);
	    if (isType (MainType::object))          return content.object->getElementInfo (index);
	
	    throwError ("Invalid type");
	}
	
	inline ElementTypeAndOffset Type::getElementRangeInfo (uint32_t start, uint32_t length) const
	{
	    if (isType (MainType::vector))          return content.vector.getElementRangeInfo (start, length);
	    if (isType (MainType::primitiveArray))  return content.primitiveArray.getElementRangeInfo (start, length);
	    if (isType (MainType::complexArray))    return content.complexArray->getElementRangeInfo (allocator, start, length);
	
	    throwError ("Invalid type");
	}

}

/// ----------------------------------------------------------------------------

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
 * serialization_macros.h
 * -------------------------------------------------------
 * Created: 11/7/2025
 * -------------------------------------------------------
 */
#pragma once

// ------------------------------------------------------

/*
#define SEDX_SERIALIZE_PROPERTY(propName, propVal, outputNode) outputNode[#propName] = propVal

#define SEDX_SERIALIZE_PROPERTY_ASSET(propName, propVal, outputData) outputData[#propName] = ((propVal) ? static_cast<uint64_t>((propVal)->Handle) : 0)

#define SEDX_DESERIALIZE_PROPERTY(propertyName, destination, node, defaultValue)	\
if ((node).is_object())																\
{																					\
	if ((node).contains(#propertyName))												\
	{																				\
		try																			\
		{																			\
			(destination) = (node)[#propertyName].get<decltype(defaultValue)>();	\
		}																			\
		catch (const std::exception& e)												\
		{																			\
			SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to deserialize {}: {}", #propertyName, e.what()); \
			(destination) = defaultValue;											\
		}																			\
	}																				\
	else																			\
	{																				\
		(destination) = defaultValue;												\
	}																				\
}																					\
else																				\
{																					\
	(destination) = defaultValue;                                                   \
}

#define SEDX_DESERIALIZE_PROPERTY_ASSET(propName, destination, inputData, assetClass)											\
{																																\
    SceneryEditorX::AssetHandle assetHandle = (inputData).contains(#propName) ? (inputData)[#propName].get<uint64_t>() : 0;		\
		if (AssetManager::IsAssetHandleValid(assetHandle))																		\
		{																														\
            (destination) = AssetManager::GetAsset<assetClass>(assetHandle);													\
        }																														\
		else																													\
		{																														\
            SEDX_CORE_ERROR_TAG("AssetManager", "Tried to load invalid asset {0}.", #assetClass);								\
		}																														\
}		
*/


///	------------------------------------------------------

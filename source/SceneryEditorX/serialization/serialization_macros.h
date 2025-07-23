/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* serialization_macros.h
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#pragma once

/// ------------------------------------------------------

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

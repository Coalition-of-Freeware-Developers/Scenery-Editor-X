/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_pipeline_cache.h
* -------------------------------------------------------
* Created: 20/6/2025
* -------------------------------------------------------
*/
#pragma once
#include <vector>
#include <vulkan/vulkan.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
     * @class PipelineCache
	 * @brief Manages a Vulkan pipeline cache
	 *
	 * This class encapsulates the functionality to create, destroy,
	 * and save/loading the pipeline cache data for efficiency in shader compilation.
	 *
	 */
	class PipelineCache
    {
    public:
        PipelineCache();
        ~PipelineCache();

        VkPipelineCache CreateCache();
        void DestroyCache() const;
        void SaveCache();
        [[nodiscard]] std::vector<uint8_t> LoadCache() const;

    private:
        VkPipelineCache pipelineCache = nullptr;
        size_t pipelineCacheDataSize = 0;
        std::vector<uint8_t> pipelineCacheData;

		/// -------------------------------------------

		struct PipelineCachePrefixHeader
        {
            uint32_t cacheHeader;       /// An arbitrary magic header to make sure this is actually our file
            uint32_t dataSize;          /// Equal to *pDataSize returned by vkGetPipelineCacheData
            uint64_t dataHash;          /// A hash of pipeline cache data, including the header
            uint32_t vendorID;          /// Equal to VkPhysicalDeviceProperties::vendorID
            uint32_t deviceID;          /// Equal to VkPhysicalDeviceProperties::deviceID
            uint32_t driverVersion;     /// Equal to VkPhysicalDeviceProperties::driverVersion
            uint32_t driverABI;         /// Equal to sizeof(void*)
            uint8_t uuid[VK_UUID_SIZE]; /// Equal to VkPhysicalDeviceProperties::pipelineCacheUUID
        };
    };
}

/// -------------------------------------------------------

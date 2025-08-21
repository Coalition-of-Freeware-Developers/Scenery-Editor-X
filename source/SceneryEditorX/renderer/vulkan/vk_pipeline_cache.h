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
    class PipelineCache : public RefCounted
    {
    public:
        PipelineCache() = delete;
        virtual ~PipelineCache() override = default;

        void CheckCache(char *cacheData, size_t cacheDataSize);
        VkPipelineCache CreateCache();
        void DestroyCache();
        //void SaveCache();
        void LoadCache();

    private:
        VkPipelineCache pipelineCache = nullptr;

		/// -------------------------------------------

        uint32_t cacheHeader = 0;			/// An arbitrary magic header to make sure this is actually our file
        uint32_t cacheHeaderVersion = 0;    /// Equal to VK_PIPELINE_CACHE_HEADER_VERSION_ONE
        uint32_t vendorID = 0;              /// Equal to VkPhysicalDeviceProperties::vendorID
        uint32_t deviceID = 0;              /// Equal to VkPhysicalDeviceProperties::deviceID
        uint8_t  uuid[VK_UUID_SIZE] = {};   /// Equal to VkPhysicalDeviceProperties::pipelineCacheUUID

        //uint32_t dataSize = 0;				/// Equal to *pDataSize returned by vkGetPipelineCacheData

    };
}

/// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_pipeline_cache.cpp
* -------------------------------------------------------
* Created: 20/6/2025
* -------------------------------------------------------
*/
#include "vk_pipeline_cache.h"
#include <filesystem>
#include <fstream>
#include "SceneryEditorX/logging/asserts.h"
#include "SceneryEditorX/renderer/render_context.h"

/// -------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Check for cache validity																			  ///
///																										  ///
/// TODO: Update this as the spec evolves. The fields are not defined by the header.					  ///
///																										  ///
/// The code below supports SDK 0.10 Vulkan spec, which contains the following table:					  ///
///																										  ///
/// Offset	 Size            Meaning																	  ///
/// ------    ------------    ------------------------------------------------------------------		  ///
///      0               4    a device ID equal to VkPhysicalDeviceProperties::DeviceId written			  ///
///                           as a stream of bytes, with the least significant byte first				  ///
///																										  ///
///      4    VK_UUID_SIZE    a pipeline cache ID equal to VkPhysicalDeviceProperties::pipelineCacheUUID  ///
///																										  ///
///																										  ///
/// The code must be updated for latest Vulkan spec, which contains the following table:				  ///
///																										  ///
/// Offset	 Size            Meaning																	  ///
/// ------    ------------    ------------------------------------------------------------------		  ///
///      0               4    length in bytes of the entire pipeline cache header written as a			  ///
///                           stream of bytes, with the least significant byte first					  ///
///      4               4    a VkPipelineCacheHeaderVersion value written as a stream of bytes,		  ///
///                           with the least significant byte first										  ///
///      8               4    a vendor ID equal to VkPhysicalDeviceProperties::vendorID written			  ///
///                           as a stream of bytes, with the least significant byte first				  ///
///     12               4    a device ID equal to VkPhysicalDeviceProperties::deviceID written			  ///
///                           as a stream of bytes, with the least significant byte first				  ///
///     16    VK_UUID_SIZE    a pipeline cache ID equal to VkPhysicalDeviceProperties::pipelineCacheUUID  ///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace SceneryEditorX
{
    /// Constants for cache directory and file
    INTERNAL std::string PIPELINE_CACHE_DIR = "../cache";
    INTERNAL std::string PIPELINE_CACHE_FILE = "pipeline_cache.bin";

    /// -------------------------------------------------------

    PipelineCache::PipelineCache() = default;
    PipelineCache::~PipelineCache() = default;

    /// -------------------------------------------------------

    void PipelineCache::CheckCache(char *cacheData, size_t cacheDataSize)
    {
        auto device = RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDeviceProperties();

		/// Check if cache data is valid
        memcpy(&cacheHeader, (uint8_t *)cacheData + 0, 4);
        memcpy(&cacheHeaderVersion, (uint8_t *)cacheData + 4, 4);
        memcpy(&vendorID, (uint8_t *)cacheData + 8, 4);
        memcpy(&deviceID, (uint8_t *)cacheData + 12, 4);
        memcpy(uuid, (uint8_t *)cacheData + 16, VK_UUID_SIZE);

        /// Check each field and report bad values before freeing existing cache
        bool badCache = false;

        /// -------------------------------------------------------

        if (cacheHeader <= 0)
        {
            badCache = true;
        }
        if (cacheHeaderVersion != VK_PIPELINE_CACHE_HEADER_VERSION_ONE )
		{
		    badCache = true;
		}
        if (vendorID != device.vendorID)
        {
            badCache = true;
        }
        if (deviceID != device.deviceID)
        {
            badCache = true;
        }
        if (memcmp(uuid, device.pipelineCacheUUID, sizeof(uuid)) != 0)
        {
            badCache = true;
        }
        if (badCache)
        {
            /// Don't submit initial cache data if any version info is incorrect
            free(cacheData);
            cacheDataSize = 0;
            cacheData = nullptr;

            if (remove(PIPELINE_CACHE_FILE.c_str()) != 0)
            {
                SEDX_CORE_FATAL_TAG("Pipeline Cache","Error attempting to delete the found pipeline cache file: {}", PIPELINE_CACHE_FILE);
            }
        }
    }

    /// -------------------------------------------------------

    void PipelineCache::LoadCache()
    {
        size_t cacheDataSize = 0;
        char* cacheData = nullptr;

        const std::string cacheFilePath = PIPELINE_CACHE_DIR + PIPELINE_CACHE_FILE;
        std::ifstream readCacheStream(PIPELINE_CACHE_FILE, std::ios_base::in | std::ios_base::binary);

        /// Check if directory exists, create if not
        if (!std::filesystem::exists(PIPELINE_CACHE_DIR))
		{
            SEDX_CORE_INFO("Creating pipeline cache directory: {}", PIPELINE_CACHE_DIR);
            std::filesystem::create_directories(PIPELINE_CACHE_DIR);
            CreateCache();
        }

        if(std::filesystem::exists(cacheFilePath))
        {
            if (readCacheStream.good())
            {
                /// Determine cache size
                readCacheStream.seekg(0, std::ifstream::end);
                cacheDataSize = static_cast<size_t>(readCacheStream.tellg());
                readCacheStream.seekg(0, std::ifstream::beg);

                SEDX_CORE_TRACE_TAG("Pipeline Cache","Found pipeline cache file, size: {} bytes", cacheDataSize);

                /// Allocate memory to hold the initial cache data
                cacheData = (char *)std::malloc(cacheDataSize);

                /// Read the data into our buffer
                readCacheStream.read(cacheData, cacheDataSize);

                /// Check the cache data for validity
                CheckCache(cacheData, cacheDataSize);

                /// Clean up and print results
                readCacheStream.close();

                SEDX_CORE_TRACE_TAG("Pipeline Cache", "Pipeline Cache loaded from ", PIPELINE_CACHE_FILE);
            }
            else
			{
                SEDX_CORE_WARN("Failed to load pipeline cache from file: {} - {}", cacheFilePath);
            }
        }
        else
        {
            SEDX_CORE_WARN("Unable to find a pipeline cache in, {}", cacheFilePath);
        }
    }

    VkPipelineCache PipelineCache::CreateCache()
    {
        auto device = RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDeviceProperties();
        size_t cacheDataSize = 0;
        char *initCacheData = nullptr;

        cacheHeader = 0;
        cacheHeaderVersion = 0;
        vendorID = 0;
        deviceID = 0;
        uuid[VK_UUID_SIZE] = {};
        pipelineCache = VK_NULL_HANDLE;

        VkPipelineCacheCreateInfo cacheCreateInfo;
        cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        cacheCreateInfo.pNext = nullptr;
        cacheCreateInfo.flags = VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT | VK_PIPELINE_CACHE_CREATE_INTERNALLY_SYNCHRONIZED_MERGE_BIT_KHR;
        cacheCreateInfo.initialDataSize = cacheDataSize;
        cacheCreateInfo.pInitialData = initCacheData;

        /// Free our initialData now that pipeline cache has been created
        free(initCacheData);
        initCacheData = nullptr;

        ///////////////////////////////////////////////////////////
		/// Create Pipeline Data								///
        ///////////////////////////////////////////////////////////


		

    }

    /// -------------------------------------------------------

    void PipelineCache::DestroyCache()
    {
        if (pipelineCache != VK_NULL_HANDLE && RenderContext::GetCurrentDevice() && RenderContext::GetCurrentDevice()->GetDevice() != VK_NULL_HANDLE)
		{
            auto device = RenderContext::GetCurrentDevice()->GetDevice();
            vkDestroyPipelineCache(device, pipelineCache, RenderContext::Get()->allocatorCallback);
            SEDX_CORE_INFO("Pipeline cache destroyed");
        }
    }

    /// -------------------------------------------------------

    /*
    void PipelineCache::SaveCache()
    {

        ///////////////////////////////////////////////////////////

        auto vkdevice = RenderContext::GetCurrentDevice()->GetDevice();
        VkResult result =
            vkCreatePipelineCache(vkdevice, &cacheCreateInfo, RenderContext::Get()->allocatorCallback, &pipelineCache);
        if (result != VK_SUCCESS)
            SEDX_CORE_ERROR("Failed to create pipeline cache! Error code: {}", static_cast<int>(result));

        /**
         * Store away the cache that we've populated.  This could conceivably happen
         * earlier, depends on when the pipeline cache stops being populated
         * internally.
         #1#
        std::vector<uint8_t> endCacheData = pipelineCache;

        /// Write the file to disk, overwriting whatever was there
        if (std::ofstream writeCacheStream(PIPELINE_CACHE_FILE, std::ios_base::out | std::ios_base::binary);
            writeCacheStream.good())
        {
            writeCacheStream.write(reinterpret_cast<char const *>(endCacheData.data()), endCacheData.size());
            writeCacheStream.close();
            SEDX_CORE_TRACE_TAG("Pipeline Cache", "cacheData written to {}", PIPELINE_CACHE_FILE);
        }
        else
        {
            SEDX_CORE_ERROR("Unable to write cache data to disk!");
        }

        SEDX_ASSERT(result == VK_SUCCESS && "Failed to create pipeline cache!");
        return pipelineCache;
    }
    */

}

/// -------------------------------------------------------

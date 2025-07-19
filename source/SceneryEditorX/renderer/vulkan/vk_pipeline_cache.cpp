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
#include <filesystem>
#include <fstream>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/vulkan/vk_pipeline_cache.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    // Constants for cache directory and file
    INTERNAL const std::string PIPELINE_CACHE_DIR = "../cache/";
    INTERNAL const std::string PIPELINE_CACHE_FILE = "pipeline.cache";

    /// -------------------------------------------------------

    PipelineCache::PipelineCache() = default;

    PipelineCache::~PipelineCache()
    {
        DestroyCache();
    }

    /// -------------------------------------------------------

    std::vector<uint8_t> PipelineCache::LoadCache() const
    {
        std::vector<uint8_t> cacheData;
        const std::string cacheFilePath = PIPELINE_CACHE_DIR + PIPELINE_CACHE_FILE;

        /// Check if directory exists, create if not
        if (!std::filesystem::exists(PIPELINE_CACHE_DIR))
		{
            SEDX_CORE_INFO("Creating pipeline cache directory: {}", PIPELINE_CACHE_DIR);
            std::filesystem::create_directories(PIPELINE_CACHE_DIR);
        }

        if (std::filesystem::exists(cacheFilePath))
		{
            try
			{
                if (std::ifstream file(cacheFilePath, std::ios::binary | std::ios::ate); file.is_open())
				{
                    size_t fileSize = static_cast<size_t>(file.tellg());
                    SEDX_CORE_INFO("Found pipeline cache file, size: {} bytes", fileSize);

                    if (fileSize > 0)
					{
                        cacheData.resize(fileSize);
                        file.seekg(0);
                        file.read(reinterpret_cast<char*>(cacheData.data()), fileSize);
                        file.close();

                        SEDX_CORE_INFO("Successfully loaded pipeline cache from file: {}", cacheFilePath);
                        return cacheData;
                    }
                }
            }
            catch (const std::exception& e)
			{
                SEDX_CORE_WARN("Failed to load pipeline cache from file: {} - {}", cacheFilePath, e.what());
            }
        }

        /// If unable to load from file or already initialized, retrieve from the current cache
        if (pipelineCache != nullptr && RenderContext::GetCurrentDevice() != VK_NULL_HANDLE)
        {
            /// Get the size of the pipeline cache data
            const auto device = RenderContext::GetCurrentDevice()->GetDevice();
            size_t dataSize = 0;
            if (VkResult result = vkGetPipelineCacheData(device, pipelineCache, &dataSize, nullptr); result == VK_SUCCESS && dataSize > 0)
            {
                /// Resize our vector to hold the data
                cacheData.resize(dataSize);

                /// Get the actual data
                result = vkGetPipelineCacheData(device, pipelineCache, &dataSize, cacheData.data());
                if (result != VK_SUCCESS)
                    cacheData.clear();
                else
                    SEDX_CORE_INFO("Retrieved pipeline cache data from device: {} bytes", dataSize);
            }
        }

        return cacheData;
    }

    VkPipelineCache PipelineCache::CreateCache()
    {
        const auto device = RenderContext::GetCurrentDevice()->GetDevice();

        /// Try to load cached data from disk
        if (pipelineCacheData.empty())
		{
            pipelineCacheData = LoadCache();
            if (!pipelineCacheData.empty())
                SEDX_CORE_INFO("Loaded {} bytes of pipeline cache data for initialization", pipelineCacheData.size());
            else
                SEDX_CORE_INFO("No pipeline cache data found, creating a new empty cache");
        }

        VkPipelineCacheCreateInfo cacheCreateInfo = {};
        cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        cacheCreateInfo.pNext = nullptr;
        cacheCreateInfo.flags = 0;
        if (!pipelineCacheData.empty())
        {
            cacheCreateInfo.initialDataSize = pipelineCacheData.size();
            cacheCreateInfo.pInitialData = pipelineCacheData.data();
        }
        else
        {
            cacheCreateInfo.initialDataSize = 0;
            cacheCreateInfo.pInitialData = nullptr;
        }

        pipelineCache = VK_NULL_HANDLE;
        const VkResult result = vkCreatePipelineCache(device, &cacheCreateInfo, RenderContext::Get()->allocatorCallback, &pipelineCache);
        if (result != VK_SUCCESS)
            SEDX_CORE_ERROR("Failed to create pipeline cache! Error code: {}", static_cast<int>(result));
        else
            SEDX_CORE_INFO("Pipeline cache created successfully");

        SEDX_ASSERT(result == VK_SUCCESS && "Failed to create pipeline cache!");
        return pipelineCache;
    }

    void PipelineCache::DestroyCache() const
    {
        if (pipelineCache != VK_NULL_HANDLE && RenderContext::GetCurrentDevice() && RenderContext::GetCurrentDevice()->GetDevice() != VK_NULL_HANDLE)
		{
            const auto device = RenderContext::GetCurrentDevice()->GetDevice();
            vkDestroyPipelineCache(device, pipelineCache, RenderContext::Get()->allocatorCallback);
            SEDX_CORE_INFO("Pipeline cache destroyed");
        }
    }

    void PipelineCache::SaveCache()
    {
        if (pipelineCache == VK_NULL_HANDLE || !RenderContext::GetCurrentDevice() || RenderContext::GetCurrentDevice()->GetDevice() == VK_NULL_HANDLE)
		{
            SEDX_CORE_WARN("Cannot save pipeline cache: Invalid device or cache handle");
            return;
        }

        const auto device = RenderContext::GetCurrentDevice()->GetDevice();
        /// Get the size of the pipeline cache data
        VkResult result = vkGetPipelineCacheData(device, pipelineCache, &pipelineCacheDataSize, nullptr);
        if (result != VK_SUCCESS)
		{
            SEDX_CORE_ERROR("Failed to get pipeline cache size! Error code: {}", static_cast<int>(result));
            return;
        }
        SEDX_ASSERT(result == VK_SUCCESS && "Failed to get pipeline cache size!");

        pipelineCacheData.resize(pipelineCacheDataSize);
        result = vkGetPipelineCacheData(device, pipelineCache, &pipelineCacheDataSize, pipelineCacheData.data());
        if (result != VK_SUCCESS)
		{
            SEDX_CORE_ERROR("Failed to get pipeline cache data! Error code: {}", static_cast<int>(result));
            return;
        }

        SEDX_ASSERT(result == VK_SUCCESS && "Failed to get pipeline cache data!");

        if (!std::filesystem::exists(PIPELINE_CACHE_DIR))
		{
            SEDX_CORE_INFO("Creating pipeline cache directory: {}", PIPELINE_CACHE_DIR);
            std::filesystem::create_directories(PIPELINE_CACHE_DIR);
        }

        /// Write the cache data to disk
        try
		{
            const std::string cacheFilePath = PIPELINE_CACHE_DIR + PIPELINE_CACHE_FILE;
            if (std::ofstream file(cacheFilePath, std::ios::binary); file.is_open())
			{
                file.write(reinterpret_cast<const char*>(pipelineCacheData.data()), pipelineCacheData.size());
                file.close();

                SEDX_CORE_INFO("Pipeline cache saved to file: {} ({} bytes)", cacheFilePath, pipelineCacheData.size());
            }
            else
                SEDX_CORE_WARN("Failed to open file for writing pipeline cache: {}", cacheFilePath);
        }
        catch (const std::exception& e)
		{
            SEDX_CORE_ERROR("Exception while saving pipeline cache: {}", e.what());
        }
    }
} // namespace SceneryEditorX

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_queue.h
* -------------------------------------------------------
* Created: 19/3/2025
* -------------------------------------------------------
*/
#pragma once

#include <SceneryEditorX/renderer/vk_buffers.h>
#include <SceneryEditorX/renderer/vk_util.h>

// -------------------------------------------------------

enum Queue
{
    Graphics = 0,
    Compute = 1,
    Transfer = 2,
    Count = 3,
};

struct CommandResources
{
    uint8_t *stagingCpu = nullptr;
    uint32_t stagingOffset = 0;
    Buffer staging;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBuffer buffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkQueryPool queryPool;
    std::vector<std::string> timeStampNames;
    std::vector<uint64_t> timeStamps;
};

struct InternalQueue
{
    VkQueue queue = VK_NULL_HANDLE;
    int family = -1;
    std::vector<CommandResources> commands;
};

InternalQueue queues[Queue::Count];
Queue currentQueue = Queue::Count;

// -------------------------------------------------------


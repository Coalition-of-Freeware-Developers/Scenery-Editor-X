/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* graphics_context.cpp
* -------------------------------------------------------
* Created: 20/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/renderer/graphics_context.h>
#include <SceneryEditorX/renderer/vk_buffers.h>
#include <SceneryEditorX/renderer/vk_image.h>

// -------------------------------------------------------

struct Context
{
    void CmdCopy(Buffer &dst, void *data, uint32_t size, uint32_t dstOfsset);
    void CmdCopy(Buffer &dst, Buffer &src, uint32_t size, uint32_t dstOffset, uint32_t srcOffset);
    void CmdCopy(Image &dst, void *data, uint32_t size);
    void CmdCopy(Image &dst, Buffer &src, uint32_t size, uint32_t srcOffset);
    void CmdBarrier(Image &img, Layout::ImageLayout layout);
    void CmdBarrier();
    void EndCommandBuffer(VkSubmitInfo submitInfo);

    void LoadShaders(Pipeline &pipeline);
    std::vector<char> CompileShader(const std::filesystem::path &path);
    void CreatePipeline(const PipelineDesc &desc, Pipeline &pipeline);

    // -------------------------------------------------------

    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;

    // -------------------------------------------------------
};


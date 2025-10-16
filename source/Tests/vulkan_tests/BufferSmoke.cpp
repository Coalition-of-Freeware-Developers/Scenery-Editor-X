// Bring in common STL headers that AppCore gets via PCH so this TU can compile standalone
#include <array>
#include <cstring>
#include <cstdlib>

#include <catch2/catch_test_macros.hpp>

#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/logging/asserts.h>

#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/vulkan/vk_buffers.h>

using namespace SceneryEditorX;

TEST_CASE("Buffer smoke: staging upload then read back via CPU mapping (sanity)", "[vulkan][buffer][smoke]")
{
    Log::Init();

    const char* runFlag = std::getenv("SEDX_RUN_VULKAN_SMOKE_TESTS");
    if (!runFlag || std::string_view(runFlag) != "1")
    {
        WARN("Set SEDX_RUN_VULKAN_SMOKE_TESTS=1 to execute Vulkan smoke tests");
        SUCCEED("Skipped");
        return;
    }
#if defined(_WIN32)
    _putenv_s("SEDX_HEADLESS", "1");
#else
    setenv("SEDX_HEADLESS", "1", 1);
#endif

    auto ctx = RenderContext::Get();
    REQUIRE(ctx != nullptr);
    ctx->Init();
    if (!ctx->IsInitialized())
    {
        WARN("RenderContext failed to initialize — skipping buffer smoke test");
        SUCCEED("Skipped");
        return;
    }

    // Prepare a small payload
    std::array<uint8_t, 256> payload{};
    for (size_t i = 0; i < payload.size(); ++i) payload[i] = static_cast<uint8_t>(i);

    // Create a CPU-visible staging buffer and copy payload into it
    Buffer staging = CreateBuffer(payload.size(), BufferUsage::TransferSrc, MemoryType::CPU, "StagingSmoke");
    REQUIRE(staging.resource != nullptr);

    void* mapped = MapBuffer(staging);
    REQUIRE(mapped != nullptr);
    std::memcpy(mapped, payload.data(), payload.size());
    UnmapBuffer(staging);

    // Create a GPU buffer (device-local) as destination
    Buffer deviceBuf = CreateBuffer(payload.size(), BufferUsage::TransferDst | BufferUsage::TransferSrc | BufferUsage::Storage, MemoryType::GPU, "GPUCopyDst");
    REQUIRE(deviceBuf.resource != nullptr);

    // Copy contents from staging -> device
    CopyBuffer(staging.resource->buffer, deviceBuf.resource->buffer, static_cast<VkDeviceSize>(payload.size()));

    // Create another CPU-visible buffer and copy back from device to verify round-trip
    Buffer readback = CreateBuffer(payload.size(), BufferUsage::TransferDst, MemoryType::CPU, "ReadbackSmoke");
    REQUIRE(readback.resource != nullptr);

    // Use region copy (device -> readback)
    CopyBuffer(deviceBuf.resource->buffer, readback.resource->buffer, static_cast<VkDeviceSize>(payload.size()));

    // Inspect readback
    void* mappedReadback = MapBuffer(readback);
    REQUIRE(mappedReadback != nullptr);

    bool equal = std::memcmp(mappedReadback, payload.data(), payload.size()) == 0;
    UnmapBuffer(readback);

    REQUIRE(equal);
}

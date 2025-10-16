// Bring in common STL headers that AppCore gets via PCH so this TU can compile standalone
#include <algorithm>
#include <unordered_set>
#include <cmath>

#include <catch2/catch_test_macros.hpp>

// Ensure logging/assert macros are available (used transitively by some renderer headers)
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/logging/asserts.h>

#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/command_manager.h>
#include <SceneryEditorX/renderer/vulkan/vk_device.h>

using namespace SceneryEditorX;

TEST_CASE("CommandPool smoke: allocate, begin, flush", "[vulkan][commandpool][smoke]")
{
    // Initialize logging first (RenderContext uses SEDX_CORE_* macros)
    Log::Init();

    // Opt-in guard to avoid crashes on systems without a Vulkan runtime
    const char* runFlag = std::getenv("SEDX_RUN_VULKAN_SMOKE_TESTS");
    if (!runFlag || std::string_view(runFlag) != "1")
    {
        WARN("Set SEDX_RUN_VULKAN_SMOKE_TESTS=1 to execute Vulkan smoke tests");
        SUCCEED("Skipped");
        return;
    }

    // Prefer headless initialization to avoid WSI dependency in CI/servers
#if defined(_WIN32)
    _putenv_s("SEDX_HEADLESS", "1");
#else
    setenv("SEDX_HEADLESS", "1", 1);
#endif

    // Initialize RenderContext once
    auto ctx = RenderContext::Get();
    REQUIRE(ctx != nullptr);
    ctx->Init();

    if (!ctx->IsInitialized())
    {
        WARN("RenderContext failed to initialize — skipping Vulkan smoke test (likely missing Vulkan runtime or validation layers)");
        SUCCEED("Skipped");
        return;
    }

    auto device = RenderContext::GetCurrentDevice();
    REQUIRE(device != nullptr);

    // Create a command pool bound to the graphics queue family
    Ref<CommandPool> pool = CreateRef<CommandPool>(device, Queue::Graphics);
    REQUIRE(pool != nullptr);

    // Allocate a primary command buffer and begin recording immediately
    VkCommandBuffer cmd = pool->AllocateCommandBuffer(true /*begin*/, false /*compute*/);
    REQUIRE(cmd != VK_NULL_HANDLE);

    // No commands necessary for smoke; flush will end, submit, wait, and free
    REQUIRE_NOTHROW(pool->FlushCmdBuffer(cmd));
}

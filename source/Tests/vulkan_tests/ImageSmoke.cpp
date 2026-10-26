// Bring in common STL headers that AppCore gets via PCH so this TU can compile standalone
#include <array>
#include <cstring>
#include <cstdlib>

#include <catch2/catch_test_macros.hpp>

#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/logging/asserts.h>

#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/image_data.h>
#include <SceneryEditorX/renderer/vulkan/vk_image.h>
#include <SceneryEditorX/renderer/vulkan/vk_buffers.h>

using namespace SceneryEditorX;

TEST_CASE("Image smoke: create, upload via staging, transition and copy back", "[vulkan][image][smoke]")
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
        WARN("RenderContext failed to initialize — skipping image smoke test");
        SUCCEED("Skipped");
        return;
    }

    // Prepare a tiny 4x4 RGBA pattern
    const uint32_t width = 4, height = 4;
    std::array<uint8_t, 4 * 4 * 4> pixels{};
    for (uint32_t y = 0; y < height; ++y)
    {
        for (uint32_t x = 0; x < width; ++x)
        {
            const size_t idx = (y * width + x) * 4;
            pixels[idx + 0] = static_cast<uint8_t>(x * 16);
            pixels[idx + 1] = static_cast<uint8_t>(y * 16);
            pixels[idx + 2] = 0x80;
            pixels[idx + 3] = 0xFF;
        }
    }

    Buffer cpuSrc = Buffer::Copy(pixels.data(), pixels.size());

    ImageSpecification spec;
    spec.debugName = "ImageSmoke";
    spec.format = VK_FORMAT_R8G8B8A8_UNORM;
    spec.usage = ImageUsage::TransferDst; // we will upload then sample-readback path uses CopyToHostBuffer
    spec.transfer = true;                 // allow transfer ops
    spec.width = width;
    spec.height = height;
    spec.mips = 1;
    spec.layers = 1;
    spec.createSampler = false;          // sampler not needed for this smoke

    Ref<Image2D> image = CreateRef<Image2D>(spec);
    REQUIRE(image);

    // Upload
    REQUIRE_NOTHROW(image->SetData(cpuSrc));

    // Read back
    Buffer readback;
    REQUIRE_NOTHROW(image->CopyToHostBuffer(readback));
    REQUIRE(readback.size == pixels.size());

    bool same = std::memcmp(readback.data, pixels.data(), pixels.size()) == 0;
    REQUIRE(same);
}

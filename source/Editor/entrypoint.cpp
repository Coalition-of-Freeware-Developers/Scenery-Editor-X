/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* entrypoint.cpp
* -------------------------------------------------------
* Main entry point for the Editor executable.
* Uses the APP_MAIN macro from entrypoint.h which creates
* the platform-appropriate entry point (WinMain on Windows,
* main on Linux/macOS) and wires up the PlatformContext.
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/platform/entrypoint.h>
#include <SceneryEditorX/renderer/vulkan/vulkan_support.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

/// -------------------------------------------------------

/* @brief Tells the user why the editor can't start, before any window exists. */
static void ShowStartupError(const std::string &message)
{
#ifdef SEDX_PLATFORM_WINDOWS
    const int length = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, nullptr, 0);
    std::wstring text(length > 0 ? length : 0, L'\0');
    if (length > 0)
        MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, text.data(), length);
    MessageBoxW(nullptr, text.c_str(), L"Scenery Editor X", MB_OK | MB_ICONERROR);
#else
    std::fprintf(stderr, "%s\n", message.c_str());
#endif
}

/// -------------------------------------------------------

/**
 * @brief Platform entry point defined via the APP_MAIN macro.
 *
 * The APP_MAIN macro expands to the correct platform entry point
 * (WinMain on Windows, main on Linux/macOS), creates the
 * PlatformContext, and invokes SceneryEditorX::Main(context).
 *
 * The body below is the platform_initializer function which is
 * called before SceneryEditorX::Main for any pre-init work.
 */
APP_MAIN(context)
{
    using namespace SceneryEditorX;

    // The installer runs "SceneryEditorX.exe --check-vulkan" and reads the exit code (VulkanSupport): no window, no log
    const std::vector<std::string> &args = context.GetCommandLineArgs();
    const VulkanSupportReport vulkan = CheckVulkanSupport();
    if (std::ranges::find(args, "--check-vulkan") != args.end())
        std::exit(static_cast<int>(vulkan.status));

    // Without Vulkan 1.3 the renderer can't start: say what to install instead of crashing
    if (vulkan.status != VulkanSupport::Supported)
    {
        ShowStartupError(DescribeVulkanSupport(vulkan));
        std::exit(static_cast<int>(vulkan.status));
    }
    return 0;
}

/// -------------------------------------------------------

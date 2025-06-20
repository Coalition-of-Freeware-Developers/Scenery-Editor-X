#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <stdio.h>
#include <stdlib.h> 


#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Volk headers
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif

//#include "../edX/edXProjectFile.h"
#include "FileBrowser.hpp"

// Data
static VkAllocationCallbacks *g_Allocator = nullptr;
static VkInstance g_Instance = VK_NULL_HANDLE;
static VkPhysicalDevice g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice g_Device = VK_NULL_HANDLE;
static uint32_t g_QueueFamily = (uint32_t)-1;
static VkQueue g_Queue = VK_NULL_HANDLE;
static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
static VkPipelineCache g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static int g_MinImageCount = 2;
static bool g_SwapChainRebuild = false;

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static void check_vk_result(VkResult error)
{
    if (error == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", error);
    if (error < 0)
        abort();
}

#ifdef APP_USE_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags,
                                                   VkDebugReportObjectTypeEXT objectType,
                                                   uint64_t object,
                                                   size_t location,
                                                   int32_t messageCode,
                                                   const char *pLayerPrefix,
                                                   const char *pMessage,
                                                   void *pUserData)
{
    (void)flags;
    (void)object;
    (void)location;
    (void)messageCode;
    (void)pUserData;
    (void)pLayerPrefix; // Unused arguments
    fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
    return VK_FALSE;
}
#endif

static bool IsExtensionAvailable(const ImVector<VkExtensionProperties> &properties, const char *extension)
{
    for (const VkExtensionProperties &p : properties)
        if (strcmp(p.extensionName, extension) == 0)
            return true;
    return false;
}

static VkPhysicalDevice SetupVulkan_SelectPhysicalDevice()
{
    uint32_t gpu_count;
    VkResult error = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, nullptr);
    check_vk_result(error);
    IM_ASSERT(gpu_count > 0);

    ImVector<VkPhysicalDevice> gpus;
    gpus.resize(gpu_count);
    error = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, gpus.Data);
    check_vk_result(error);

    // If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available. This covers
    // most common cases (multi-gpu/integrated+dedicated graphics). Handling more complicated setups (multiple
    // dedicated GPUs) is out of scope of this sample.
    for (VkPhysicalDevice &device : gpus)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            return device;
    }

    // Use first GPU (Integrated) is a Discrete one is not available.
    if (gpu_count > 0)
        return gpus[0];
    return VK_NULL_HANDLE;
}

static void SetupVulkan(ImVector<const char *> instance_extensions)
{
    VkResult error;
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
    volkInitialize();
#endif

    // Create Vulkan Instance
    {
        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        // Enumerate available extensions
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
        properties.resize(properties_count);
        error = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.Data);
        check_vk_result(error);

        // Enable required extensions
        if (IsExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
            instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
        {
            instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
#endif

        // Enabling validation layers
#ifdef APP_USE_VULKAN_DEBUG_REPORT
        const char *layers[] = {"VK_LAYER_KHRONOS_validation"};
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;
        instance_extensions.push_back("VK_EXT_debug_report");
#endif

        // Create Vulkan Instance
        create_info.enabledExtensionCount = (uint32_t)instance_extensions.Size;
        create_info.ppEnabledExtensionNames = instance_extensions.Data;
        error = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
        check_vk_result(error);
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
        volkLoadInstance(g_Instance);
#endif

        // Setup the debug report callback
#ifdef APP_USE_VULKAN_DEBUG_REPORT
        auto f_vkCreateDebugReportCallbackEXT =
            (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkCreateDebugReportCallbackEXT");
        IM_ASSERT(f_vkCreateDebugReportCallbackEXT != nullptr);
        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
        debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debug_report_ci.pfnCallback = debug_report;
        debug_report_ci.pUserData = nullptr;
        error = f_vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport);
        check_vk_result(error);
#endif
    }

    // Select Physical Device (GPU)
    g_PhysicalDevice = SetupVulkan_SelectPhysicalDevice();

    // Select graphics queue family
    {
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, nullptr);
        VkQueueFamilyProperties *queues = (VkQueueFamilyProperties *)malloc(sizeof(VkQueueFamilyProperties) * count);
        vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, queues);
        for (uint32_t i = 0; i < count; i++)
            if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                g_QueueFamily = i;
                break;
            }
        free(queues);
        IM_ASSERT(g_QueueFamily != (uint32_t)-1);
    }

    // Create Logical Device (with 1 queue)
    {
        ImVector<const char *> device_extensions;
            device_extensions.push_back("VK_KHR_swapchain");

        // Enumerate physical device extension
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, nullptr);
            properties.resize(properties_count);
        vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, properties.Data);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
        if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
            device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

        const float queue_priority[] = {1.0f};
        VkDeviceQueueCreateInfo queue_info[1] = {};
            queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info[0].queueFamilyIndex = g_QueueFamily;
            queue_info[0].queueCount = 1;
            queue_info[0].pQueuePriorities = queue_priority;

        VkDeviceCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
            create_info.pQueueCreateInfos = queue_info;
            create_info.enabledExtensionCount = (uint32_t)device_extensions.Size;
            create_info.ppEnabledExtensionNames = device_extensions.Data;
        error = vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device);

        check_vk_result(error);
        vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
    }

    // Create Descriptor Pool
    // The example only requires a single combined image sampler descriptor for the font image and only uses one descriptor set (for that)
    // If you wish to load e.g. additional textures you may need to alter pools sizes.
    {
        VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
        };
        VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 1;
            pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
            pool_info.pPoolSizes = pool_sizes;

        error = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
        check_vk_result(error);
    }
}

static void SetupVulkanWindow(ImGui_ImplVulkanH_Window *wd, VkSurfaceKHR surface, int width, int height)
{
    wd->Surface = surface;

    // Check for WSI support
    VkBool32 result;
    vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, wd->Surface, &result);
    if (result != VK_TRUE)
    {
        fprintf(stderr, "Error no WSI support on physical device 0\n");
        exit(-1);
    }

    // Select Surface Format
    const VkFormat requestSurfaceImageFormat[] = {VK_FORMAT_B8G8R8A8_UNORM,
                                                  VK_FORMAT_R8G8B8A8_UNORM,
                                                  VK_FORMAT_B8G8R8_UNORM,
                                                  VK_FORMAT_R8G8B8_UNORM};
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_PhysicalDevice,
                                                              wd->Surface,
                                                              requestSurfaceImageFormat,
                                                              (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat),
                                                              requestSurfaceColorSpace);

    // Select Present Mode
#ifdef APP_USE_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_MAILBOX_KHR,
                                        VK_PRESENT_MODE_IMMEDIATE_KHR,
                                        VK_PRESENT_MODE_FIFO_KHR};
#else
    VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
#endif
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice,
                                                          wd->Surface,
                                                          &present_modes[0],
                                                          IM_ARRAYSIZE(present_modes));
    //printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

    // Create SwapChain, RenderPass, Framebuffer, etc.
    IM_ASSERT(g_MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance,
                                           g_PhysicalDevice,
                                           g_Device,
                                           wd,
                                           g_QueueFamily,
                                           g_Allocator,
                                           width,
                                           height,
                                           g_MinImageCount);
}

static void CleanupVulkan()
{
    vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

#ifdef APP_USE_VULKAN_DEBUG_REPORT
    // Remove the debug report callback
    auto f_vkDestroyDebugReportCallbackEXT =
        (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT");
    f_vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif // APP_USE_VULKAN_DEBUG_REPORT

    vkDestroyDevice(g_Device, g_Allocator);
    vkDestroyInstance(g_Instance, g_Allocator);
}

static void CleanupVulkanWindow()
{
    ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData, g_Allocator);
}

static void FrameRender(ImGui_ImplVulkanH_Window *wd, ImDrawData *draw_data)
{
    VkResult error;

    VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    error = vkAcquireNextImageKHR(g_Device,
                                wd->Swapchain,
                                UINT64_MAX,
                                image_acquired_semaphore,
                                VK_NULL_HANDLE,
                                &wd->FrameIndex);
    if (error == VK_ERROR_OUT_OF_DATE_KHR || error == VK_SUBOPTIMAL_KHR)
    {
        g_SwapChainRebuild = true;
        return;
    }
    check_vk_result(error);

    ImGui_ImplVulkanH_Frame *fd = &wd->Frames[wd->FrameIndex];
    {
        error = vkWaitForFences(g_Device,
                              1,
                              &fd->Fence,
                              VK_TRUE,
                              UINT64_MAX); // wait indefinitely instead of periodically checking
        check_vk_result(error);

        error = vkResetFences(g_Device, 1, &fd->Fence);
        check_vk_result(error);
    }
    {
        error = vkResetCommandPool(g_Device, fd->CommandPool, 0);
        check_vk_result(error);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        error = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        check_vk_result(error);
    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        error = vkEndCommandBuffer(fd->CommandBuffer);
        check_vk_result(error);
        error = vkQueueSubmit(g_Queue, 1, &info, fd->Fence);
        check_vk_result(error);
    }
}

static void FramePresent(ImGui_ImplVulkanH_Window *wd)
{
    if (g_SwapChainRebuild)
        return;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult error = vkQueuePresentKHR(g_Queue, &info);
    if (error == VK_ERROR_OUT_OF_DATE_KHR || error == VK_SUBOPTIMAL_KHR)
    {
        g_SwapChainRebuild = true;
        return;
    }
    check_vk_result(error);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount; // Now we can use the next set of semaphores
}

void SaveProject(const std::string &directory, const ProjectFile::projectFile &project);
void LoadProject(const std::string &filePath, ProjectFile::projectFile &project);

int main(int, char **)
{
    // Initialize GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Vulkan support
    GLFWwindow* window = glfwCreateWindow(800, 600, "Scenery Editor X | File Format Tester", nullptr, nullptr);
    if (!glfwVulkanSupported())
    {
        printf("GLFW: Vulkan Not Supported\n");
        return 1;
    }

    ImVector<const char *> extensions;
    uint32_t extensions_count = 0;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
    for (uint32_t i = 0; i < extensions_count; i++)
        extensions.push_back(glfw_extensions[i]);
    SetupVulkan(extensions);

    // Create Window Surface
    VkSurfaceKHR surface;
    VkResult error = glfwCreateWindowSurface(g_Instance, window, g_Allocator, &surface);
    check_vk_result(error);

    // Create Framebuffers
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    ImGui_ImplVulkanH_Window *wd = &g_MainWindowData;
    SetupVulkanWindow(wd, surface, w, h);

    // Initialize ImGui for Vulkan
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle &style = ImGui::GetStyle();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = g_Instance;
    init_info.PhysicalDevice = g_PhysicalDevice;
    init_info.Device = g_Device;
    init_info.QueueFamily = g_QueueFamily;
    init_info.Queue = g_Queue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.RenderPass = wd->RenderPass;
    init_info.Subpass = 0;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = wd->ImageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = g_Allocator;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info);

    // Upload fonts (Vulkan requires this step)
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE; // Create and allocate a command buffer
    ImGui_ImplVulkan_CreateFontsTexture();

    // Main loop
    ProjectFile::projectFile project = {"example", "New Scenery", "1.0", "12.00"};
    std::string directory = std::filesystem::current_path().string();
    char fileNameBuffer[256] = "example.edX";

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Main window
        ImGui::Begin("edX File Format Tester");

        ImGui::InputText("Scenery Name", project.sceneryName.data(), 256);
        ImGui::InputText("Editor Version", project.editorVersion.data(), 256);
        ImGui::InputText("XP Version", project.XPVersion.data(), 256);
        ImGui::InputText("File Name", fileNameBuffer, 256);

        // Todo - Add the returned directory to the project source from ShowDirectoryBrowser
        ImGui::Text("Current Directory: %s", directory.c_str());
        if (ImGui::Button("Select Directory"))
        {
            ShowDirectoryBrowser();
            /*
            const char *selectedDirectory = ShowDirectoryBrowser();
            if (selectedDirectory != nullptr)
            {
                directory = selectedDirectory;
            }
            */
        }

        if (ImGui::Button("Save File"))
        {
            project.filename = fileNameBuffer;
            SaveProject(directory, project);
        }

        if (ImGui::Button("Load File"))
        {
            ShowFileBrowser();
            LoadProject(directory + "/" + fileNameBuffer, project);
        }

        ImGui::End();

        // Rendering
        ImGui::Render();
        ImDrawData *main_draw_data = ImGui::GetDrawData();
        const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
        if (!main_is_minimized)
            FrameRender(wd, main_draw_data);

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        // Present Main Platform Window
        if (!main_is_minimized)
            FramePresent(wd);
    }

    // Cleanup
    error = vkDeviceWaitIdle(g_Device);
    check_vk_result(error);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    CleanupVulkanWindow();
    CleanupVulkan();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

// Vulkan setup functions (simplified)
void CheckVkResult(VkResult error)
{
    if (error != VK_SUCCESS)
    {
        std::cerr << "Vulkan error: " << error << std::endl;
        abort();
    }
}

void SetupVulkan(VkInstance &instance,
                 VkDevice &device,
                 VkPhysicalDevice &physicalDevice,
                 VkQueue &queue,
                 VkCommandPool &commandPool,
                 VkDescriptorPool &descriptorPool,
                 VkRenderPass &renderPass)
{
    // Instance creation
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    CheckVkResult(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

    // Enumerate physical devices
    uint32_t deviceCount = 0;
    CheckVkResult(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
    if (deviceCount == 0)
    {
        std::cerr << "Failed to find GPUs with Vulkan support!" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    CheckVkResult(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

    // Select the first physical device (for simplicity)
    physicalDevice = devices[0];

    // Device creation and queue setup
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = 0; // Set appropriate queue family index
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    CheckVkResult(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));

    // Queue retrieval
    vkGetDeviceQueue(device, 0, 0, &queue);

    // Command pool
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CheckVkResult(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool));

    // Descriptor pool
    VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.maxSets = 1000;
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
    descriptorPoolCreateInfo.pPoolSizes = poolSizes;
    CheckVkResult(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));

    // Render pass
    VkAttachmentDescription attachment = {};
    attachment.format = VK_FORMAT_B8G8R8A8_UNORM; // Set appropriate format
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachment = {};
    colorAttachment.attachment = 0;
    colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachment;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &attachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    CheckVkResult(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}


// Function to strip the file extension if it exists
std::string StripExtension(const std::string &filename)
{
    size_t last_dot = filename.find_last_of(".");
    if (last_dot == std::string::npos)
        return filename;
    return filename.substr(0, last_dot);
}

// File save/load implementations
void SaveProject(const std::string &directory, const ProjectFile::projectFile &project)
{
    std::string filename = StripExtension(project.filename) + ".edX";
    std::ofstream outFile(directory + "/" + filename);
    if (!outFile)
    {
        std::cerr << "Failed to open file for writing!" << std::endl;
        return;
    }
    outFile << "SceneryName: " << project.sceneryName << "\n";
    outFile << "EditorVersion: " << project.editorVersion << "\n";
    outFile << "XPVersion: " << project.XPVersion << "\n";
    outFile.close();
    std::cout << "File saved to " << directory + "/" + filename << std::endl;
}

void LoadProject(const std::string &filePath, ProjectFile::projectFile &project)
{
    std::ifstream inFile(filePath);
    if (!inFile)
    {
        std::cerr << "Failed to open file for reading!" << std::endl;
        return;
    }
    std::string line;
    while (std::getline(inFile, line))
    {
        if (line.find("SceneryName:") != std::string::npos)
            project.sceneryName = line.substr(line.find(":") + 1);
        else if (line.find("EditorVersion:") != std::string::npos)
            project.editorVersion = line.substr(line.find(":") + 1);
        else if (line.find("XPVersion:") != std::string::npos)
            project.XPVersion = line.substr(line.find(":") + 1);
    }
    inFile.close();
    std::cout << "File loaded from " << filePath << std::endl;
}

void CleanupVulkan(VkInstance instance, VkDevice device, VkCommandPool commandPool, VkDescriptorPool descriptorPool)
{
    if (device != VK_NULL_HANDLE)
    {
        if (commandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device, commandPool, nullptr);
        }
        if (descriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        }
        vkDestroyDevice(device, nullptr);
    }
    if (instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, nullptr);
    }
}

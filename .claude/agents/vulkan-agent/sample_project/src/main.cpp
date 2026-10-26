#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

int main() {
    // Basic Vulkan instance creation with validation layers enabled in Debug builds
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "vulkan_sample";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "sample_engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    std::vector<const char*> extNames;
    std::vector<const char*> layerNames;

#ifdef ENABLE_VULKAN_VALIDATION
    layerNames.push_back("VK_LAYER_KHRONOS_validation");
#endif

    VkInstanceCreateInfo instInfo{};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = static_cast<uint32_t>(extNames.size());
    instInfo.ppEnabledExtensionNames = extNames.empty() ? nullptr : extNames.data();
    instInfo.enabledLayerCount = static_cast<uint32_t>(layerNames.size());
    instInfo.ppEnabledLayerNames = layerNames.empty() ? nullptr : layerNames.data();

    VkInstance instance;
    VkResult r = vkCreateInstance(&instInfo, nullptr, &instance);
    if (r != VK_SUCCESS) {
        std::cerr << "vkCreateInstance failed: " << r << std::endl;
        return -1;
    }

    std::cout << "Vulkan instance created.\n";

    // Normally we'd enumerate physical devices, create device, queues, command pools, etc.
    vkDestroyInstance(instance, nullptr);
    return 0;
}

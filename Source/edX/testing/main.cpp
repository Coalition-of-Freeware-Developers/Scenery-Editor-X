#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>
#include "../edXProjectFile.h"


// Function prototypes
void CheckVkResult(VkResult err);
void SetupVulkan(VkInstance &instance,
                 VkDevice &device,
                 VkPhysicalDevice &physicalDevice,
                 VkQueue &queue,
                 VkCommandPool &commandPool,
                 VkDescriptorPool &descriptorPool);
void CleanupVulkan(VkInstance instance, VkDevice device, VkCommandPool commandPool, VkDescriptorPool descriptorPool);

void SaveProject(const std::string &directory, const ProjectFile::projectFile &project);
void LoadProject(const std::string &filePath, ProjectFile::projectFile &project);

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Vulkan support
    GLFWwindow *window = glfwCreateWindow(800, 600, "edX File Format Tester - Vulkan", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Vulkan setup
    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkQueue queue;
    VkCommandPool commandPool;
    VkDescriptorPool descriptorPool;
    SetupVulkan(instance, device, physicalDevice, queue, commandPool, descriptorPool);

    // Initialize ImGui for Vulkan
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance;
    init_info.PhysicalDevice = physicalDevice;
    init_info.Device = device;
    init_info.QueueFamily = 0; // Set appropriate queue family index
    init_info.Queue = queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = CheckVkResult;
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

        ImGui::Text("Current Directory: %s", directory.c_str());
        if (ImGui::Button("Select Directory"))
        {
            std::cout << "Directory selection not implemented in this example." << std::endl;
        }

        if (ImGui::Button("Save File"))
        {
            project.filename = fileNameBuffer;
            SaveProject(directory, project);
        }

        if (ImGui::Button("Load File"))
        {
            LoadProject(directory + "/" + fileNameBuffer, project);
        }

        ImGui::End();

        // Rendering
        ImGui::Render();
        // Add Vulkan rendering commands here
    }

    // Cleanup
    vkDeviceWaitIdle(device);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    CleanupVulkan(instance, device, commandPool, descriptorPool);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

// Vulkan setup functions (simplified)
void CheckVkResult(VkResult err)
{
    if (err != VK_SUCCESS)
    {
        std::cerr << "Vulkan error: " << err << std::endl;
        abort();
    }
}

void SetupVulkan(VkInstance &instance,
                 VkDevice &device,
                 VkPhysicalDevice &physicalDevice,
                 VkQueue &queue,
                 VkCommandPool &commandPool,
                 VkDescriptorPool &descriptorPool)
{
    // Instance creation
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    CheckVkResult(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

    // Device creation and queue setup
    // (Simplified: Ensure to enumerate physical devices and choose the appropriate one)
    physicalDevice = VK_NULL_HANDLE; // Assign proper device
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
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
}

void CleanupVulkan(VkInstance instance, VkDevice device, VkCommandPool commandPool, VkDescriptorPool descriptorPool)
{
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}

// File save/load implementations
void SaveProject(const std::string &directory, const ProjectFile::projectFile &project)
{
    std::ofstream outFile(directory + "/" + project.filename + ".edX");
    if (!outFile)
    {
        std::cerr << "Failed to open file for writing!" << std::endl;
        return;
    }
    outFile << "SceneryName: " << project.sceneryName << "\n";
    outFile << "EditorVersion: " << project.editorVersion << "\n";
    outFile << "XPVersion: " << project.XPVersion << "\n";
    outFile.close();
    std::cout << "File saved to " << directory + "/" + project.filename + ".edX" << std::endl;
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

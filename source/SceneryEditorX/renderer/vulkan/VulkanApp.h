// VulkanApp.h
// Small C++20 wrapper for the Vulkan sample application.
#pragma once
#include <cstdint>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

struct Vertex 
{
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
};

struct ShaderData 
{
    Mat4 projection;
    Mat4 view;
    Mat4 model[3];
    Vec4 lightPos{ 0.0f, -10.0f, 10.0f, 0.0f };
    uint32_t selected{ 1 };
};

struct ShaderDataBuffer 
{
    VmaAllocation allocation{ VK_NULL_HANDLE };
    VkBuffer buffer{ VK_NULL_HANDLE };
    VkDeviceAddress deviceAddress{};
    void* mapped{ nullptr };
};

struct Texture 
{
    VmaAllocation allocation{ VK_NULL_HANDLE };
    VkImage image{ VK_NULL_HANDLE };
    VkImageView view{ VK_NULL_HANDLE };
    VkSampler sampler{ VK_NULL_HANDLE };
};

class VulkanApp 
{
public:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    VulkanApp(int argc, char* argv[]);
    ~VulkanApp();

    // Run the application. Returns process exit code.
    int Run();

private:
    int argc_{};
    char** argv_{};
};

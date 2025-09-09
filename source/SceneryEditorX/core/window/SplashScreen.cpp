/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* SplashScreen.cpp
* -------------------------------------------------------
* Created: 26/8/2025
* -------------------------------------------------------
*/
//#include "SplashScreen.h"
//#include <stb_image.h>
//#include <GLFW/glfw3.h>
//#include <SceneryEditorX/logging/logging.hpp>
//#include <SceneryEditorX/renderer/vulkan/vk_swapchain.h>
//#include <SceneryEditorX/utils/pointers.h>

/// -------------------------------------------------------

/*
namespace SceneryEditorX
{
    
    SplashScreen::SplashScreen()
    {
        // Load splash screen image using stb_image
        int width, height, channels;
        unsigned char *imageData = stbi_load("assets\\splash_screen.bmp", &width, &height, &channels, STBI_rgb_alpha);
        if (!imageData)
        {
            SEDX_CORE_ERROR_TAG("SPLASH", "Failed to load splash screen image");
            return;
        }

        SEDX_CORE_INFO_TAG("SPLASH", "Loaded splash screen image: {}x{} with {} channels", width, height, channels);

        // Set window hints for splash screen
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);   // Borderless
        glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);     // Always on top
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // Not resizable
        glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);      // Focused on creation
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No OpenGL context needed

        // Create splash screen window centered on screen
        splashWindow = glfwCreateWindow(width, height, "Scenery Editor X", nullptr, nullptr);
        if (!splashWindow)
        {
            SEDX_CORE_ERROR_TAG("SPLASH", "Failed to create splash screen window");
            stbi_image_free(imageData);
            return;
        }

        // Center the splash screen window
        if (const GLFWvidmode *videoMode = glfwGetVideoMode(glfwGetPrimaryMonitor()))
        {
            const int xPos = (videoMode->width - width) / 2;
            const int yPos = (videoMode->height - height) / 2;
            glfwSetWindowPos(splashWindow, xPos, yPos);
            SEDX_CORE_INFO_TAG("SPLASH", "Splash screen positioned at: {}x{}", xPos, yPos);
        }

        // Set window icon for splash screen (reuse existing icon logic)
        //Window::SetWindowIcon(splashWindow);

        // Simple framebuffer approach. For simplicity.
        // Initialize minimal Vulkan context for splash screen
        if (!InitializeSplashContext())
        {
            SEDX_CORE_ERROR_TAG("SPLASH", "Failed to initialize Vulkan context for splash screen");
            glfwDestroyWindow(splashWindow);
            splashWindow = nullptr;
            stbi_image_free(imageData);
            return;
        }

        // Create texture from image data
        if (!CreateSplashTexture(imageData, width, height))
        {
            SEDX_CORE_ERROR_TAG("SPLASH", "Failed to create splash screen texture");
            CleanupSplashContext();
            glfwDestroyWindow(splashWindow);
            splashWindow = nullptr;
            stbi_image_free(imageData);
            return;
        }

        // Free CPU image data
        stbi_image_free(imageData);

        // Render the splash screen
        RenderSplashScreen();

        // Show the window and present
        glfwShowWindow(splashWindow);
        glfwPollEvents();

        SEDX_CORE_INFO_TAG("SPLASH", "Splash screen displayed successfully");
    }

    // Helper method to initialize minimal Vulkan context for splash screen
    bool SplashScreen::InitializeSplashContext()
    {
        // Create Vulkan surface for splash screen window
        VkResult result = glfwCreateWindowSurface(RenderContext::GetInstance(),splashWindow,nullptr, &splashSurface);

        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("SPLASH", "Failed to create Vulkan surface for splash screen");
            return false;
        }

        // Create minimal swap chain for splash screen
        splashSwap = CreateRef<SwapChain>();
        splashSwap->InitSurface(splashWindow);
        splashSwap->Create();

        return true;
    }

    // Helper method to create splash screen texture
    bool SplashScreen::CreateSplashTexture(unsigned char *imageData, int width, int height)
    {
        try
        {
            // Create texture using existing texture creation system
            splashTexture = CreateRef<Texture2D>(imageData, width, height, 4); // RGBA format

            if (!splashTexture /*|| !splashTexture->IsValid()#1#)
            {
                SEDX_CORE_ERROR_TAG("SPLASH", "Failed to create valid splash texture");
                return false;
            }
            
            SEDX_CORE_INFO_TAG("SPLASH", "Splash texture created successfully: {}x{}", width, height);
            return true;
        }
        catch (const std::exception &e)
        {
            SEDX_CORE_ERROR_TAG("SPLASH", "Exception creating splash texture: {}", e.what());
            return false;
        }
    }

    // Helper method to render splash screen
    void SplashScreen::RenderSplashScreen()
    {
        if (!splashSwap || !splashTexture)
        {
            SEDX_CORE_WARN_TAG("SPLASH", "Cannot render splash screen - missing resources");
            return;
        }

        try
        {
            // Begin frame
            splashSwap->BeginFrame();
            splashSwap->AcquireNextImage();

            // Get command buffer
            VkCommandBuffer cmdBuffer = splashSwap->GetActiveDrawCommandBuffer();

            // Simple render pass to clear and present the texture
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = splashSwap->GetRenderPass();
            renderPassInfo.framebuffer = splashSwap->GetActiveFramebuffer();

            int fbWidth, fbHeight;
            glfwGetFramebufferSize(splashWindow, &fbWidth, &fbHeight);
            renderPassInfo.renderArea.offset = {.x = 0,.y = 0};
            renderPassInfo.renderArea.extent = {.width = static_cast<uint32_t>(fbWidth),.height = static_cast<uint32_t>(fbHeight)};

            VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            // Here you would normally render the texture using a quad
            // For simplicity, we'll just clear to black and present
            // In a full implementation, you'd render a fullscreen quad with the splash texture

            vkCmdEndRenderPass(cmdBuffer);

            // Present the frame
            splashSwap->Present(nullptr);
        }
        catch (const std::exception &e)
        {
            SEDX_CORE_ERROR_TAG("SPLASH", "Exception rendering splash screen: {}", e.what());
        }
    }

    void SplashScreen::CleanupSplashContext()
    {
        if (splashTexture)
        {
            splashTexture.Reset();
        }

        if (splashSwap)
        {
            splashSwap->Destroy();
            splashSwap.Reset();
        }

        if (splashSurface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(RenderContext::GetInstance(), splashSurface, nullptr);
            splashSurface = VK_NULL_HANDLE;
        }
    }

    // Method to hide and cleanup splash screen
    void SplashScreen::HideSplashScreen()
    {
        if (splashWindow)
        {
            SEDX_CORE_INFO_TAG("SPLASH", "Hiding splash screen");

            // Cleanup Vulkan resources
            CleanupSplashContext();

            // Destroy GLFW window
            glfwDestroyWindow(splashWindow);
            splashWindow = nullptr;

            SEDX_CORE_INFO_TAG("SPLASH", "Splash screen cleanup complete");
        }
    }
}
*/

/// -------------------------------------------------------

#ifndef COMMON
#define COMMON

#ifdef _WIN32
#include <Windows.h>
#elif defined __linux
#include <dlfcn.h>
#endif

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <cstring>
#include <thread>
#include <cmath>
#include <functional>
#include <memory>
//#include "VulkanDestroyer.h"

namespace SceneryEditorX
{
    
    // Vulkan library type
    #ifdef _WIN32
    #define LIBRARY_TYPE HMODULE
    #elif defined __linux
    #define LIBRARY_TYPE void *
    #endif
    
    // OS-specific parameters
    struct WindowParameters
    {
        #ifdef VK_USE_PLATFORM_WIN32_KHR
        
            HINSTANCE HInstance;
            HWND HWnd;
        
        #elif defined VK_USE_PLATFORM_XLIB_KHR
        
            Display *Dpy;
            Window Window;
        
        #elif defined VK_USE_PLATFORM_XCB_KHR
        
            xcb_connection_t *Connection;
            xcb_window_t Window;
        
        #endif
    };
    
    // Extension availability check
    bool IsExtensionSupported(std::vector<VkExtensionProperties> const &available_extensions, char const *const extension);
    
} // namespace SceneryEditorX

#endif // COMMON

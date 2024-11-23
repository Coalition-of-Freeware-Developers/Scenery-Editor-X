#include <windows.h>
#include <shellapi.h>
#include <thread>
#include <iostream>
#include <string>
#include <sysinfoapi.h>

#include "resource.h"
#include "DirectoryManager.hpp"
#include "Logging.hpp"
#include "RegInit.h"
#include <filesystem>
#include "ElevatePerms.h"
#include "RegistryCheck.h"

namespace Launcher
{
    class Loader
    {
    public:
        void run()
        {
            CreateSplash();
            Setup();
            MainLoop();
            Finish();
        }

        void Setup()
        {
            OperationThreads();
            PerformPreloading();
        }

        // Global Variables
        HBITMAP hSplashBitmap; // Handle for the splash screen image
        HWND hSplashWnd;       // Handle for the splash window
    
        static LRESULT CALLBACK SplashWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            Loader *pThis = nullptr;
            if (message == WM_NCCREATE)
            {
                CREATESTRUCT *pCreate = (CREATESTRUCT *)lParam;
                pThis = (Loader *)pCreate->lpCreateParams;
                SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
            }
            else
            {
                pThis = (Loader *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            }

            if (pThis)
            {
                return pThis->RealSplashWndProc(hWnd, message, wParam, lParam);
            }
            return DefWindowProc(hWnd, message, wParam, lParam);
        }

        LRESULT RealSplashWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            switch (message)
            {
            case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
                HDC hdcMem = CreateCompatibleDC(hdc);
                HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hSplashBitmap);
                BITMAP bitmap;
                GetObject(hSplashBitmap, sizeof(bitmap), &bitmap);
                BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);
                SelectObject(hdcMem, hbmOld);
                DeleteDC(hdcMem);
                EndPaint(hWnd, &ps);
            }
            break;
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
            return 0;
        }
    
        void CreateSplash()
        {
            // Load the splash screen bitmap
            hSplashBitmap = LoadBitmap(GetModuleHandle(nullptr), MAKEINTRESOURCE(SEDX_SPLASH));
            if (!hSplashBitmap)
            {
                spdlog::error("Failed to load splash screen image.");
                MessageBox(nullptr, "Failed to load splash screen image.", "Error", MB_OK | MB_ICONERROR);
                return;
            }
    
            // Register splash window class
            WNDCLASS wc = {};
            wc.lpfnWndProc = SplashWndProc;
            wc.hInstance = GetModuleHandle(nullptr);
            wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
            wc.lpszClassName = "SplashScreen";
            RegisterClass(&wc);
    
            // Get bitmap dimensions
            BITMAP bitmap;
            GetObject(hSplashBitmap, sizeof(BITMAP), &bitmap);
    
            // Create the splash window
            hSplashWnd = CreateWindowEx(0,
                                        "SplashScreen",
                                        nullptr,
                                        WS_POPUP | WS_VISIBLE,
                                        (GetSystemMetrics(SM_CXSCREEN) - bitmap.bmWidth) / 2,
                                        (GetSystemMetrics(SM_CYSCREEN) - bitmap.bmHeight) / 2,
                                        bitmap.bmWidth,
                                        bitmap.bmHeight,
                                        nullptr,
                                        nullptr,
                                        GetModuleHandle(nullptr),
                                        nullptr);
    
            if (!hSplashWnd)
            {
                spdlog::critical("Failed to create splash screen window.");
                MessageBox(nullptr, "Failed to create splash screen window.", "Error", MB_OK | MB_ICONERROR);
                return;
            }
    
            // Message loop for the splash screen
            MSG msg;
            while (GetMessage(&msg, nullptr, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        void OperationThreads()
        {
            // Create a thread to perform the preloading tasks
            std::thread preloadThread(&Loader::PerformPreloading, this);
            preloadThread.detach();
        }

        // Function to perform preloading tasks
        void PerformPreloading()
        {
            Sleep(600);
            spdlog::info("Preloading tasks started.");
    
            // TODO: Add Scenery Gateway API pull and cache
            // Get All Airports (GET /apiv1/airports)
            //
            // TODO: Add Check for app updates
            // TODO: Add Check for directory to X-Plane 12
            // This can be done by checking the registry for the X-Plane 12 directory
    
            Sleep(600);
            spdlog::info("Loading resources.");


            RegistryCheck();
            spdlog::info("Registry init complete");

            Sleep(8000);
            spdlog::info("Preloading tasks completed.");
        }

        void MainLoop()
        {
            // Main loop
            MSG msg;
            while (GetMessage(&msg, nullptr, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        void Finish() const
        {

            spdlog::info("Launching main program.");
            // After preloading, launch the main program
            ShellExecute(nullptr, "open", "SceneryEditorX.exe", nullptr, nullptr, SW_SHOWDEFAULT);

            // Close the splash screen
            spdlog::info("Closing splash screen.");
            PostMessage(hSplashWnd, WM_CLOSE, 0, 0);
        }
    };
} // namespace Launcher

// Entry point
int APIENTRY WinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPSTR lpCmdLine,
                     _In_ int nCmdShow)
{
    Launcher::Log::Init();
    Launcher::Log::LogHeader();
    spdlog::info("Launcher is starting...");

    Launcher::Loader SplashScreen{};
    try
    {
        SplashScreen.run();
    }
    catch (const std::exception &e)
    {
        spdlog::error("An exception occurred: {}", e.what());
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return 0;
}

#include <windows.h>
#include "Logging.hpp"
#include "resource.h"
#include <basetsd.h>
#include <condition_variable>
#include <libloaderapi.h>
#include <mutex>
#include <spdlog/spdlog.h>
#include <thread>


class SplashHandler
{
public:
    // Global Variables
    HBITMAP hSplashBitmap;          // Handle for the splash screen image
    HWND hSplashWnd;                // Handle for the splash window
    std::thread splashThread;       // Thread for the splash screen
    std::mutex mtx;                 // Mutex for the splash screen
    std::condition_variable cv;     // Condition variable for the splash screen
    bool splashCreated = false;     // Flag to indicate if the splash screen has been created

    /**
    * @brief Window procedure for the splash screen window.
    * 
    * This function handles messages sent to the splash screen window.
    * It processes the WM_NCCREATE, WM_PAINT, and WM_DESTROY messages.
    * 
    * @param hWnd Handle to the window.
    * @param uMsg Message identifier.
    * @param wParam Additional message information.
    * @param lParam Additional message information.
    * @return LRESULT Result of the message processing.
    */
    static LRESULT CALLBACK SplashWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        SplashHandler *pThis = nullptr;

        if (uMsg == WM_NCCREATE)
        {
            CREATESTRUCT *pCreate = (CREATESTRUCT *)lParam;
            pThis = (SplashHandler *)pCreate->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);

            pThis->hSplashWnd = hWnd;
        }
        else
        {
            pThis = (SplashHandler *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        }

        if (pThis)
        {
            switch (uMsg)
            {
            case WM_PAINT:
            {
                //spdlog::info("Painting splash screen.");
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
                HDC hMemDC = CreateCompatibleDC(hdc);
                SelectObject(hMemDC, pThis->hSplashBitmap);
                BITMAP bitmap;
                GetObject(pThis->hSplashBitmap, sizeof(BITMAP), &bitmap);
                BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hMemDC, 0, 0, SRCCOPY);
                DeleteDC(hMemDC);
                EndPaint(hWnd, &ps);
                return 0;
            }
            case WM_DESTROY:
                //spdlog::info("Destroying splash screen.");
                PostQuitMessage(0);
                return 0;
            }
        }

        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    /**
    * @brief Displays the splash screen.
    * 
    * This function loads the splash screen bitmap, registers the splash window class,
    * creates the splash window, and enters the message loop to display the splash screen.
    * 
    * @param hInstance Handle to the instance of the application.
    */
    void ShowSplashScreen(HINSTANCE hInstance)
    {
        // Load the splash screen bitmap
        hSplashBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(SEDX_SPLASH));
        if (!hSplashBitmap)
        {
            //spdlog::error("Failed to load splash screen image.");
            MessageBox(nullptr, "Failed to load splash screen image.", "Error", MB_OK | MB_ICONERROR);
            return;
        }

        // Register splash window class
        WNDCLASS wc = {};
        wc.lpfnWndProc = SplashWndProc;
        wc.hInstance = hInstance;
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
                                    hInstance,
                                    this);

        if (!hSplashWnd)
        {
            //spdlog::critical("Failed to create splash screen window.");
            MessageBox(nullptr, "Failed to create splash screen window.", "Error", MB_OK | MB_ICONERROR);
            return;
        }

        // Notify that the splash screen has been created
        {
            std::lock_guard<std::mutex> lock(mtx);
            splashCreated = true;
        }
        cv.notify_one();

        // Message loop for the splash screen
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Function to create the splash screen
    void CreateSplashScreen()
    {
        splashThread = std::thread(&SplashHandler::ShowSplashScreen, this, GetModuleHandle(nullptr));

        // Wait until the splash screen is created
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return splashCreated; });
    }

    // Function to close the splash screen
    void DestroySplashScreen()
    {
        // Close the splash screen
        spdlog::info("Closing splash screen.");

        PostMessage(hSplashWnd, WM_CLOSE, 0, 0);
        if (splashThread.joinable())
        {
            splashThread.join();
        }

        DeleteObject(hSplashBitmap);
    }
};

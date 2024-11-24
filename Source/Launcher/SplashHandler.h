#include <windows.h>
#include "resource.h"
#include "Logging.hpp"

class SplashHandler
{
public:
    // Global Variables
    HBITMAP hSplashBitmap; // Handle for the splash screen image
    HWND hSplashWnd;       // Handle for the splash window

    // Static function to create the splash screen window
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
                spdlog::info("Painting splash screen.");
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
                spdlog::info("Destroying splash screen.");
                PostQuitMessage(0);
                return 0;
            }
        }

        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    // Function to create and display the splash screen
    void ShowSplashScreen(HINSTANCE hInstance)
    {
        // Load the splash screen bitmap
        hSplashBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(SEDX_SPLASH));
        if (!hSplashBitmap)
        {
            spdlog::error("Failed to load splash screen image.");
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

    // Function to create the splash screen
    void CreateSplashScreen()
    {
        ShowSplashScreen(GetModuleHandle(nullptr));
    }

    // Function to close the splash screen
    void DestroySplashScreen()
    {
        // Close the splash screen
        spdlog::info("Closing splash screen.");

        DestroyWindow(hSplashWnd);
        DeleteObject(hSplashBitmap);

        // Post a close message to the splash screen window
        //PostMessage(hSplashWnd, WM_CLOSE, 0, 0);
    }
};

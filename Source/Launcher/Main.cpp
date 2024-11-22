#include <windows.h>
#include <thread>
#include <shellapi.h>

// Global Variables
HBITMAP hSplashBitmap; // Handle for the splash screen image
HWND hSplashWnd;       // Handle for the splash window

// Function to create the splash screen window
LRESULT CALLBACK SplashWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        HDC hMemDC = CreateCompatibleDC(hdc);
        SelectObject(hMemDC, hSplashBitmap);
        BITMAP bitmap;
        GetObject(hSplashBitmap, sizeof(BITMAP), &bitmap);
        BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hMemDC, 0, 0, SRCCOPY);
        DeleteDC(hMemDC);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Function to perform preloading tasks
void PerformPreloading()
{
    // Simulate preloading tasks (replace with actual functions)
    Sleep(3000); // Simulating tasks taking 3 seconds
    // After preloading, launch the main program
    ShellExecute(nullptr, L"open", L"Scenery Editor X.exe", nullptr, nullptr, SW_SHOWDEFAULT);
    // Close the splash screen
    PostMessage(hSplashWnd, WM_CLOSE, 0, 0);
}

// Function to create and display the splash screen
void ShowSplashScreen(HINSTANCE hInstance)
{
    // Load the splash screen bitmap
    hSplashBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(SEDX_SPLASH));
    if (!hSplashBitmap)
    {
        MessageBox(nullptr, L"Failed to load splash screen image.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Register splash window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = SplashWndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = L"SplashScreen";
    RegisterClass(&wc);

    // Get bitmap dimensions
    BITMAP bitmap;
    GetObject(hSplashBitmap, sizeof(BITMAP), &bitmap);

    // Create the splash window
    hSplashWnd = CreateWindowEx(0,
                                L"SplashScreen",
                                nullptr,
                                WS_POPUP | WS_VISIBLE,
                                (GetSystemMetrics(SM_CXSCREEN) - bitmap.bmWidth) / 2,
                                (GetSystemMetrics(SM_CYSCREEN) - bitmap.bmHeight) / 2,
                                bitmap.bmWidth,
                                bitmap.bmHeight,
                                nullptr,
                                nullptr,
                                hInstance,
                                nullptr);

    if (!hSplashWnd)
    {
        MessageBox(nullptr, L"Failed to create splash screen window.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Start the preloading tasks in a separate thread
    std::thread preloadThread(PerformPreloading);
    preloadThread.detach();

    // Message loop for the splash screen
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// Entry point
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    ShowSplashScreen(hInstance);
    return 0;
}

#include <windows.h>
#include <commdlg.h>
#include <string>
#include <iostream>
#include <ShlObj_core.h>
#include <shobjidl.h>

void ShowFileBrowser()
{
    OPENFILENAME ofn; // common dialog box structure
    char szFile[260]; // buffer for file name
    HWND hwnd = NULL; // owner window
    HANDLE hf;        // file handle

    // Initialize OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    // Set lpstrFile[0] to '\0' so that GetOpenFileName does not
    // use the contents of szFile to initialize itself.
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "EDX Project\0*.edX\0All\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // Display the Open dialog box.
    if (GetOpenFileName(&ofn) == TRUE)
    {
        std::cout << "Selected file: " << ofn.lpstrFile << std::endl;
    }
    else
    {
        std::cout << "No file selected or an error occurred." << std::endl;
    }
}

void ShowDirectoryBrowser()
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        std::cout << "Failed to initialize COM library." << std::endl;
        return;
    }

    IFileDialog *pfd = NULL;
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));

    if (SUCCEEDED(hr))
    {
        DWORD dwOptions;
        hr = pfd->GetOptions(&dwOptions);
        if (SUCCEEDED(hr))
        {
            hr = pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
        }

        if (SUCCEEDED(hr))
        {
            hr = pfd->Show(NULL);
            if (SUCCEEDED(hr))
            {
                IShellItem *psi;
                hr = pfd->GetResult(&psi);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFolderPath = NULL;
                    hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszFolderPath);
                    if (SUCCEEDED(hr))
                    {
                        std::wcout << L"Selected folder: " << pszFolderPath << std::endl;
                        CoTaskMemFree(pszFolderPath);
                    }
                    psi->Release();
                }
            }
        }
        pfd->Release();
    }
    if (FAILED(hr))
    {
        std::cout << "No folder selected or an error occurred." << std::endl;
    }

    CoUninitialize();
}

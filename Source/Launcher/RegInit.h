#pragma once

#include "atlbase.h"
#include "resource.h"

void RegisterEDXAssociation()
{
    CComModule _Module; // ATL module object for handling registry operations
    HRESULT hr;

    // Initialize the module
    hr = _Module.Init(NULL, GetModuleHandle(NULL));
    if (FAILED(hr))
    {
        MessageBox(NULL, "Failed to initialize the module", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Update the registry using the .rgs file
    hr = _Module.UpdateRegistryFromResource(IDR_REGISTRY_EDX_FORMAT, TRUE);
    if (FAILED(hr))
    {
        MessageBox(NULL, "Failed to register file association", "Error", MB_OK | MB_ICONERROR);
    }

    // Clean up
    _Module.Term();
}

void RegisterLibraryAssociation()
{
    CComModule _Module; // ATL module object for handling registry operations
    HRESULT hr;

    // Initialize the module
    hr = _Module.Init(NULL, GetModuleHandle(NULL));
    if (FAILED(hr))
    {
        MessageBox(NULL, "Failed to initialize the module", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Update the registry using the .rgs file
    hr = _Module.UpdateRegistryFromResource(IDR_REGISTRY_LIBRARY_FORMAT, TRUE);
    if (FAILED(hr))
    {
        MessageBox(NULL, "Failed to register file association", "Error", MB_OK | MB_ICONERROR);
    }

    // Clean up
    _Module.Term();
}

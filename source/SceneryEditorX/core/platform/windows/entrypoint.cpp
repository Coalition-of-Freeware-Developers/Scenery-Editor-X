/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* entrypoint.cpp
* -------------------------------------------------------
* Windows entry point for the application
* -------------------------------------------------------
* Created: 25/5/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/platform/entrypoint.h>
#include <SceneryEditorX/core/platform/windows/context.h>
#include <Windows.h>
// -----------------------------------------------

std::unique_ptr<SceneryEditorX::PlatformContext> CreatePlatformContext(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
    return std::make_unique<SceneryEditorX::WindowsContext>(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

// -----------------------------------------------

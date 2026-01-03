/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* context.h
* -------------------------------------------------------
* Windows application context
* -------------------------------------------------------
* Created: 25/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <Windows.h>
#include <string>
#include <SceneryEditorX/core/platform/platform_context.h>
// -----------------------------------------------

namespace SceneryEditorX
{
	class WindowsContext final : public PlatformContext
	{
	public:
		WindowsContext(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow);
		~WindowsContext() override = default;

	};

} // namespace SceneryEditorX

// -----------------------------------------------

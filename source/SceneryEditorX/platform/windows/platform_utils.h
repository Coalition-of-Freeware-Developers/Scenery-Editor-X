/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* PlatformUtils.h
* -------------------------------------------------------
* Created: 17/3/2025
* -------------------------------------------------------
*/
#pragma once

#include <string>

// -------------------------------------------------------

namespace SceneryEditorX::IO {

	class FileDialogs
	{
	public:
		// These return empty strings if cancelled
		static std::string OpenFile(const char* filter);
		static std::string SaveFile(const char* filter);
	};

	class Time
	{
	public:
		static float GetTime();
	};

} // namespace SceneryEditorX::IO

// -------------------------------------------------------

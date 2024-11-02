
#pragma once

#include <string>
#include <vector>

class FileManager
{
public:
    static std::vector<char> ReadRawBytes(const std::string &filename);
};


/*
namespace SceneryEditorX {

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

}
*/

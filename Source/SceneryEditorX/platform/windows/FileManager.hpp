
#pragma once

#include <string>
#include <vector>

class FileManager
{
public:
    /**
     * @brief Reads raw bytes from a file.
     * 
     * This function opens the specified file and reads its contents into a vector of bytes.
     * 
     * @param filename The name of the file to read.
     * @return A vector containing the raw bytes read from the file.
     */
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

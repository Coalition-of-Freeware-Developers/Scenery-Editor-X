#include "VK_Pipeline.hpp"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <windows.h>

//#ifndef ENGINE_DIR
//#ifdef _DEBUG
//#define ENGINE_DIR "./" // This is the path to the engine directory
//#else
//#define ENGINE_DIR "./" // This is the path to the engine directory
//#endif
//#endif

/*
* IMPORTANT NOTE:
* ########################
* IF YOU KEEP GETTING ERROR SHADERS NOT FOUND
* CHECK THE PATHS BELOW AS WELL AS THE SHADER COMPILER PATHS
* LOCATED IN THE "VK_Wrapper.cpp" FILE AS THE SHADERS ARE TO BE COMPILED
* ########################
*/

namespace fs = std::filesystem;

namespace SceneryEditorX
{

std::string getRegistryKeyPath()
{
    HKEY hKey;
    char path[256];
    DWORD pathLen = sizeof(path);
    std::string registryPath = "SOFTWARE\\SceneryEditorX";

    if (RegOpenKeyExA(HKEY_CURRENT_USER, registryPath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExA(hKey, "Location", NULL, NULL, (LPBYTE)path, &pathLen) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return std::string(path, pathLen - 1); // Remove the null terminator
        }
        RegCloseKey(hKey);
    }
    throw std::runtime_error("Failed to get registry key path");
}

VK_Pipeline::VK_Pipeline(const std::string &vertFilepath, const std::string &fragFilepath)
{
    // Get the path from the registry
    std::string absolutePath = getRegistryKeyPath();
    fs::path exeDir = fs::path(absolutePath).parent_path();
    fs::path shaderDir = exeDir / "resources" / "cache";

    // Combine with provided shader filenames
    fs::path vertexShaderPath = shaderDir / vertFilepath;
    fs::path fragmentShaderPath = shaderDir / fragFilepath;

    // Log paths for debugging
    spdlog::info("Vertex Shader Path: {}", vertexShaderPath.string());
    spdlog::info("Fragment Shader Path: {}", fragmentShaderPath.string());

    // Create graphics pipeline with the specified shader paths
    createGraphicsPipeline(vertexShaderPath.string(), fragmentShaderPath.string());
}


/**
     * @brief Reads the contents of a file and returns it as a vector of characters.
     * 
     * This function opens the specified file in binary mode and reads its contents
     * into a vector of characters. If the file cannot be opened, it throws a runtime error.
     * 
     * @param filePath The path to the file to be read.
     * @return std::vector<char> A vector containing the contents of the file.
     * @throws std::runtime_error If the file cannot be opened.
     */
std::vector<char> VK_Pipeline::readFile(const std::string &filePath)
{
    // Open the file at the end (ate) and in binary mode
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    // Check if the file is open, if not, throw a runtime error with the file path
    if (!file.is_open())
    {
        spdlog::error("Failed to open shader files: {}", filePath);
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    if (!fs::exists(filePath))
    {
        spdlog::error("Shader file does not exist: {}", filePath);
        throw std::runtime_error("Shader file does not exist: " + filePath);
    }

    size_t fileSize = file.tellg(); // Get the size of the file
    //size_t fileSize = static_cast<size_t>(file.tellg());  // Get the size of the file
    std::vector<char> buffer(fileSize); // Create a buffer to store the file contents

    file.seekg(0);                      // Move the file pointer to the beginning of the file
    file.read(buffer.data(), fileSize); // Read the file contents into the buffer
    file.close();                       // Close the file

    return buffer; // Return the buffer
}

/**
     * @brief Creates a graphics pipeline using the specified vertex and fragment shader files.
     * 
     * This function reads the vertex and fragment shader files, logs their sizes, and
     * creates a graphics pipeline. The shader files are read using the readFile function.
     * 
     * @param vertFilepath The path to the vertex shader file.
     * @param fragFilepath The path to the fragment shader file.
     */
void VK_Pipeline::createGraphicsPipeline(const std::string &vertFilepath, const std::string &fragFilepath)
{
    // Read shader files
    std::vector<char> vertCode = VK_Pipeline::readFile(vertFilepath);
    std::vector<char> fragCode = VK_Pipeline::readFile(fragFilepath);

    spdlog::info("Vertex Shader Code Size: {}", vertCode.size());
    spdlog::info("Fragment Shader Code Size: {}", fragCode.size());
}

} // namespace SceneryEditorX

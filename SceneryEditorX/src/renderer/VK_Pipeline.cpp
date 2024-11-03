#include "../src/xpeditorpch.h"
#include "VK_Pipeline.hpp"

//#ifndef ENGINE_DIR 
//#define ENGINE_DIR "../" // This is the path to the engine directory
#ifndef ENGINE_DIR 
#ifdef _DEBUG
#define ENGINE_DIR "../shaders" // Path for debug mode
#else
#define ENGINE_DIR "/shaders/" // Path for release mode
#endif
#endif

namespace SceneryEditorX
{
    VK_Pipeline::VK_Pipeline(const std::string &vertFilepath, const std::string &fragFilepath)
    {
        createGraphicsPipeline(vertFilepath, fragFilepath);
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
            //throw std::runtime_error("Failed to open file: " + std::filesystem::absolute(filePath).string());
            throw std::runtime_error("Failed to open file: " + filePath);
        }

        size_t fileSize = static_cast<size_t>(file.tellg());    // Get the size of the file
        std::vector<char> buffer(fileSize);                     // Create a buffer to store the file contents

        file.seekg(0);                                          // Move the file pointer to the beginning of the file
        file.read(buffer.data(), fileSize);                     // Read the file contents into the buffer

        file.close();                                           // Close the file
        return buffer;                                          // Return the buffer
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
        auto vertCode = readFile(vertFilepath);
        auto fragCode = readFile(fragFilepath);

        spdlog::info("Vertex Shader Code Size: {}", vertCode.size());
        spdlog::info("Fragment Shader Code Size: {}", fragCode.size());
    }

}

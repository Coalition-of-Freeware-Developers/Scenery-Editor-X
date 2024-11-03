#include "../src/xpeditorpch.h"
#include "VK_Pipeline.hpp"

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace SceneryEditorX
{
VK_Pipeline::VK_Pipeline(const std::string &vertFilepath, const std::string &fragFilepath)
{
    createGraphicsPipeline(vertFilepath, fragFilepath);
}
    
    std::vector<char> VK_Pipeline::readFile(const std::string &filePath)
    {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            //throw std::runtime_error("Failed to open file: " + std::filesystem::absolute(filePath).string());
            throw std::runtime_error("Failed to open file: " + filePath);
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer;
    }

    void VK_Pipeline::createGraphicsPipeline(const std::string &vertFilepath, const std::string &fragFilepath)
    {
        auto vertCode = readFile(vertFilepath);
        auto fragCode = readFile(fragFilepath);

        spdlog::info("Vertex Shader Code Size: {}", vertCode.size());
        spdlog::info("Fragment Shader Code Size: {}", fragCode.size());
    }

}

#pragma once

#include <string>
#include <vector>

namespace SceneryEditorX
{
    class VK_Pipeline
    {
    public:
        VK_Pipeline(const std::string &vertFilepath, const std::string &fragFilepath); // Constructor
    
    private:
        static std::vector<char> readFile(const std::string &filePath); // Read file
    
        void createGraphicsPipeline(const std::string &vertFilepath,
                                    const std::string &fragFilepath); // Create graphics pipeline
    };
}

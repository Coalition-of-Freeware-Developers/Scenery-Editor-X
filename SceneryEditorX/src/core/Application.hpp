#pragma once

#include "VK_Window.hpp"
#include "../renderer/VK_Pipeline.hpp"


namespace SceneryEditorX
{
    class Application
    {
    public:
        std::string Title = "Scenery Editor X";
        static inline uint32_t Width = 1280;
        static inline uint32_t Height = 720;

    void run();

    private:
        //void FileCheck(0, nullptr);
        VK_Window VK_Window{Width, Height, Title};
        VK_Pipeline VK_Pipeline{"../shaders/vert.spv", "../shaders/frag.spv"};
    };
}

#pragma once

#include <core/VK_Window.hpp>
#include <renderer/VK_Pipeline.hpp>

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
    VK_Window VK_Window{static_cast<int>(Width), static_cast<int>(Height), Title};
    VK_Pipeline VK_Pipeline{"../shaders/vert.spv", "../shaders/frag.spv"};
};
} // namespace SceneryEditorX

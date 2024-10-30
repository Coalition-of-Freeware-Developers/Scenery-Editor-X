#include "../src/xpeditorpch.h"
#include "Application.hpp"

int main(int, char **)
{
    SceneryEditorX::Application app;

    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

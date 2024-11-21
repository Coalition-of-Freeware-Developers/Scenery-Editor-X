#include <core/MainWindow.h>

#ifdef SCENERYEDITORX_PLATFORM_WINDOWS
#include "WindowProperties.h"
#endif

namespace SceneryEditorX
{
Scope<Window> Window::Create(const WindowProperties &properties)
{
#ifdef SCENERYEDITORX_PLATFORM_WINDOWS
    return CreateScope<WindowProperties>(properties);
#else
    SCENERYEDITORX_CORE_ASSERT(false, "Unknown platform!");
    return nullptr;
#endif
}

} // namespace SceneryEditorX

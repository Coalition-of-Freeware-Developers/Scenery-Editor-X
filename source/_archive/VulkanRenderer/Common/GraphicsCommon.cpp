#include "GraphicsCommon.h"

namespace SceneryEditorX
{
    bool IsExtensionSupported(std::vector<VkExtensionProperties> const &available_extensions, char const *const extension)
    {
        for (auto &available_extension : available_extensions)
        {
            if (strstr(available_extension.extensionName, extension))
            {
                return true;
            }
        }
        return false;
    }
} // namespace SceneryEditorX

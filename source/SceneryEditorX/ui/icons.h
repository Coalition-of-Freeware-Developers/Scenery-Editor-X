/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* icons.h
* -------------------------------------------------------
* Created: 2/4/2025
* -------------------------------------------------------
*/

#pragma once
#include <IconsFontAwesome5.h>
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/ui/ui_manager.h>
#include <SceneryEditorX/ui/colors.h>
#include <SceneryEditorX/ui/fonts.h>
#include <SceneryEditorX/core/layer.h>

// -------------------------------------------------------

struct Icons
{
    // Core UI icons
    GLOBAL constexpr const char* ARROW_RIGHT	= ICON_FA_ARROW_RIGHT;
    GLOBAL constexpr const char* ARROW_LEFT		= ICON_FA_ARROW_LEFT;
    GLOBAL constexpr const char* ARROW_UP		= ICON_FA_ARROW_UP;
    GLOBAL constexpr const char* ARROW_DOWN		= ICON_FA_ARROW_DOWN;
    GLOBAL constexpr const char* PLUS			= ICON_FA_PLUS;
    GLOBAL constexpr const char* MINUS			= ICON_FA_MINUS;
    GLOBAL constexpr const char* CHECK			= ICON_FA_CHECK;
    GLOBAL constexpr const char* TIMES			= ICON_FA_TIMES;
    GLOBAL constexpr const char* TRASH			= ICON_FA_TRASH;
    GLOBAL constexpr const char* SAVE			= ICON_FA_SAVE;
    GLOBAL constexpr const char* FOLDER			= ICON_FA_FOLDER;
    GLOBAL constexpr const char* FOLDER_OPEN	= ICON_FA_FOLDER_OPEN;
    GLOBAL constexpr const char* FILE			= ICON_FA_FILE;
    GLOBAL constexpr const char* FILE_ALT		= ICON_FA_FILE_ALT;

    // Editor tools
    GLOBAL constexpr const char* PENCIL			= ICON_FA_PENCIL_ALT;
    GLOBAL constexpr const char* EDIT			= ICON_FA_EDIT;
    GLOBAL constexpr const char* MOVE			= ICON_FA_ARROWS_ALT;
    GLOBAL constexpr const char* ROTATE			= ICON_FA_SYNC;
    GLOBAL constexpr const char* SCALE			= ICON_FA_EXPAND;
    GLOBAL constexpr const char* UNDO			= ICON_FA_UNDO;
    GLOBAL constexpr const char* REDO			= ICON_FA_REDO;
    GLOBAL constexpr const char* EYE			= ICON_FA_EYE;
    GLOBAL constexpr const char* EYE_SLASH		= ICON_FA_EYE_SLASH;
    GLOBAL constexpr const char* LOCK			= ICON_FA_LOCK;
    GLOBAL constexpr const char* UNLOCK			= ICON_FA_UNLOCK;

    // 3D visualization
    GLOBAL constexpr const char* CUBE			= ICON_FA_CUBE;
    GLOBAL constexpr const char* CUBES			= ICON_FA_CUBES;
    GLOBAL constexpr const char* MOUNTAIN		= ICON_FA_MOUNTAIN;
    GLOBAL constexpr const char* MAP			= ICON_FA_MAP;
    GLOBAL constexpr const char* COMPASS		= ICON_FA_COMPASS;
    GLOBAL constexpr const char* LOCATION		= ICON_FA_MAP_MARKER_ALT;
    GLOBAL constexpr const char* RULER			= ICON_FA_RULER_COMBINED;
    GLOBAL constexpr const char* CROSSHAIRS		= ICON_FA_CROSSHAIRS;
    GLOBAL constexpr const char* OBJECT_GROUP	= ICON_FA_OBJECT_GROUP;
    GLOBAL constexpr const char* OBJECT_UNGROUP = ICON_FA_OBJECT_UNGROUP;

    // Interface elements
    GLOBAL constexpr const char* COG			= ICON_FA_COG;
    GLOBAL constexpr const char* COGS			= ICON_FA_COGS;
    GLOBAL constexpr const char* BARS			= ICON_FA_BARS;
    GLOBAL constexpr const char* QUESTION		= ICON_FA_QUESTION;
    GLOBAL constexpr const char* INFO			= ICON_FA_INFO_CIRCLE;
    GLOBAL constexpr const char* EXCLAMATION	= ICON_FA_EXCLAMATION_TRIANGLE;
    GLOBAL constexpr const char* SEARCH			= ICON_FA_SEARCH;
    GLOBAL constexpr const char* HOME			= ICON_FA_HOME;
    GLOBAL constexpr const char* DOWNLOAD		= ICON_FA_DOWNLOAD;
    GLOBAL constexpr const char* UPLOAD			= ICON_FA_UPLOAD;
    GLOBAL constexpr const char* SYNC			= ICON_FA_SYNC_ALT;
    GLOBAL constexpr const char* PLAY			= ICON_FA_PLAY;
    GLOBAL constexpr const char* PAUSE			= ICON_FA_PAUSE;
    GLOBAL constexpr const char* STOP			= ICON_FA_STOP;

// -------------------------------------------------------

    // Helper method to get font icon
    static const char* GetIcon(const std::string& name)
    {
        // Map common names to icon constants
        static const std::unordered_map<std::string, const char*> iconMap = {
            {"arrow_right", ARROW_RIGHT},
            {"arrow_left", ARROW_LEFT},
            {"arrow_up", ARROW_UP},
            {"arrow_down", ARROW_DOWN},
            {"plus", PLUS},
            {"minus", MINUS},
            {"check", CHECK},
            {"times", TIMES},
            {"trash", TRASH},
            {"save", SAVE},
            {"folder", FOLDER},
            {"folder_open", FOLDER_OPEN},
            {"file", FILE},
            {"file_alt", FILE_ALT},
            {"pencil", PENCIL},
            {"edit", EDIT},
            {"move", MOVE},
            {"rotate", ROTATE},
            {"scale", SCALE},
            {"undo", UNDO},
            {"redo", REDO},
            {"eye", EYE},
            {"eye_slash", EYE_SLASH},
            {"lock", LOCK},
            {"unlock", UNLOCK},
            {"cube", CUBE},
            {"cubes", CUBES},
            {"mountain", MOUNTAIN},
            {"map", MAP},
            {"compass", COMPASS},
            {"location", LOCATION},
            {"ruler", RULER},
            {"crosshairs", CROSSHAIRS},
            {"object_group", OBJECT_GROUP},
            {"object_ungroup", OBJECT_UNGROUP},
            {"cog", COG},
            {"cogs", COGS},
            {"bars", BARS},
            {"question", QUESTION},
            {"info", INFO},
            {"exclamation", EXCLAMATION},
            {"search", SEARCH},
            {"home", HOME},
            {"download", DOWNLOAD},
            {"upload", UPLOAD},
            {"sync", SYNC},
            {"play", PLAY},
            {"pause", PAUSE},
            {"stop", STOP}
        };

        auto it = iconMap.find(name);
        return it != iconMap.end() ? it->second : "";
    }
};

// -------------------------------------------------------

struct Image
{
    //std::shared_ptr<ImageResource> resource;
    uint32_t width = 0;
    uint32_t height = 0;
    //ImageUsageFlags usage;
    //Format format;
    //Layout::ImageLayout layout;
    //AspectFlags aspect;
    uint32_t layers = 1;
    uint32_t RID();
    ImTextureID ImGuiRID();
    ImTextureID ImGuiRID(uint32_t layer);
};

// -------------------------------------------------------

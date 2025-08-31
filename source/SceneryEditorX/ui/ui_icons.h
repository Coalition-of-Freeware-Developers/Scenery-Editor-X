/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ui_icons.h
* -------------------------------------------------------
* Created: 31/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <IconsFontAwesome5.h>

/// -------------------------------------------------------

namespace SceneryEditorX::UI
{
		
	struct Icons
	{
	    /// Core UI icons
	    static constexpr const char* ARROW_RIGHT = ICON_FA_ARROW_RIGHT;
	    static constexpr const char* ARROW_LEFT = ICON_FA_ARROW_LEFT;
	    static constexpr const char* ARROW_UP = ICON_FA_ARROW_UP;
	    static constexpr const char* ARROW_DOWN = ICON_FA_ARROW_DOWN;
	    static constexpr const char* PLUS = ICON_FA_PLUS;
	    static constexpr const char* MINUS = ICON_FA_MINUS;
	    static constexpr const char* CHECK = ICON_FA_CHECK;
	    static constexpr const char* TIMES = ICON_FA_TIMES;
	    static constexpr const char* TRASH = ICON_FA_TRASH;
	    static constexpr const char* SAVE = ICON_FA_SAVE;
	    static constexpr const char* FOLDER = ICON_FA_FOLDER;
	    static constexpr const char* FOLDER_OPEN = ICON_FA_FOLDER_OPEN;
	    static constexpr const char* FILE = ICON_FA_FILE;
	    static constexpr const char* FILE_ALT = ICON_FA_FILE_ALT;
	
	    /// Editor tools
	    static constexpr const char* PENCIL = ICON_FA_PENCIL_ALT;
	    static constexpr const char* EDIT = ICON_FA_EDIT;
	    static constexpr const char* MOVE = ICON_FA_ARROWS_ALT;
	    static constexpr const char* ROTATE = ICON_FA_SYNC;
	    static constexpr const char* SCALE = ICON_FA_EXPAND;
	    static constexpr const char* UNDO = ICON_FA_UNDO;
	    static constexpr const char* REDO = ICON_FA_REDO;
	    static constexpr const char* EYE = ICON_FA_EYE;
	    static constexpr const char* EYE_SLASH = ICON_FA_EYE_SLASH;
	    static constexpr const char* LOCK = ICON_FA_LOCK;
	    static constexpr const char* UNLOCK = ICON_FA_UNLOCK;
	
	    /// 3D visualization
	    static constexpr const char* CUBE = ICON_FA_CUBE;
	    static constexpr const char* CUBES = ICON_FA_CUBES;
	    static constexpr const char* MOUNTAIN = ICON_FA_MOUNTAIN;
	    static constexpr const char* MAP = ICON_FA_MAP;
	    static constexpr const char* COMPASS = ICON_FA_COMPASS;
	    static constexpr const char* LOCATION = ICON_FA_MAP_MARKER_ALT;
	    static constexpr const char* RULER = ICON_FA_RULER_COMBINED;
	    static constexpr const char* CROSSHAIRS = ICON_FA_CROSSHAIRS;
	    static constexpr const char* OBJECT_GROUP = ICON_FA_OBJECT_GROUP;
	    static constexpr const char* OBJECT_UNGROUP = ICON_FA_OBJECT_UNGROUP;
	
	    /// Interface elements
	    static constexpr const char* COG = ICON_FA_COG;
	    static constexpr const char* COGS = ICON_FA_COGS;
	    static constexpr const char* BARS = ICON_FA_BARS;
	    static constexpr const char* QUESTION = ICON_FA_QUESTION;
	    static constexpr const char* INFO = ICON_FA_INFO_CIRCLE;
	    static constexpr const char* EXCLAMATION = ICON_FA_EXCLAMATION_TRIANGLE;
	    static constexpr const char* SEARCH = ICON_FA_SEARCH;
	    static constexpr const char* HOME = ICON_FA_HOME;
	    static constexpr const char* DOWNLOAD = ICON_FA_DOWNLOAD;
	    static constexpr const char* UPLOAD = ICON_FA_UPLOAD;
	    static constexpr const char* SYNC = ICON_FA_SYNC_ALT;
	    static constexpr const char* PLAY = ICON_FA_PLAY;
	    static constexpr const char* PAUSE = ICON_FA_PAUSE;
	    static constexpr const char* STOP = ICON_FA_STOP;
	
	    /// Helper method to get font icon
	    static const char* GetIcon(const std::string& name)
	    {
	        /// Map common names to icon constants
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
	
	
}

/// -------------------------------------------------------

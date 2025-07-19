/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* FileDialog.h
* -------------------------------------------------------
* Created: 5/2/2025
* -------------------------------------------------------
*/
#pragma once

/// -------------------------------------------------------

namespace SceneryEditorX::IO
{
    class FileDialog
    {
    public:
        INTERNAL FileDialog* instance();

        GLOBAL void OpenFileDialog(char *fileName, char *filter, char *title);
		GLOBAL void SaveFileDialog(char *fileName, char *filter, char *title);
		GLOBAL void OpenFolderDialog(char *folderName, char *title);
		GLOBAL void SaveFolderDialog(char *folderName, char *title);


    };

}

/// -------------------------------------------------------

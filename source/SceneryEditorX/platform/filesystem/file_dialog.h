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
        static FileDialog* instance();

        static void OpenFileDialog(char *fileName, char *filter, char *title);
		static void SaveFileDialog(char *fileName, char *filter, char *title);
		static void OpenFolderDialog(char *folderName, char *title);
		static void SaveFolderDialog(char *folderName, char *title);


    };

}

/// -------------------------------------------------------

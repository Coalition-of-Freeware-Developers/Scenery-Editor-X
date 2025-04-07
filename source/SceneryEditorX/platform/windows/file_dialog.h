<<<<<<<< Updated upstream:source/SceneryEditorX/core/settings/windows/file_manager/file_dialog.h
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

namespace SceneryEditorX::IO
{
    class fileDialog
    {
    public:
        static fileDialog* instance();

    };

    inline fileDialog* fileDialog::instance()
    {
        static fileDialog instance;
        return &instance;
    }

    void OpenFileDialog(char *fileName, char *filter, char *title);
    void SaveFileDialog(char *fileName, char *filter, char *title);
    void OpenFolderDialog(char *folderName, char *title);
    void SaveFolderDialog(char *folderName, char *title);

}
========
/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* file_dialog.h
* -------------------------------------------------------
* Created: 5/2/2025
* -------------------------------------------------------
*/

#pragma once

namespace SceneryEditorX :: IO
{
    class fileDialog
    {
    public:
        static fileDialog* instance();

    };

    inline fileDialog* fileDialog::instance()
    {
        static fileDialog instance;
        return &instance;
    }

    void OpenFileDialog(char *fileName, char *filter, char *title);
    void SaveFileDialog(char *fileName, char *filter, char *title);
    void OpenFolderDialog(char *folderName, char *title);
    void SaveFolderDialog(char *folderName, char *title);

} // namespace SceneryEditorX::IO

// ---------------------------------------------------------


>>>>>>>> Stashed changes:source/SceneryEditorX/platform/windows/file_dialog.h

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

#ifndef FILEDIALOG_H
#define FILEDIALOG_H

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
#endif

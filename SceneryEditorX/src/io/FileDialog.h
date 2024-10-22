#pragma once
#include "../xeditorPCH.h"

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

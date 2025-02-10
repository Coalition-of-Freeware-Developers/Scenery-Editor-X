#pragma once

#if (defined(_WIN) || defined(WIN32) || defined(_WINDOWS))

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
#endif // _WIN || WIN32 || _WINDOWS

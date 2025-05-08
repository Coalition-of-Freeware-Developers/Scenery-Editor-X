/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* FileDialog.cpp
* -------------------------------------------------------
* Created: 5/2/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/platform/file_dialog.h>

// -------------------------------------------------------

namespace SceneryEditorX::IO
{
    /**
     * Opens a file dialog to select one or multiple files.
     *
     * @return A vector of selected file paths.
     */
    void FileDialog::OpenFileDialog(char *fileName, char *filter, char *title)
    {
        const auto selection = pfd::open_file("Select a file",
                                        ".",
                                        {"Scenery Editor-X Files", "*.edX *.edX-lib *.wed", "All Files", "*"},
                                        pfd::opt::multiselect).result();
    
        // Do something with selection
        for (const auto &filename : selection)
            std::cout << "Selected file: " << filename << "\n";
    }

    
    /**
     * Opens a file dialog to save a file.
     *
     * @return The selected file path.
     */
    void FileDialog::SaveFileDialog(char *fileName, char *filter, char *title)
    {
        const auto destination = pfd::save_file("Select a file",
                                          ".",
                                          {"Scenery Editor-X Files", "*.edX", "All Files", "*"},
                                          pfd::opt::force_overwrite).result();

        // Do something with destination
        std::cout << "Selected file: " << destination << "\n";
    }

    /**
     * Opens a folder dialog to select a folder.
     *
     * @return The selected folder path.
     */
    void FileDialog::OpenFolderDialog(char *folderName, char *title)
    {
        const auto selection = pfd::select_folder("Select a folder", ".").result();

        // Do something with selection
        std::cout << "Selected folder: " << selection << "\n";
    }

    /**
     * Opens a folder dialog to save a folder.
     *
     * @return The selected folder path.
     */
    void FileDialog::SaveFolderDialog(char *folderName, char *title)
    {
        const auto destination = pfd::select_folder("Select a folder", ".").result();

        // Do something with destination
        std::cout << "Selected folder: " << destination << "\n";
    }

} // namespace SceneryEditorX

// -------------------------------------------------------


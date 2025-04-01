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

#if (defined(_WIN) || defined(WIN32) || defined(_WINDOWS))

#include "FileDialog.h"

// -------------------------------------------------------

namespace SceneryEditorX::IO
{
    /**
        * Opens a file dialog to select one or multiple files.
        *
        * @return A vector of selected file paths.
        */
    inline void OpenFileDialog(char *fileName, char *filter, char *title)
    {
        auto selection = pfd::open_file("Select a file",
                                        ".",
                                        {"Scenery Editor-X Files", "*.edX *.edX-lib *.wed", "All Files", "*"},
                                        pfd::opt::multiselect)
                             .result();
    
        // Do something with selection
        for (auto const &filename : selection)
            std::cout << "Selected file: " << filename << "\n";
    }

    
    /**
    * Opens a file dialog to save a file.
    *
    * @return The selected file path.
    */
    inline void SaveFileDialog(char *fileName, char *filter, char *title)
    {
        auto destination = pfd::save_file("Select a file",
                                          ".",
                                          {"Scenery Editor-X Files", "*.edX", "All Files", "*"},
                                          pfd::opt::force_overwrite)
                               .result();

        // Do something with destination
        std::cout << "Selected file: " << destination << "\n";
    }

    /**
    * Opens a folder dialog to select a folder.
    *
    * @return The selected folder path.
    */
    inline void OpenFolderDialog(char *folderName, char *title)
    {
        auto selection = pfd::select_folder("Select a folder", ".").result();

        // Do something with selection
        std::cout << "Selected folder: " << selection << "\n";
    }

    /**
    * Opens a folder dialog to save a folder.
    *
    * @return The selected folder path.
    */
    inline void SaveFolderDialog(char *folderName, char *title)
    {
        auto destination = pfd::select_folder("Select a folder", ".").result();

        // Do something with destination
        std::cout << "Selected folder: " << destination << "\n";
    }
} // namespace SceneryEditorX

#endif // _WIN || WIN32 || _WINDOWS


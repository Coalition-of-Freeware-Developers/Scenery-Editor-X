/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * FileDialog.cpp
 * -------------------------------------------------------
 * Created: 5/2/2025
 * -------------------------------------------------------
 */
#include "file_dialog.h"

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

        ///< TODO: Write open code and do something with selection
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

        ///< TODO: Write open code and do something with selection
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

        ///< TODO: Write open code and do something with selection
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

        ///< TODO: Write open code and do something with selection
        std::cout << "Selected folder: " << destination << "\n";
    }

}

// -------------------------------------------------------


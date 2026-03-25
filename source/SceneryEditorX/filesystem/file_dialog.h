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
 * file_dialog.h
 * -------------------------------------------------------
 * Created: 5/2/2025
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------


class FileDialogItem;
namespace SceneryEditorX::IO
{

	/**
	 * @class FileDialog
	 * @brief The FileDialog class provides static methods for opening 
	 * and saving files and folders through native file dialogs.
	 */
	class FileDialog
	{
	public:
		static FileDialog* Instance();

		static void OpenFileDialog(char *fileName, char *filter, char *title);
		static void SaveFileDialog(char *fileName, char *filter, char *title);
		static void OpenFolderDialog(char *folderName, char *title);
		static void SaveFolderDialog(char *folderName, char *title);

	    static std::string OpenFile(const char* filter);
		static std::string SaveFile(const char* filter);

	    /**
		 * @brief Opens a file dialog for selecting a file.
		 * @param inFilters The file filters to apply in the dialog.
		 * @return The path of the selected file.
		 */
		static std::filesystem::path OpenFileDialog(const std::initializer_list<FileDialogItem> inFilters = {});

		/**
		 * @brief Opens a folder dialog for selecting a folder.
		 * @param initialFolder The initial folder to open in the dialog.
		 * @return The path of the selected folder.
		 */
		static std::filesystem::path OpenFolderDialog(const char *initialFolder = "");

		/**
		 * @brief Opens a file dialog for saving a file.
		 * @param inFilters The file filters to apply in the dialog.
		 * @return The path of the selected file.
		 */
		static std::filesystem::path SaveFileDialog(const std::initializer_list<FileDialogItem> inFilters = {});

		static std::vector<uint8_t> ReadFileBytes(const std::filesystem::path &path);
		static void WriteFileBytes(const std::filesystem::path &path, const std::vector<uint8_t> &content);
		static void WriteFile(const std::filesystem::path &path, const std::string &content);

	};

}

// -------------------------------------------------------

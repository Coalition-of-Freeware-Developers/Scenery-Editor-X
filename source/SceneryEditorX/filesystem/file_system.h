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
 * file_system.h
 * -------------------------------------------------------
 * Created: 24/03/2026
 * -------------------------------------------------------
 */
#pragma once
#ifdef CreateDirectory
#undef CreateDirectory
#undef DeleteFile
#undef MoveFile
#undef CopyFile
#endif

#include <filesystem>
#include <string>
#include <vector>
#include <SceneryEditorX/asset/asset_types.h>
#include <SceneryEditorX/core/memory/buffer.h>

// -------------------------------------------------------

namespace SceneryEditorX::IO
{

	/**
	 * @enum FileStatus
	 * @brief Represents the status of a file operation.
	 */
	enum class FileStatus : uint8_t
	{
		Success			= 0,
		Invalid			= 1,
		NotFound		= 2,
		Locked			= 3,
		AccessDenied	= 4,
		AlreadyExists	= 5,
		UnknownError	= 6
	};
	
	/**
	 * @class FileSystem
	 * @brief Provides file system operations such as creating, deleting, and querying files and directories.
	 */
	class FileSystem
	{
	public:
		/**
		 * @struct FileDialogItem
		 * @brief Represents an item in a file dialog, including its name and extension.
		 */
		struct FileDialogItem
		{
			const char *name; // Display name for the file type (e.g., "Text Files")
			const char *ext;  // File specification string (e.g., "*.txt") used for filtering files in the dialog
		};

		/**
		 * @brief Gets the current working directory.
		 * @return The current working directory as a std::filesystem::path.
		 */
		static std::filesystem::path GetWorkingDir();

		/**
		 * @brief Sets the current working directory to the specified path.
		 * @param path The path to set as the current working directory.
		 */
		static void SetWorkingDir(const std::filesystem::path &path);

		/**
		 * @brief Creates a directory at the specified path.
		 * @param directory The path of the directory to create.
		 * @return True if the directory was created successfully, false otherwise.
		 */
		static bool CreateDir(const std::filesystem::path &directory);

		/**
		 * @brief Creates a directory at the specified path.
		 * @param directory The path of the directory to create.
		 * @return True if the directory was created successfully, false otherwise.
		 */
		static bool CreateDir(const std::string &directory);

		/**
		 * @brief Checks if a directory exists at the specified path.
		 * @param directory The path of the directory to check.
		 * @return True if the directory exists, false otherwise.
		 */
		static bool DirExists(const std::filesystem::path &directory);

		/**
		 * @brief Checks if a directory exists at the specified path.
		 * @param directory The path of the directory to check.
		 * @return True if the directory exists, false otherwise.
		 */
		static bool DirExists(const std::string &directory);

		/**
		 * @brief Deletes a file at the specified path.
		 * @param filepath The path of the file to delete.
		 * @return True if the file was deleted successfully, false otherwise.
		 */
		static bool DeleteFile(const std::filesystem::path &filepath);

		/**
		 * @brief Moves a file from one location to another.
		 * @param filepath The path of the file to move.
		 * @param dest The destination path where the file should be moved.
		 * @return True if the file was moved successfully, false otherwise.
		 */
		static bool MoveFile(const std::filesystem::path &filepath, const std::filesystem::path &dest);

		/**
		 * @brief Copies a file from one location to another.
		 * @param filepath The path of the file to copy.
		 * @param dest The destination path where the file should be copied.
		 * @return True if the file was copied successfully, false otherwise.
		 */
		static bool CopyFile(const std::filesystem::path &filepath, const std::filesystem::path &dest);

		/**
		 * @brief Checks if a file exists at the specified path.
		 * @param filepath The path of the file to check.
		 * @return True if the file exists, false otherwise.
		 */
		static bool Exists(const std::string &filepath);

		/**
		 * @brief Checks if a file exists at the specified path.
		 * @param filepath The path of the file to check.
		 * @return True if the file exists, false otherwise.
		 */
		static bool Exists(const std::filesystem::path &filepath);

		/**
		 * @brief Checks if the specified path is a file.
		 * @param path The path to check.
		 * @return True if the path is a file, false otherwise.
		 */
		static bool IsFile(const std::string &path);

		/**
		 * @brief Tries to open a file at the specified path.
		 * @param filePath The path of the file to open.
		 * @return The status of the file operation.
		 */
		static FileStatus TryOpenFile(const std::filesystem::path &filePath);

		/**
		 * @brief Tries to open a file at the specified path and waits for a specified duration if the file is not immediately available.
		 * @param filepath The path of the file to open.
		 * @param waitMs The duration in milliseconds to wait for the file to become available.
		 * @return The status of the file operation.
		 */
		static FileStatus TryOpenFileAndWait(const std::filesystem::path &filepath, uint64_t waitMs);

		/**
		 * @brief Moves a file from one location to another.
		 * @param oldFilepath The path of the file to move.
		 * @param newFilepath The destination path where the file should be moved.
		 * @return True if the file was moved successfully, false otherwise.
		 */
		static bool Move(const std::filesystem::path &oldFilepath, const std::filesystem::path &newFilepath);

		/**
		 * @brief Copies a file from one location to another.
		 * @param oldFilepath The path of the file to copy.
		 * @param newFilepath The destination path where the file should be copied.
		 * @return True if the file was copied successfully, false otherwise.
		 */
		static bool Copy(const std::filesystem::path &oldFilepath, const std::filesystem::path &newFilepath);

		/**
		 * @brief Renames a file from one name to another.
		 * @param oldFilepath The path of the file to rename.
		 * @param newFilepath The new path for the file.
		 * @return True if the file was renamed successfully, false otherwise.
		 */
		static bool Rename(const std::filesystem::path &oldFilepath, const std::filesystem::path &newFilepath);

		/**
		 * @brief Renames the filename of a file while keeping its directory path unchanged.
		 * @param oldFilepath The path of the file to rename.
		 * @param newName The new name for the file.
		 * @return True if the file was renamed successfully, false otherwise.
		 */
		static bool RenameFilename(const std::filesystem::path &oldFilepath, const std::string &newName);

		/**
		 * @brief Checks if the specified path is a directory.
		 * @param filepath The path to check.
		 * @return True if the path is a directory, false otherwise.
		 */
		static bool IsDirectory(const std::filesystem::path &filepath);

		/**
		 * @brief Checks if the first file is newer than the second file.
		 * @param fileA The first file to compare.
		 * @param fileB The second file to compare.
		 * @return True if fileA is newer than fileB, false otherwise.
		 */
		static bool IsNewer(const std::filesystem::path &fileA, const std::filesystem::path &fileB);

		/**
		 * @brief Shows the specified file in the system's file explorer.
		 * @param path The path of the file to show.
		 * @return True if the file was successfully shown, false otherwise.
		 */
		static bool ShowFileInExplorer(const std::filesystem::path &path);

		/**
		 * @brief Opens the specified directory in the system's file explorer.
		 * @param path The path of the directory to open.
		 * @return True if the directory was successfully opened, false otherwise.
		 */
		static bool OpenDirectoryInExplorer(const std::filesystem::path &path);

		/**
		 * @brief Opens the specified file or directory with the default external application.
		 * @param path The path of the file or directory to open.
		 * @return True if the file or directory was successfully opened, false otherwise.
		 */
		static bool OpenExternally(const std::filesystem::path &path);

		/**
		 * @brief Writes the contents of a buffer to a file.
		 * @param filepath The path of the file to write to.
		 * @param buffer The buffer containing the data to write.
		 * @return True if the data was successfully written, false otherwise.
		 */
		static bool WriteBytes(const std::filesystem::path &filepath, const Memory::Buffer &buffer);

		/**
		 * @brief Reads the contents of a file into a buffer.
		 * @param filepath The path of the file to read.
		 * @return A buffer containing the file's contents.
		 */
		static Memory::Buffer ReadBytes(const std::filesystem::path& filepath);

		/**
		 * @brief Generates a unique file name based on the specified file path.
		 * @param filepath The path of the file to generate a unique name for.
		 * @return A unique file name based on the specified file path.
		 */
		static std::filesystem::path GetUniqueFileName(const std::filesystem::path &filepath);

		/**
		 * @brief Gets the last write time of the specified file.
		 * @param filepath The path of the file to get the last write time for.
		 * @return The last write time of the file.
		 */
		static uint64_t GetLastWriteTime(const std::filesystem::path &filepath);

		/**
		 * @brief Checks if the specified string is empty or contains only whitespace characters.
		 * @param var The string to check.
		 * @return True if the string is empty or contains only whitespace, false otherwise.
		 */
		static bool IsEmptyOrWhitespace(const std::string &var);

		/**
		 * @brief Checks if the specified string contains only alphanumeric characters.
		 * @param var The string to check.
		 * @return True if the string is alphanumeric, false otherwise.
		 */
		static bool IsAlphanumeric(const std::string &var);

		/**
		 * @brief Removes illegal characters from the specified string.
		 * @param text The string to process.
		 * @return A string with illegal characters removed.
		 */
		static std::string RemoveIllegalCharacters(const std::string &text);

		/**
		 * @brief Gets the substring before the specified expression.
		 * @param str The string to process.
		 * @param exp The expression to search for.
		 * @return The substring before the specified expression.
		 */
		static std::string GetStringBeforeExpression(const std::string &str, const std::string &exp);

		/**
		 * @brief Gets the substring after the specified expression.
		 * @param str The string to process.
		 * @param exp The expression to search for.
		 * @return The substring after the specified expression.
		 */
		static std::string GetStringAfterExpression(const std::string &str, const std::string &exp);

		/**
		 * @brief 
		 * @brief Gets the substring between the specified expressions.
		 * @param str The string to process.
		 * @param exp_a The starting expression.
		 * @param exp_b The ending expression.
		 * @return The substring between the specified expressions.
		 */
		static std::string GetStringBetweenExpressions(const std::string &str, const std::string &exp_a, const std::string &exp_b);

		/**
		 * @brief Converts the specified string to uppercase.
		 * @param lower The string to convert.
		 * @return The uppercase version of the string.
		 */
		static std::string ConvertToUppercase(const std::string &lower);

		/**
		 * @brief Replaces all occurrences of a specified substring with another substring.
		 * @param str The string to process.
		 * @param from The substring to replace.
		 * @param to The substring to replace with.
		 * @return The string with the specified replacements.
		 */
		static std::string ReplaceExpression(const std::string &str, const std::string &from, const std::string &to);

		/**
		 * @brief Converts the specified string to a wide string.
		 * @param str The string to convert.
		 * @return The wide string version of the string.
		 */
		static std::wstring StringToWstring(const std::string &str);


		/**
		 * @brief Gets the path to the persistent storage directory.
		 * @return The path to the persistent storage directory.
		 */
		static std::filesystem::path GetPersistentStoragePath();

		/**
		 * @brief Checks if the specified environment variable exists.
		 * @param key The name of the environment variable.
		 * @return true if the environment variable exists, false otherwise.
		 */
		static bool CheckEnvVariable(const std::string &key);

		/**
		 * @brief Sets the specified environment variable.
		 * @param key The name of the environment variable.
		 * @param value The value to set for the environment variable.
		 * @return true if the environment variable was set successfully, false otherwise.
		 */
		static bool SetEnvVariable(const std::string &key, const std::string &value);

		/**
		 * @brief Gets the value of the specified environment variable.
		 * @param key The name of the environment variable.
		 * @return The value of the environment variable if it exists, an empty string otherwise.
		 */
		static std::string GetEnvVariable(const std::string &key);

		/**
		 * @brief Gets the directory from the specified file path.
		 * @param path The file path to process.
		 * @return The directory from the specified file path.
		 */
		static std::string GetDirectoryFromFilePath(const std::string &path);

		/**
		 * @brief Gets the current working directory.
		 * @return The current working directory.
		 */
		static std::string GetWorkingDirectory();

		/**
		 * @brief Gets the root directory from the specified file path.
		 * @param path The file path to process.
		 * @return The root directory from the specified file path.
		 */
		static std::string GetRootDirectory(const std::string &path);

		/**
		 * @brief Gets the relative path from the specified file path.
		 * @param path The file path to process.
		 * @return The relative path from the specified file path.
		 */
		static std::string GetRelativePath(const std::string &path);

		/**
		 * @brief Gets the parent directory from the specified file path.
		 * @param path The file path to process.
		 * @return The parent directory from the specified file path.
		 */
		static std::string GetParentDirectory(const std::string &path);

		/**
		 * @brief Gets the file name without the extension from the specified file path.
		 * @param path The file path to process.
		 * @return The file name without the extension from the specified file path.
		 */
		static std::string GetFileNameWithoutExtensionFromFilePath(const std::string &path);

		/**
		 * @brief Gets the file name from the specified file path.
		 * @param path The file path to process.
		 * @return The file name from the specified file path.
		 */
		static std::string GetFileNameFromFilePath(const std::string &path);

		/**
		 * @brief Gets the list of files in the specified directory.
		 * @param path The directory path to process.
		 * @return A vector containing the file names in the specified directory.
		 */
		static std::vector<std::string> GetFilesInDirectory(const std::string &path);

		/**
		 * @brief Gets the size of the specified file.
		 * @param path The file path to process.
		 * @return The size of the specified file in bytes.
		 */
		static uint32_t GetFileSize(const std::filesystem::path &path);

		/**
		 * @brief Gets the number of files in the specified directory.
		 * @param path The directory path to process.
		 * @return The number of files in the specified directory.
		 */
		static uint32_t GetDirFileCount(const std::filesystem::path &path);

		/**
		 * @brief Gets the path to the user's profile directory.
		 * @return An optional containing the path to the user's profile directory if available, std::nullopt otherwise.
		 */
		static std::optional<std::filesystem::path> GetUserProfileDir();

		/**
		 * @brief Deletes the specified file or directory.
		 * @param path The path to the file or directory to delete.
		 * @return true if the file or directory was deleted successfully, false otherwise.
		 */
		static bool Delete(const std::string &path);

		/**
		 * @brief Checks if the specified executable is available in the system's PATH.
		 * @param executable The name of the executable to check.
		 * @return true if the executable is found in the PATH, false otherwise.
		 */
		static bool IsExecutableInPath(const std::string &executable);

		/**
		 * @brief Creates an archive at the specified path, including the specified paths.
		 * @param path The path where the archive will be created.
		 * @param includePaths The paths to include in the archive.
		 * @return true if the archive was created successfully, false otherwise.
		 */
		static bool CreateArchive(const std::string &path, const std::vector<std::string> &includePaths);

		/**
		 * @brief Opens the specified URL in the default web browser.
		 * @param url The URL to open.
		 */
		static void OpenUrl(const char *url);

		/**
		 * @brief Opens the specified URL in the default web browser.
		 * @param url The URL to open.
		 */
		static void OpenUrl(const std::string & url);

		/**
		 * @brief Checks if the file extension of the given path matches the specified asset type.
		 * @param path The file path to check.
		 * @param assetType The asset type to compare against.
		 * @return true if the file extension of 'path' maps to the provided asset type.
		 */
		static bool IsValidExtension(const std::string &path, SceneryEditorX::AssetType assetType);

		/**
		 * @brief Opens the specified file and writes the provided data to it.
		 * @return true if the file was written successfully.
		 * @return false if the file was not written successfully.
		 */
		static void GetAppData();
	};

}

// -------------------------------------------------------

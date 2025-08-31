/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* FileManager.hpp
* -------------------------------------------------------
* Created: 17/3/2025
* -------------------------------------------------------
*/
#pragma once

#include "SceneryEditorX/renderer/vulkan/vk_buffers.h"
//#include "SceneryEditorX/asset/asset_manager.h"
#ifdef CreateDirectory
#undef CreateDirectory
#undef DeleteFile
#undef MoveFile
#undef CopyFile
#endif
#include <functional>
#include <filesystem>
#include <string>
#include <vector>


/// -------------------------------------------------------

namespace SceneryEditorX::IO
{
    enum class FileStatus : uint8_t
    {
        Success = 0,
		Invalid,
        NotFound,
        Locked,
		AccessDenied,
		AlreadyExists,
        UnknownError
    };

	class FileManager
	{
	public:
		/**
		 * @brief Reads raw bytes from a file.
		 * 
		 * This function opens the specified file and reads its contents into a vector of bytes.
		 * 
		 * @param path
		 * @return A vector containing the raw bytes read from the file.
         */
        static std::vector<uint8_t> ReadRawBytes(const std::filesystem::path &path);

        /**
		 * @brief Reads shader code from a file.
		 * 
		 * This function opens the specified shader file and reads its contents into a vector of bytes.
		 * 
		 * @param filename The name of the shader file to read.
		 * @return A vector containing the raw bytes read from the shader file.
         */
        static std::vector<char> ReadShaders(const std::string &filename);

        /**
         * @brief Reads the raw bytes from a file.
         * @param filename The name of the file to read.
         * @return std::vector<char> A vector containing the raw bytes of the file.
         */
        static std::vector<char> ReadFile(const std::string &filename);

        /**
         * @brief Writes data to a file.
         *
         * This function opens the specified file and writes the provided data to it.
         * @return true if the file was written successfully.
         * @return false if the file was not written successfully.
         */
        static void GetAppData();

	};

    /// -------------------------------------------------------

	class FileDialogs
	{
	public:
		/// These return empty strings if cancelled
		static std::string OpenFile(const char* filter);
		static std::string SaveFile(const char* filter);

		/// -------------------------------------------------------

        static bool IsTexture(const std::filesystem::path &path);
        static bool IsModel(const std::filesystem::path &path);

        static std::vector<uint8_t> ReadFileBytes(const std::filesystem::path &path);
        static void WriteFileBytes(const std::filesystem::path &path, const std::vector<uint8_t> &content);
        static void WriteFile(const std::filesystem::path &path, const std::string &content);

		/// -------------------------------------------------------

	    /*
        uint64_t Import(const std::filesystem::path &path, AssetManager &assets);
        uint64_t ImportModel(const std::filesystem::path &path, const AssetManager &assets);

		uint64_t ImportGLTF(const std::filesystem::path& path, AssetManager& manager);
        uint64_t ImportOBJ(const std::filesystem::path& path, AssetManager& manager);
        uint64_t ImportFBX(const std::filesystem::path &path, const AssetManager &assets);
        uint64_t Import3DS(const std::filesystem::path &path, const AssetManager &assets);
        */

		/// -------------------------------------------------------

        //static void ImportTexture(const std::filesystem::path &path, Ref<TextureAsset> &t);
        //static uint64_t ImportTexture(const std::filesystem::path &path, AssetManager &assets);
        static void ReadTexture(const std::filesystem::path &path, std::vector<uint8_t> &data, int32_t &w, int32_t &h);

	};

	class FileSystem
    {
    public:
        struct FileDialogItem
        {
            const char *Name;
            const char *Spec;
        };

        static std::filesystem::path GetWorkingDir();
        static void SetWorkingDir(const std::filesystem::path &path);

        static bool CreateDir(const std::filesystem::path &directory);
        static bool CreateDir(const std::string &directory);
        static bool DirExists(const std::filesystem::path &directory);
        static bool DirExists(const std::string &directory);

        static bool DeleteFile(const std::filesystem::path &filepath);
        static bool MoveFile(const std::filesystem::path &filepath, const std::filesystem::path &dest);
        static bool CopyFile(const std::filesystem::path &filepath, const std::filesystem::path &dest);

        static bool Exists(const std::string &filepath);
        static bool Exists(const std::filesystem::path &filepath);

        static FileStatus TryOpenFile(const std::filesystem::path &filePath);
        static FileStatus TryOpenFileAndWait(const std::filesystem::path &filepath, uint64_t waitms);
        static bool Move(const std::filesystem::path &oldFilepath, const std::filesystem::path &newFilepath);
        static bool Copy(const std::filesystem::path &oldFilepath, const std::filesystem::path &newFilepath);
        static bool Rename(const std::filesystem::path &oldFilepath, const std::filesystem::path &newFilepath);
        static bool RenameFilename(const std::filesystem::path &oldFilepath, const std::string &newName);

        static bool IsDirectory(const std::filesystem::path &filepath);
        static bool IsNewer(const std::filesystem::path &fileA, const std::filesystem::path &fileB);
        static bool ShowFileInExplorer(const std::filesystem::path &path);
        static bool OpenDirectoryInExplorer(const std::filesystem::path &path);
        static bool OpenExternally(const std::filesystem::path &path);

        static bool WriteBytes(const std::filesystem::path &filepath, const BufferResource &buffer);
        static Buffer ReadBytes(const std::filesystem::path& filepath);

        static std::filesystem::path GetUniqueFileName(const std::filesystem::path &filepath);
        static uint64_t GetLastWriteTime(const std::filesystem::path &filepath);

        static std::filesystem::path OpenFileDialog(const std::initializer_list<FileDialogItem> inFilters = {});
        static std::filesystem::path OpenFolderDialog(const char *initialFolder = "");
        static std::filesystem::path SaveFileDialog(const std::initializer_list<FileDialogItem> inFilters = {});

        static std::filesystem::path GetPersistentStoragePath();

        static bool CheckEnvVariable(const std::string &key);
        static bool SetEnvVariable(const std::string &key, const std::string &value);
        static std::string GetEnvVariable(const std::string &key);
    };

}

/// -------------------------------------------------------

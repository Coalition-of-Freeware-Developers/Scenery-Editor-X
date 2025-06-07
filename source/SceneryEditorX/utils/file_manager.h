/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* file_manager.h
* -------------------------------------------------------
* Created: 27/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <GraphicsEngine/buffers/buffer_data.h>
#ifdef CreateDirectory
#undef CreateDirectory
#undef DeleteFile
#undef MoveFile
#undef CopyFile
#endif

#include <functional>
#include <filesystem>

/// -------------------------------------------------------

namespace SceneryEditorX
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

	class FileSystem
	{
	public:

        struct FileDialogItem
        {
            const char *Name;
            const char *Spec;
        };

        GLOBAL std::filesystem::path GetWorkingDir();
        GLOBAL void SetWorkingDir(std::filesystem::path path);

		GLOBAL bool CreateDir(const std::filesystem::path &directory);
		GLOBAL bool CreateDir(const std::string &directory);
        GLOBAL bool DirExists(const std::filesystem::path &directory);
        GLOBAL bool DirExists(const std::string &directory);

		GLOBAL bool DeleteFile(const std::filesystem::path &filePath);
        GLOBAL bool MoveFile(const std::filesystem::path &source, const std::filesystem::path &destination);
        GLOBAL bool CopyFile(const std::filesystem::path &source, const std::filesystem::path &destination);

		GLOBAL FileStatus TryOpenFile(const std::filesystem::path &filePath);

	    GLOBAL bool Move(const std::filesystem::path &oldFilepath, const std::filesystem::path &newFilepath);
        GLOBAL bool Copy(const std::filesystem::path &oldFilepath, const std::filesystem::path &newFilepath);
        GLOBAL bool Rename(const std::filesystem::path &oldFilepath, const std::filesystem::path &newFilepath);
        GLOBAL bool RenameFilename(const std::filesystem::path &oldFilepath, const std::string &newName);

	    GLOBAL bool ShowFileInExplorer(const std::filesystem::path &path);
        GLOBAL bool OpenDirectoryInExplorer(const std::filesystem::path &path);
        GLOBAL bool OpenExternally(const std::filesystem::path &path);

	    GLOBAL bool WriteBytes(const std::filesystem::path &filepath, const BufferResource &buffer);
        GLOBAL Buffer ReadBytes(const std::filesystem::path &filepath);

        GLOBAL std::filesystem::path GetUniqueFileName(const std::filesystem::path &filepath);
        GLOBAL uint64_t GetLastWriteTime(const std::filesystem::path &filepath);

	    GLOBAL std::filesystem::path OpenFileDialog(const std::initializer_list<FileDialogItem> inFilters = {});
        GLOBAL std::filesystem::path OpenFolderDialog(const char *initialFolder = "");
        GLOBAL std::filesystem::path SaveFileDialog(const std::initializer_list<FileDialogItem> inFilters = {});

        GLOBAL std::filesystem::path GetPersistentStoragePath();

	    static bool CheckEnvVariable(const std::string &key);
        static bool SetEnvVariable(const std::string &key, const std::string &value);
        static std::string GetEnvVariable(const std::string &key);
	};
}

/// -------------------------------------------------------

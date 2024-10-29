#include "../src/xpeditorpch.h"

namespace SceneryEditorX
{
    int FileLock(int nFileDescriptor, bool lockBool)
    {
        HANDLE hFile = (HANDLE)_get_osfhandle(nFileDescriptor);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            spdlog::error("Invalid file handle");
            return -1;

        }

        DWORD dwFlags = lockBool ? LOCKFILE_EXCLUSIVE_LOCK : 0;
        OVERLAPPED overlapped = {0};

        if (lockBool)
        {
            // Lock the file
            if (!LockFileEx(hFile, dwFlags, 0, MAXDWORD, MAXDWORD, &overlapped))
            {
                spdlog::error("Failed to lock the file");
                return -1;
            }
        }
        else
        {
            // Unlock the file
            if (!UnlockFileEx(hFile, 0, MAXDWORD, MAXDWORD, &overlapped))
            {
                spdlog::error("Failed to unlock the file");
                return -1;
            }
        }

        return 0;
    }

    bool lockCheck(int nFileDescriptor)
    {
        HANDLE hFile = (HANDLE)_get_osfhandle(nFileDescriptor);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            spdlog::error("Invalid file handle");
            return false;
        }

        OVERLAPPED overlapped = {0};
        DWORD dwFlags = LOCKFILE_FAIL_IMMEDIATELY | LOCKFILE_EXCLUSIVE_LOCK;

        // Attempt to lock the file
        if (!LockFileEx(hFile, dwFlags, 0, MAXDWORD, MAXDWORD, &overlapped))
        {
            DWORD dwError = GetLastError();
            if (dwError == ERROR_LOCK_VIOLATION)
            {
                spdlog::info("File is locked");
                return true;
            }
            else
            {
                spdlog::error("Failed to check file lock status, error code: {}", dwError);
            }
        }
        else
        {
            // Unlock the file immediately if it was successfully locked
            UnlockFileEx(hFile, 0, MAXDWORD, MAXDWORD, &overlapped);
        }

        return false;
    }
}

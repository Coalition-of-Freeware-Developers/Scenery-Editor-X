/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* LockFile.cpp
* -------------------------------------------------------
* Created: 5/2/2025
* -------------------------------------------------------
*/

// -----------------------------------------------------------

namespace SceneryEditorX::IO
{
    /**
     * @brief Locks or unlocks a file based on the provided file descriptor and lock flag.
     * 
     * @param nFileDescriptor The file descriptor of the file to be locked or unlocked.
     * @param lockBool A boolean flag indicating whether to lock (true) or unlock (false) the file.
     * @return int Returns 0 on success, -1 on failure.
     */
    static int FileLock(int nFileDescriptor, bool lockBool)
    {
        HANDLE hFile = (HANDLE)_get_osfhandle(nFileDescriptor);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            SEDX_CORE_ERROR("Invalid file handle");
            return -1;

        }

        DWORD dwFlags = lockBool ? LOCKFILE_EXCLUSIVE_LOCK : 0;
        OVERLAPPED overlapped = {};

        if (lockBool)
        {
            // Lock the file
            if (!LockFileEx(hFile, dwFlags, 0, MAXDWORD, MAXDWORD, &overlapped))
            {
                SEDX_CORE_ERROR("Failed to lock the file");
                return -1;
            }
        }
        else
        {
            // Unlock the file
            if (!UnlockFileEx(hFile, 0, MAXDWORD, MAXDWORD, &overlapped))
            {
                SEDX_CORE_ERROR("Failed to unlock the file");
                return -1;
            }
        }

        return 0;
    }

    /**
     * @brief Checks if the file is currently locked.
     * 
     * @param nFileDescriptor The file descriptor of the file to check.
     * @return bool Returns true if the file is locked, false otherwise.
     */
    bool lockCheck(int nFileDescriptor)
    {
        HANDLE hFile = reinterpret_cast<HANDLE>(_get_osfhandle(nFileDescriptor));
        if (hFile == INVALID_HANDLE_VALUE)
        {
            SEDX_CORE_ERROR("Invalid file handle");
            return false;
        }

        OVERLAPPED overlapped = {};
        DWORD dwFlags = LOCKFILE_FAIL_IMMEDIATELY | LOCKFILE_EXCLUSIVE_LOCK;

        // Attempt to lock the file
        if (!LockFileEx(hFile, dwFlags, 0, MAXDWORD, MAXDWORD, &overlapped))
        {
            DWORD dwError = GetLastError();
            if (dwError == ERROR_LOCK_VIOLATION)
            {
                SEDX_CORE_WARN("File is locked");
                return true;
            }
            else
            {
                SEDX_CORE_ERROR("Failed to check file lock status, error code: {}", dwError);
            }
        }
        else
        {
            // Unlock the file immediately if it was successfully locked
            UnlockFileEx(hFile, 0, MAXDWORD, MAXDWORD, &overlapped);
        }

        return false;
    }
} // namespace SceneryEditorX::IO

// -----------------------------------------------------------

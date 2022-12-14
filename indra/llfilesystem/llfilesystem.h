/**
 * @file filesystem.h
 * @brief Simulate local file system operations.
 * @Note The initial implementation does actually use standard C++
 *       file operations but eventually, there will be another
 *       layer that caches and manages file meta data too.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_FILESYSTEM_H
#define LL_FILESYSTEM_H

#include "lluuid.h"
#include "llassettype.h"
#include "lldiskcache.h"

#include <boost/filesystem.hpp>

class LLFileSystem
{
public:
    LLFileSystem(const LLUUID& file_id, const LLAssetType::EType file_type, S32 mode = LLFileSystem::READ);
    ~LLFileSystem() = default;

    bool open();
    void close();

    bool is_open() { return mFile; }

    BOOL read(U8* buffer, S32 bytes);
    S32  getLastBytesRead() const;
    BOOL eof();

    BOOL write(const U8* buffer, S32 bytes);
    BOOL seek(S32 offset, S32 origin = -1);
    S32  tell() const;

    S32        getSize() const;
    S32        getMaxSize();
    BOOL       rename(const LLUUID& new_id, const LLAssetType::EType new_type);
    BOOL       remove();
    BOOL       exists() const;

    /**
     * Update the "last write time" of a file to "now". This must be called whenever a
     * file in the cache is read (not written) so that the last time the file was
     * accessed is up to date (This is used in the mechanism for purging the cache)
     */
    void updateFileAccessTime();

    static bool getExists(const LLUUID& file_id, const LLAssetType::EType file_type);
    static bool removeFile(const LLUUID& file_id, const LLAssetType::EType file_type);
    static bool renameFile(const LLUUID& old_file_id, const LLAssetType::EType old_file_type,
        const LLUUID& new_file_id, const LLAssetType::EType new_file_type);

public:
    enum
    {
        READ = 0x00000001,
        WRITE = 0x00000002,
        READ_WRITE = 0x00000003,  // LLFileSystem::READ & LLFileSystem::WRITE
        APPEND = 0x00000006,  // 0x00000004 & LLFileSystem::WRITE
    };

protected:
    boost::filesystem::path mFilePath;
    LLUniqueFile mFile;
    LLUUID  mFileID;
    LLAssetType::EType mFileType;
    S32     mPosition;
    S32     mMode;
    S32     mBytesRead;
};

#endif  // LL_FILESYSTEM_H

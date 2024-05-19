#pragma once

#include <Disks/IDisk.h>
#include <Disks/ObjectStorages/IMetadataStorage.h>
#include <IO/ReadBufferFromFileBase.h>
#include <IO/WriteBufferFromFile.h>

namespace DB
{

class DiskOverlay : public IDisk
{
public:
    DiskOverlay(const String & name_, DiskPtr disk_base_, DiskPtr disk_diff_, MetadataStoragePtr metadata_, MetadataStoragePtr tracked_metadata_);
    DiskOverlay(const String & name_, const Poco::Util::AbstractConfiguration & config_, const String & config_prefix_, const DisksMap & map_);

    const String & getPath() const override;

    ReservationPtr reserve(UInt64 bytes) override;

    std::optional<UInt64> getTotalSpace() const override;
    std::optional<UInt64> getAvailableSpace() const override;
    std::optional<UInt64> getUnreservedSpace() const override;

    UInt64 getKeepingFreeSpace() const override;

    bool exists(const String & path) const override;

    bool isFile(const String & path) const override;

    bool isDirectory(const String & path) const override;

    size_t getFileSize(const String & path) const override;

    void createDirectory(const String & path) override;

    void createDirectories(const String & path) override;

    void clearDirectory(const String & path) override;

    void moveDirectory(const String & from_path, const String & to_path) override;

    DirectoryIteratorPtr iterateDirectory(const String & path) const override;

    void createFile(const String & path) override;

    void moveFile(const String & from_path, const String & to_path) override;

    void replaceFile(const String & from_path, const String & to_path) override;

    void listFiles(const String & path, std::vector<String> & file_names) const override;

    std::unique_ptr<ReadBufferFromFileBase> readFile(
        const String & path,
        const ReadSettings & settings,
        std::optional<size_t> read_hint,
        std::optional<size_t> file_size) const override;

    std::unique_ptr<WriteBufferFromFileBase> writeFile(
        const String & path,
        size_t buf_size,
        WriteMode mode,
        const WriteSettings & settings) override;

    Strings getBlobPath(const String &  /*path*/) const override;
    void writeFileUsingBlobWritingFunction(const String &  /*path*/, WriteMode  /*mode*/, WriteBlobFunction &&  /*write_blob_function*/) override;

    void removeFile(const String & path) override;
    void removeFileIfExists(const String & path) override;
    void removeDirectory(const String & path) override;
    void removeRecursive(const String & path) override;

    void setLastModified(const String &  path, const Poco::Timestamp &  timestamp) override;

    Poco::Timestamp getLastModified(const String &  path) const override;

    time_t getLastChanged(const String &  path) const override;

    void setReadOnly(const String & path) override;

    void createHardLink(const String &  src_path, const String &  dst_path) override;

    DataSourceDescription getDataSourceDescription() const override;

    bool supportParallelWrite() const override;

    /// Involves network interaction.
    bool isRemote() const override;

    /// Whether this disk support zero-copy replication.
    /// Overrode in remote fs disks.
    bool supportZeroCopyReplication() const override { return false; }

private:
    DiskPtr disk_base, disk_diff;
    MetadataStoragePtr metadata, tracked_metadata;

    // A tracked file is a file that exists on the diff disk (possibly under another name)
    // If a file is tracked, we don't need to list it from the base disk in calls to file listing functions
public:
    bool isTracked(const String& path) const;
private:
    void setTracked(const String& path);

    // When a file or directory needs to be created on disk_diff, we might be missing some parent
    // directories that are present on disk_base
    void ensureHaveDirectories(const String& path);

    // Same as above, but also create the file itself
    void ensureHaveFile(const String& path);

    // Get path to file in base disk
    std::optional<String> basePath(const String& path) const;
};

/**
 * This read buffer wraps around two read buffers, transparently concatenating them.
 */
class ReadBufferFromOverlayDisk : public ReadBufferFromFileBase
{
public:
    ReadBufferFromOverlayDisk(
        size_t buffer_size_,
        std::unique_ptr<ReadBufferFromFileBase> base_,
        std::unique_ptr<ReadBufferFromFileBase> diff_);

    off_t seek(off_t off, int whence) override;

    off_t getPosition() override;

    std::string getFileName() const override { return diff->getFileName(); }

    size_t getFileSize() override { return base->getFileSize() + diff->getFileSize(); }

private:
    bool nextImpl() override;

    std::unique_ptr<ReadBufferFromFileBase> base, diff;
    bool done_base = false, done = false;
    size_t base_size, diff_size;

};

};

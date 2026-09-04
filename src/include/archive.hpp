#pragma once

#include <archive.h>
#include <archive_entry.h>
#include <expected>
#include <filesystem>

namespace fs = std::filesystem;

struct ArchiveError : std::runtime_error {
    ArchiveError() : std::runtime_error("archive error") {
    }
};

class EntryView {
    struct archive_entry *entry_;

  public:
    EntryView(struct archive_entry *entry);

    bool is_regular_file();
    const char *pathname();
    bool size_is_set();
    la_int64_t size();
};

class ArchiveReader {
    struct ArchiveDeleter {
        void operator()(struct archive *archive) const noexcept {
            archive_read_free(archive);
        }
    };
    using Archive = std::unique_ptr<struct archive, ArchiveDeleter>;

    Archive archive_;

    ArchiveReader(Archive &&archive);

  public:
    [[nodiscard]] static std::expected<ArchiveReader, std::string>
    open(const char *filename);

    [[nodiscard]] std::optional<EntryView> next();

    [[nodiscard]] std::expected<size_t, std::string>
    read_data(void *data, size_t size);

    void close();
};

class ArchiveWriter {
    struct ArchiveDeleter {
        void operator()(struct archive *archive) const noexcept {
            archive_write_free(archive);
        }
    };

    std::unique_ptr<struct archive, ArchiveDeleter> archive_;

    void write_header(const char *filename, la_int64_t size);

  public:
    explicit ArchiveWriter(const char *filename);

    void add_file(const char *filename, const std::string &file_contents);
    void add_file(const char *filename, const fs::path &path);

    void set_compression_store();
    void set_compression_deflate();

    void close();
};

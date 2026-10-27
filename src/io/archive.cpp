#include <archive.h>
#include <archive_entry.h>
#include <expected>
#include <fstream>
#include <new>

#include "include/archive.hpp"

EntryView::EntryView(struct archive_entry *entry) : entry_(entry) {
}

bool EntryView::is_regular_file() {
    return archive_entry_filetype(entry_) == AE_IFREG;
}

auto EntryView::pathname() -> const char * {
    return archive_entry_pathname(entry_);
}

bool EntryView::size_is_set() {
    return static_cast<bool>(archive_entry_size_is_set(entry_));
}

la_int64_t EntryView::size() {
    return archive_entry_size(entry_);
}

ArchiveReader::ArchiveReader(Archive &&other) : archive_(std::move(other)) {
}

std::expected<ArchiveReader, std::string>
ArchiveReader::open(const char *filename) {
    auto archive = Archive{archive_read_new()};
    if (!archive) {
        throw std::bad_alloc();
    }

    archive_read_support_filter_all(archive.get());
    archive_read_support_format_all(archive.get());
    auto archive_open
        = archive_read_open_filename(archive.get(), filename, 10240);
    if (archive_open != ARCHIVE_OK) {
        const auto *error = archive_error_string(archive.get());
        return std::unexpected(error ? error : "");
    }
    return ArchiveReader(std::move(archive));
}

std::optional<EntryView> ArchiveReader::next() {
    struct archive_entry *entry;
    if (archive_read_next_header(archive_.get(), &entry) == ARCHIVE_OK) {
        return EntryView(entry);
    }
    return std::nullopt;
}

std::expected<size_t, std::string>
ArchiveReader::read_data(void *data, size_t size) {
    auto bytes_read = archive_read_data(archive_.get(), data, size);
    if (bytes_read < 0) {
        auto *error = archive_error_string(archive_.get());
        std::string message = error ? error : "";
        return std::unexpected(message);
    }
    return static_cast<size_t>(bytes_read);
}

void ArchiveReader::close() {
    if (archive_read_close(archive_.get()) == ARCHIVE_FATAL) {
        throw ArchiveError();
    }
}

struct EntryDeleter {
    void operator()(struct archive_entry *entry) const noexcept {
        archive_entry_free(entry);
    }
};
using Entry = std::unique_ptr<struct archive_entry, EntryDeleter>;

void ArchiveWriter::write_header(const char *filename, la_int64_t size) {
    auto entry = Entry{archive_entry_new()};
    archive_entry_set_pathname(entry.get(), filename);
    archive_entry_set_size(entry.get(), size);
    archive_entry_set_filetype(entry.get(), AE_IFREG);
    archive_entry_set_perm(entry.get(), 0644);
    if (archive_write_header(archive_.get(), entry.get()) == ARCHIVE_FATAL) {
        throw ArchiveError();
    }
}

ArchiveWriter::ArchiveWriter(const char *filename)
    : archive_(archive_write_new()) {
    if (!archive_) {
        throw std::bad_alloc();
    }
    archive_write_set_format_zip(archive_.get());
    if (archive_write_open_filename(archive_.get(), filename) != ARCHIVE_OK) {
        const auto *error = archive_error_string(archive_.get());
        auto message = std::string(error ? error : "");
        throw std::runtime_error(message);
    }
}

void ArchiveWriter::set_compression_store() {
    archive_write_zip_set_compression_store(archive_.get());
}

void ArchiveWriter::set_compression_deflate() {
    archive_write_zip_set_compression_deflate(archive_.get());
}

void ArchiveWriter::add_file(
    const char *filename, const std::string &file_contents
) {
    write_header(filename, static_cast<la_int64_t>(file_contents.size()));
    auto written = archive_write_data(
        archive_.get(), file_contents.data(), file_contents.size()
    );
    if (written == ARCHIVE_FATAL) {
        throw ArchiveError();
    }
}

void ArchiveWriter::add_file(const char *filename, const fs::path &path) {
    write_header(filename, static_cast<la_int64_t>(fs::file_size(path)));
    auto stream = std::ifstream(path, std::ios::binary);
    auto buffer = std::array<char, 8192>{};
    while (stream.good()) {
        stream.read(buffer.data(), sizeof(buffer));
        archive_write_data(
            archive_.get(), buffer.data(), static_cast<size_t>(stream.gcount())
        );
    }
}

void ArchiveWriter::close() {
    if (archive_write_close(archive_.get()) == ARCHIVE_FATAL) {
        throw ArchiveError();
    }
}

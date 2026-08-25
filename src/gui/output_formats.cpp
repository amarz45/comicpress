#include "include/output_formats.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

static std::string create_epub_container_xml() {
    auto out = QByteArray{};
    auto buf = QBuffer(&out);
    buf.open(QIODevice::WriteOnly);

    auto w = QXmlStreamWriter(&buf);
    w.setAutoFormatting(true);

    w.writeStartDocument();

    w.writeStartElement("container");
    w.writeAttribute("version", "1.0");
    w.writeAttribute(
        "xmlns", "urn:oasis:names:tc:opendocument:xmlns:container"
    );
    w.writeStartElement("rootfiles");
    w.writeEmptyElement("rootfile");
    w.writeAttribute("full-path", "OEBPS/content.opf");
    w.writeAttribute("media-type", "application/oebps-package+xml");
    w.writeEndElement(); // rootfiles
    w.writeEndElement(); // container

    w.writeEndDocument();
    return out.toStdString();
}

// Constructing a QCollator builds locale tables, so keep one around rather than
// paying for it on every comparison. Pinned to the en_US locale so page order
// doesn’t shift with the user’s system locale.
static QCollator &filename_collator() {
    static auto collator = [] {
        auto collator
            = QCollator(QLocale(QLocale::English, QLocale::UnitedStates));
        collator.setNumericMode(true);
        collator.setCaseSensitivity(Qt::CaseInsensitive);
        return collator;
    }();
    return collator;
}

static char to_lower_ascii(char c) {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c - 'A' + 'a');
    }
    else {
        return c;
    }
}

static std::string image_media_type(const fs::path &path) {
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), to_lower_ascii);

    if (ext == ".png") {
        return "image/png";
    }
    if (ext == ".jpg" || ext == ".jpeg") {
        return "image/jpeg";
    }
    if (ext == ".webp") {
        return "image/webp";
    }
    if (ext == ".avif") {
        return "image/avif";
    }
    if (ext == ".jxl") {
        return "image/jxl";
    }
    return {};
}

static std::string create_epub_nav_xhtml(QAnyStringView first_page_path) {
    auto out = QByteArray{};
    auto buf = QBuffer(&out);
    buf.open(QIODevice::WriteOnly);

    auto w = QXmlStreamWriter(&buf);
    w.setAutoFormatting(true);

    w.writeStartDocument();
    w.writeDTD("<!DOCTYPE html>");

    w.writeStartElement("html");
    w.writeAttribute("xmlns", "http://www.w3.org/1999/xhtml");
    w.writeAttribute("xmlns:epub", "http://www.idpf.org/2007/ops");
    w.writeAttribute("xml:lang", "en");
    w.writeAttribute("lang", "en");

    w.writeStartElement("head");
    w.writeEmptyElement("meta");
    w.writeAttribute("charset", "utf-8");
    w.writeTextElement("title", "Contents");
    w.writeEndElement(); // head

    w.writeStartElement("body");

    w.writeStartElement("nav");
    w.writeAttribute("epub:type", "toc");
    w.writeAttribute("id", "toc");
    w.writeTextElement("h1", "Contents");
    w.writeStartElement("ol");
    w.writeStartElement("li");
    w.writeStartElement("a");
    w.writeAttribute("href", first_page_path);
    w.writeCharacters("Start");
    w.writeEndElement(); // a
    w.writeEndElement(); // li
    w.writeEndElement(); // ol
    w.writeEndElement(); // nav

    w.writeStartElement("nav");
    w.writeAttribute("epub:type", "landmarks");
    w.writeAttribute("hidden", "hidden");
    w.writeStartElement("ol");
    w.writeStartElement("li");
    w.writeStartElement("a");
    w.writeAttribute("epub:type", "bodymatter");
    w.writeAttribute("href", first_page_path);
    w.writeCharacters("Start");
    w.writeEndElement(); // a
    w.writeEndElement(); // li
    w.writeEndElement(); // ol
    w.writeEndElement(); // nav

    w.writeEndElement(); // body

    w.writeEndElement(); // html

    w.writeEndDocument();
    return out.toStdString();
}

static void write_epub_content_opf_beginning(
    QXmlStreamWriter &w,
    QAnyStringView book_title,
    QAnyStringView book_uuid,
    QAnyStringView modified_time
) {
    w.setAutoFormatting(true);

    w.writeStartDocument();

    w.writeStartElement("package");
    w.writeAttribute("xmlns", "http://www.idpf.org/2007/opf");
    w.writeAttribute("version", "3.0");
    w.writeAttribute("unique-identifier", "pub-id");

    w.writeStartElement("metadata");
    w.writeAttribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");

    w.writeStartElement("dc:identifier");
    w.writeAttribute("id", "pub-id");
    w.writeCharacters(book_uuid);
    w.writeEndElement(); // dc:identifier

    w.writeTextElement("dc:title", book_title);
    w.writeTextElement("dc:language", "en");

    w.writeStartElement("meta");
    w.writeAttribute("property", "dcterms:modified");
    w.writeCharacters(modified_time);
    w.writeEndElement(); // meta
    w.writeStartElement("meta");
    w.writeAttribute("property", "rendition:layout");
    w.writeCharacters("pre-paginated");
    w.writeEndElement(); // meta
    w.writeStartElement("meta");
    w.writeAttribute("property", "rendition:spread");
    w.writeCharacters("auto");
    w.writeEndElement(); // meta

    w.writeEndElement(); // metadata

    w.writeStartElement("manifest");

    w.writeEmptyElement("item");
    w.writeAttribute("id", "nav");
    w.writeAttribute("href", "nav.xhtml");
    w.writeAttribute("media-type", "application/xhtml+xml");
    w.writeAttribute("properties", "nav");

    w.writeEmptyElement("item");
    w.writeAttribute("id", "css");
    w.writeAttribute("href", "style.css");
    w.writeAttribute("media-type", "text/css");
}

static void write_manifest_item(
    QXmlStreamWriter &writer,
    QAnyStringView id,
    QAnyStringView href,
    QAnyStringView media_type
) {
    writer.writeEmptyElement("item");
    writer.writeAttribute("id", id);
    writer.writeAttribute("href", href);
    writer.writeAttribute("media-type", media_type);
};

static std::string create_epub_page_xhtml(
    int page_num, const fs::path &image_path, int width, int height
) {
    auto out = QByteArray{};
    auto buf = QBuffer(&out);
    buf.open(QIODevice::WriteOnly);

    auto w = QXmlStreamWriter(&buf);
    w.setAutoFormatting(true);

    w.writeStartDocument();
    w.writeDTD("<!DOCTYPE html>");

    w.writeStartElement("html");
    w.writeAttribute("xmlns", "http://www.w3.org/1999/xhtml");
    w.writeAttribute("xmlns:epub", "http://www.idpf.org/2007/ops");

    w.writeStartElement("head");
    w.writeEmptyElement("meta");
    w.writeAttribute("charset", "utf-8");
    w.writeEmptyElement("meta");
    w.writeAttribute("name", "viewport");
    w.writeAttribute(
        "content",
        "width=" + std::to_string(width) + ", height=" + std::to_string(height)
    );
    w.writeTextElement("title", "Page " + std::to_string(page_num));
    w.writeEmptyElement("link");
    w.writeAttribute("rel", "stylesheet");
    w.writeAttribute("type", "text/css");
    w.writeAttribute("href", "../style.css");
    w.writeEndElement(); // head

    w.writeStartElement("body");
    w.writeStartElement("div");
    w.writeAttribute("class", "page");
    w.writeEmptyElement("img");
    w.writeAttribute("src", "../images/" + image_path.string());
    w.writeAttribute("alt", "Page " + std::to_string(page_num));
    w.writeEndElement(); // div
    w.writeEndElement(); // body

    w.writeEndElement(); // html

    w.writeEndDocument();
    return out.toStdString();
}

static std::string create_epub_mimetype() {
    return "application/epub+zip";
}

static std::string create_epub_style_css() {
    return "@page { margin: 0; }\n"
           "html, body { margin: 0; padding: 0; height: 100%; }\n"
           "body { background-color: #000000; }\n"
           "div.page { width: 100%; height: 100%; margin: 0; padding: 0; }\n"
           "img { width: 100%; height: 100%; margin: 0; padding: 0; display: "
           "block; }\n";
}

struct EpubImage {
    fs::path path;
    std::string media_type;
};

// Returns paths relative to `dir`, in page order. Recursive because
// `output_base_name` carries the source archive’s directory structure, so pages
// can sit in subdirectories rather than flat in the `image_dir`.
static std::vector<EpubImage> collect_epub_images(const fs::path &dir) {
    auto image_paths = std::vector<EpubImage>{};
    for (const auto &entry : fs::recursive_directory_iterator(dir)) {
        auto media_type = image_media_type(entry.path());
        if (media_type.empty()) {
            continue;
        }
        image_paths.push_back(
            {.path = fs::relative(entry.path(), dir),
             .media_type = std::move(media_type)}
        );
    }

    std::sort(
        image_paths.begin(),
        image_paths.end(),
        [](const EpubImage &a, const EpubImage &b) {
            auto a_path = a.path.generic_string();
            auto b_path = b.path.generic_string();
            auto cmp = filename_collator().compare(
                QString::fromStdString(a_path), QString::fromStdString(b_path)
            );

            if (cmp != 0) {
                return cmp < 0;
            }
            // If case-insensitive comparison ties (e.g., `page1.png` and
            // `Page1.png` tie), fall back to standard string comparison as a
            // tie-breaker.
            return a_path < b_path;
        }
    );

    return image_paths;
}

static std::vector<fs::path> collect_cbz_images(const fs::path &dir) {
    auto image_paths = std::vector<fs::path>{};
    for (const auto &entry : fs::recursive_directory_iterator(dir)) {
        auto media_type = image_media_type(entry.path());
        if (!media_type.empty()) {
            image_paths.push_back(entry.path());
        }
    }
    return image_paths;
}

static void add_file_to_archive(
    struct archive *archive, const char *filename, std::string file_contents
) {
    auto entry = archive_entry_new();
    archive_entry_set_pathname(entry, filename);
    archive_entry_set_size(entry, file_contents.length());
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    if (archive_write_header(archive, entry) == ARCHIVE_FATAL) {
        throw ArchiveError();
    }

    auto stream = std::istringstream(file_contents);
    char buffer[8192];
    while (stream.good()) {
        stream.read(buffer, sizeof(buffer));
        auto write = archive_write_data(
            archive, buffer, static_cast<size_t>(stream.gcount())
        );
        if (write == ARCHIVE_FATAL) {
            throw ArchiveError();
        }
    }

    archive_entry_free(entry);
}

static void add_file_to_archive(
    struct archive *archive,
    const char *filename,
    const fs::path &path,
    std::int64_t size
) {
    auto entry = archive_entry_new();
    archive_entry_set_pathname(entry, filename);
    archive_entry_set_size(entry, size);
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    if (archive_write_header(archive, entry) == ARCHIVE_FATAL) {
        throw ArchiveError();
    }

    auto stream = std::ifstream(path, std::ios::binary);
    char buffer[8192];
    while (stream.good()) {
        stream.read(buffer, sizeof(buffer));
        archive_write_data(
            archive, buffer, static_cast<size_t>(stream.gcount())
        );
    }

    archive_entry_free(entry);
}

void create_epub(
    const fs::path &image_dir,
    const fs::path &output_path,
    const std::string &title
) {
    auto archive = archive_write_new();
    archive_write_set_format_zip(archive);

    // Check if we can actually open the output archive path.
    if (archive_write_open_filename(archive, output_path.string().c_str())
        != ARCHIVE_OK) {
        const auto *error = archive_error_string(archive);
        auto message = std::string(error ? error : "");
        archive_write_free(archive);
        throw std::runtime_error(message);
    }

    auto epub_images = collect_epub_images(image_dir);
    if (epub_images.empty()) {
        archive_write_free(archive);
        throw NoImagesError();
        return;
    }

    archive_write_zip_set_compression_store(archive);
    add_file_to_archive(archive, "mimetype", create_epub_mimetype());
    archive_write_zip_set_compression_deflate(archive);

    auto first_image_path = fs::path(epub_images[0].path);
    auto first_page_path
        = "text/"
        + first_image_path.replace_extension(".xhtml").generic_string();
    auto nav_xhtml = create_epub_nav_xhtml(first_page_path);
    add_file_to_archive(archive, "OEBPS/nav.xhtml", nav_xhtml);

    auto container_xml = create_epub_container_xml();
    add_file_to_archive(archive, "META-INF/container.xml", container_xml);

    auto style_css = create_epub_style_css();
    add_file_to_archive(archive, "OEBPS/style.css", style_css);

    QByteArray content_opf_out;
    QBuffer content_opf_buf(&content_opf_out);
    content_opf_buf.open(QIODevice::WriteOnly);
    auto content_opf_writer = QXmlStreamWriter(&content_opf_buf);

    auto book_uuid
        = "urn:uuid:" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto modified_time = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    write_epub_content_opf_beginning(
        content_opf_writer, title, book_uuid, modified_time
    );

    auto page_ids = std::vector<std::string>{};
    page_ids.reserve(epub_images.size());
    for (auto page_num = 1; const auto &epub_image : epub_images) {
        auto image_path_rel = epub_image.path;
        auto image_path_abs = image_dir / image_path_rel;
        auto image
            = vips::VImage::new_from_file(image_path_abs.string().c_str());
        auto page_path = fs::path(image_path_rel)
                             .replace_extension(".xhtml")
                             .generic_string();

        // Add the image file. Store the bytes directly instead of compressing
        // since images are already compressed.
        archive_write_zip_set_compression_store(archive);
        auto image_path_epub
            = "OEBPS/images/" + image_path_rel.generic_string();
        add_file_to_archive(
            archive,
            image_path_epub.c_str(),
            image_path_abs,
            fs::file_size(image_path_abs)
        );
        archive_write_zip_set_compression_deflate(archive);

        // Add the page file.
        auto page_path_epub = "OEBPS/text/" + page_path;
        auto page_xhtml = create_epub_page_xhtml(
            page_num, image_path_rel, image.width(), image.height()
        );
        add_file_to_archive(archive, page_path_epub.c_str(), page_xhtml);

        // Write an element for the image.
        auto image_id = "img" + std::to_string(page_num);
        auto image_href = "images/" + image_path_rel.generic_string();
        write_manifest_item(
            content_opf_writer, image_id, image_href, epub_image.media_type
        );
        if (page_num == 1) {
            content_opf_writer.writeAttribute("properties", "cover-image");
        }

        // Write an element for the page.
        auto page_id = "pg" + std::to_string(page_num);
        auto page_href = "text/" + page_path;
        write_manifest_item(
            content_opf_writer, page_id, page_href, "application/xhtml+xml"
        );
        page_ids.push_back(std::move(page_id));

        page_num += 1;
    }
    content_opf_writer.writeEndElement(); // manifest

    content_opf_writer.writeStartElement("spine");
    for (const auto &page_id : page_ids) {
        content_opf_writer.writeEmptyElement("itemref");
        content_opf_writer.writeAttribute("idref", page_id);
    }
    content_opf_writer.writeEndElement(); // spine

    content_opf_writer.writeEndElement(); // package
    content_opf_writer.writeEndDocument();

    add_file_to_archive(
        archive, "OEBPS/content.opf", content_opf_out.toStdString()
    );

    if (archive_write_close(archive) == ARCHIVE_FATAL) {
        throw ArchiveError();
    }
    archive_write_free(archive);
}

void create_cbz(const fs::path &image_dir, const fs::path &output_path) {
    auto archive = archive_write_new();
    archive_write_set_format_zip(archive);
    archive_write_zip_set_compression_store(archive);

    // Check if we can actually open the output archive path.
    if (archive_write_open_filename(archive, output_path.string().c_str())
        != ARCHIVE_OK) {
        const auto *error = archive_error_string(archive);
        auto message = std::string(error ? error : "");
        archive_write_free(archive);
        throw std::runtime_error(message);
    }

    auto cbz_images = collect_cbz_images(image_dir);
    if (cbz_images.empty()) {
        archive_write_free(archive);
        throw NoImagesError();
        return;
    }

    for (const auto &image_path_rel : cbz_images) {
        auto image_path_abs = image_dir / image_path_rel;

        auto entry = archive_entry_new();
        archive_entry_set_pathname(entry, image_path_rel.string().c_str());
        archive_entry_set_size(entry, fs::file_size(image_path_abs));
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        if (archive_write_header(archive, entry) == ARCHIVE_FATAL) {
            throw ArchiveError();
        }

        auto file_stream = std::ifstream(image_path_abs, std::ios::binary);
        char buffer[8192];
        while (file_stream.good()) {
            file_stream.read(buffer, sizeof(buffer));
            archive_write_data(
                archive, buffer, static_cast<size_t>(file_stream.gcount())
            );
        }

        archive_entry_free(entry);
    }

    if (archive_write_close(archive) == ARCHIVE_FATAL) {
        throw ArchiveError();
    }
    archive_write_free(archive);
}

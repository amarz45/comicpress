#include <algorithm>
#include <archive.h>
#include <archive_entry.h>
#include <filesystem>
#include <functional>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vips/vips8>
#include "fpdfview.h"

#include "task.h"

using Logger = const std::function<void(const std::string&)>&;

namespace fs = std::filesystem;

vips::VImage load_pdf_page(const PageTask& task, Logger log) {
    FPDF_DOCUMENT doc = FPDF_LoadDocument(task.source_file.c_str(), nullptr);
    if (!doc) {
        throw std::runtime_error(
            "PDFium: Cannot open document. Error code: "
            + std::to_string(FPDF_GetLastError())
        );
    }

    FPDF_PAGE page = FPDF_LoadPage(doc, task.page_number);
    if (!page) {
        FPDF_CloseDocument(doc);
        throw std::runtime_error(
            "PDFium: Failed to load page "
            + std::to_string(task.page_number)
        );
    }

    auto width_pt = FPDF_GetPageWidth(page);
    auto height_pt = FPDF_GetPageHeight(page);
    auto width = static_cast<int>(width_pt * 1200.0 / 72.0);
    auto height = static_cast<int>(height_pt * 1200.0 / 72.0);

    FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(width, height, FPDFBitmap_BGR, nullptr, width * 3);
    if (!bitmap) {
        FPDF_ClosePage(page);
        FPDF_CloseDocument(doc);
        throw std::runtime_error(
            "PDFium: Failed to create bitmap for page "
            + std::to_string(task.page_number)
        );
    }

    // Fill with white background.
    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);

    // Render page to bitmap.
    auto render_flags = FPDF_REVERSE_BYTE_ORDER | FPDF_ANNOT | FPDF_NO_NATIVETEXT;
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, render_flags);

    void* buffer = FPDFBitmap_GetBuffer(bitmap);

    // Create VImage by copying the buffer. The buffer is owned by the bitmap,
    // which is destroyed before the function returns, so we must copy.
    vips::VImage img = vips::VImage::new_from_memory_copy(
        buffer, static_cast<size_t>(width) * height * 3, width, height, 3, VIPS_FORMAT_UCHAR
    );

    // Cleanup PDFium objects
    FPDFBitmap_Destroy(bitmap);
    FPDF_ClosePage(page);
    FPDF_CloseDocument(doc);

    return img;
}

vips::VImage load_archive_image(const PageTask& task, Logger log) {
    auto archive = archive_read_new();
    archive_read_support_filter_all(archive);
    archive_read_support_format_all(archive);

    auto archive_open = archive_read_open_filename(
        archive, task.source_file.c_str(), 10240
    );

    if (archive_open != ARCHIVE_OK) {
        std::string err = archive_error_string(archive);
        archive_read_free(archive);
        throw std::runtime_error("LibArchive: Could not open file: " + err);
    }

    struct archive_entry* entry;
    std::vector<char> buffer;

    while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {
        auto path_name = std::string(archive_entry_pathname(entry));
        if (path_name == task.path_in_archive) {
            auto size = archive_entry_size(entry);
            buffer.resize(size);
            archive_read_data(archive, buffer.data(), size);
            break;
        }
    }

    archive_read_close(archive);
    archive_read_free(archive);

    if (buffer.empty()) {
        throw std::runtime_error(
            "Could not find or read '" + task.path_in_archive + "'"
        );
    }

    return vips::VImage::new_from_buffer(buffer.data(), buffer.size(), "");
}

void process_vimage(
    vips::VImage img,
    const fs::path &output_dir,
    const std::string& base_name,
    Logger log
) {
    try {
        auto png_path = output_dir / (base_name + ".png");
        auto output_path = output_dir / (base_name + ".webp");

        log("Processing in-memory page -> " + output_path.filename().string());

        img = img.colourspace(VIPS_INTERPRETATION_B_W);

        auto low = img.min();
        auto high = img.max();

        if (high != low) {
            auto scale = 255.0 / (high - low);
            auto offset = -low * scale;
            img = img.linear(scale, offset);
        }

        double scale = std::min(1440.0 / img.width(), 1920.0 / img.height());
        img = img.resize(
            scale, vips::VImage::option()->set("kernel", VIPS_KERNEL_MKS2021)
        );

        img.pngsave(
            png_path.c_str(),
            vips::VImage::option()
                ->set("compression", 0)
                ->set("palette", true)
                ->set("bitdepth", 4)
                ->set("dither", 1.0)
                ->set("effort", 10)
        );

        vips::VImage final_img = vips::VImage::new_from_file(png_path.c_str());
        final_img.webpsave(
            output_path.c_str(),
            vips::VImage::option()
                ->set("lossless", true)
                ->set("effort", 4)
        );

        fs::remove(png_path);
    }
    catch (const vips::VError &e) {
        log(
            "  -> VIPS Error processing in-memory image "
            + base_name
            + ": "
            + e.what()
        );
    }
}

void process_image_file(
    const fs::path& input_path,
    const fs::path& output_dir,
    const std::string& base_name,
    Logger log
) {
    try {
        log("Processing " + input_path.filename().string());
        auto img = vips::VImage::new_from_file(input_path.c_str());
        process_vimage(img, output_dir, base_name, log);
    }
    catch (const vips::VError& e) {
        log(
            "  -> VIPS Error loading file " + input_path.filename().string()
            + ": " + e.what()
        );
    }
}

void handle_archive(
    const fs::path& archive_path,
    const fs::path& output_dir,
    Logger log
) {
    log("Processing Archive: " + archive_path.filename().string());
    auto temp_extract_dir = fs::temp_directory_path() / archive_path.stem();
    fs::create_directories(temp_extract_dir);

    auto archive = archive_read_new();
    struct archive_entry* entry;

    archive_read_support_filter_all(archive);
    archive_read_support_format_all(archive);

    auto archive_open = archive_read_open_filename(
        archive, archive_path.c_str(), 10240
    );

    if (archive_open != ARCHIVE_OK) {
        log(
            "Error: libarchive could not open file: "
            + std::string(archive_error_string(archive))
        );
        archive_read_free(archive);
        return;
    }

    while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {
        if (archive_entry_filetype(entry) != AE_IFREG) {
            continue;
        }

        auto dest_path = temp_extract_dir / archive_entry_pathname(entry);
        fs::create_directories(dest_path.parent_path());

        std::ofstream out_file(dest_path, std::ios::binary);
        if (!out_file) {
            continue;
        }

        const void *buff;
        size_t size;
        int64_t offset;
        while (true) {
            auto archive_read = archive_read_data_block(
                archive, &buff, &size, &offset
            );
            if (archive_read != ARCHIVE_OK) {
                break;
            }
            out_file.write(static_cast<const char*>(buff), size);
        }
    }

    archive_read_close(archive);
    archive_read_free(archive);

    auto iter = fs::recursive_directory_iterator(temp_extract_dir);

    for (const auto &dir_entry : iter) {
        if (!dir_entry.is_regular_file()) {
            continue;
        }
        process_image_file(
            dir_entry.path(), output_dir, dir_entry.path().stem().string(), log
        );
    }

    fs::remove_all(temp_extract_dir);
}

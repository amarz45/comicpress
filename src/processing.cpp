#include <algorithm>
#include <archive.h>
#include <archive_entry.h>
#include <filesystem>
#include <fpdfview.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <vips/vips8>

#include "include/task.hpp"

using Logger = const std::function<void(const std::string &)> &;

namespace fs = std::filesystem;

vips::VImage load_pdf_page(const PageTask &task, Logger log) {
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
            "PDFium: Failed to load page " + std::to_string(task.page_number)
        );
    }

    auto width_pt = FPDF_GetPageWidth(page);
    auto height_pt = FPDF_GetPageHeight(page);
    auto scale = task.pdf_pixel_density / 72.0;
    auto width = static_cast<int>(width_pt * scale);
    auto height = static_cast<int>(height_pt * scale);

    FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(
        width, height, FPDFBitmap_BGR, nullptr, width * 3
    );
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
    auto render_flags
        = FPDF_REVERSE_BYTE_ORDER | FPDF_ANNOT | FPDF_NO_NATIVETEXT;
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, render_flags);

    void *buffer = FPDFBitmap_GetBuffer(bitmap);

    // Create VImage by copying the buffer. The buffer is owned by the bitmap,
    // which is destroyed before the function returns, so we must copy.
    vips::VImage img = vips::VImage::new_from_memory_copy(
        buffer,
        static_cast<size_t>(width) * height * 3,
        width,
        height,
        3,
        VIPS_FORMAT_UCHAR
    );

    // Cleanup PDFium objects
    FPDFBitmap_Destroy(bitmap);
    FPDF_ClosePage(page);
    FPDF_CloseDocument(doc);

    return img;
}

vips::VImage load_archive_image(const PageTask &task, Logger log) {
    auto archive = archive_read_new();
    archive_read_support_filter_all(archive);
    archive_read_support_format_all(archive);

    auto archive_open
        = archive_read_open_filename(archive, task.source_file.c_str(), 10240);

    if (archive_open != ARCHIVE_OK) {
        std::string err = archive_error_string(archive);
        archive_read_free(archive);
        throw std::runtime_error("LibArchive: Could not open file: " + err);
    }

    struct archive_entry *entry;
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

void process_vimage(vips::VImage img, PageTask task, Logger log) {
    try {
        auto png_path = task.output_dir / (task.output_base_name + ".png");
        auto output_path = task.output_dir / (task.output_base_name + ".webp");

        log("Processing in-memory page -> " + output_path.filename().string());

        img = img.colourspace(VIPS_INTERPRETATION_B_W);

        if (task.stretch_page_contrast) {
            auto min = img.min();
            auto max = img.max();

            if (min != max) {
                auto scale = 255.0 / (max - min);
                auto offset = -min * scale;
                img = img.linear(scale, offset);
            }
        }

        if (task.scale_pages) {
            auto width_ratio = (double)task.page_width / (double)img.width();
            auto height_ratio = (double)task.page_height / (double)img.height();
            double scale = std::min(width_ratio, height_ratio);
            img = img.resize(
                scale,
                vips::VImage::option()->set("kernel", task.page_resampler)
            );
        }

        if (task.quantize_pages) {
            img.pngsave(
                png_path.c_str(),
                vips::VImage::option()
                    ->set("compression", 0)
                    ->set("palette", true)
                    ->set("bitdepth", task.bit_depth)
                    ->set("dither", task.dither)
                    ->set("effort", 10)
            );
        }
        else {
            img.pngsave(
                png_path.c_str(), vips::VImage::option()->set("compression", 0)
            );
        }

        vips::VImage final_img = vips::VImage::new_from_file(png_path.c_str());

        final_img.webpsave(
            output_path.c_str(),
            vips::VImage::option()
                ->set("lossless", !task.is_lossy)
                ->set("effort", task.compression_effort)
        );

        fs::remove(png_path);
    }
    catch (const vips::VError &e) {
        log("  -> VIPS Error processing in-memory image "
            + task.output_base_name + ": " + e.what());
    }
}

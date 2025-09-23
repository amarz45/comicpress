#include <algorithm>
#include <archive.h>
#include <archive_entry.h>
#include <filesystem>
#include <functional>
#include <fstream>
#include <iostream>
#include <mupdf/fitz.h>
#include <stdexcept>
#include <string>
#include <vips/vips8>

#include "task.h"

using Logger = const std::function<void(const std::string&)>&;

namespace fs = std::filesystem;

vips::VImage load_pdf_page(const PageTask& task, Logger log) {
    auto ctx = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
    if (!ctx) {
        throw std::runtime_error("Cannot create MuPDF context.");
    }

    fz_register_document_handlers(ctx);

    fz_document* doc = nullptr;
    vips::VImage img;
    fz_page* page = nullptr;
    fz_pixmap* pix = nullptr;

    try {
        fz_try(ctx) {
            doc = fz_open_document(ctx, task.source_file.c_str());
        }
        fz_catch(ctx) {
            std::string err = fz_caught_message(ctx);
            fz_drop_context(ctx);
            throw std::runtime_error("MuPDF: Cannot open document: " + err);
        }

        auto zoom = 1200.0 / 72.0;
        auto transform = fz_scale(zoom, zoom);

        fz_try(ctx) {
            page = fz_load_page(ctx, doc, task.page_number);
            pix = fz_new_pixmap_from_page(
                ctx, page, transform, fz_device_gray(ctx), 0
            );

            auto samples = fz_pixmap_samples(ctx, pix);
            auto width = fz_pixmap_width(ctx, pix);
            auto height = fz_pixmap_height(ctx, pix);
            auto bands = fz_pixmap_components(ctx, pix);

            img = vips::VImage::new_from_memory_copy(
                samples, (size_t)width * height * bands, width, height, bands,
                VIPS_FORMAT_UCHAR
            );
        }
        fz_catch(ctx) {
            std::string err = fz_caught_message(ctx);
            throw std::runtime_error(
                "MuPDF: Error loading or rendering page: " + err
            );
        }
    }
    catch (...) {
        if (pix) {
            fz_drop_pixmap(ctx, pix);
        }
        if (page) {
            fz_drop_page(ctx, page);
        }
        if (doc) {
            fz_drop_document(ctx, doc);
        }
        if (ctx) {
            fz_drop_context(ctx);
        }

        throw;
    }

    if (pix) {
        fz_drop_pixmap(ctx, pix);
    }
    if (page) {
        fz_drop_page(ctx, page);
    }
    if (doc) {
        fz_drop_document(ctx, doc);
    }
    if (ctx) {
        fz_drop_context(ctx);
    }

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

void handle_pdf(
    const fs::path& pdf_path,
    const fs::path& output_dir,
    Logger log
) {
    log("Processing PDF: " + pdf_path.filename().string());
    fz_context *ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
    if (!ctx) {
        log("Error: Cannot create MuPDF context.");
        return;
    }

    fz_document *doc = nullptr;
    try {
        fz_try(ctx) {
            fz_register_document_handlers(ctx);
        }
        fz_catch(ctx) {
            throw std::runtime_error(
                std::string("MuPDF Error: Cannot register document handlers. ")
                    + fz_caught_message(ctx)
            );
        }

        fz_try(ctx) {
            doc = fz_open_document(ctx, pdf_path.c_str());
        }
        fz_catch(ctx) {
            throw std::runtime_error(
                std::string("MuPDF Error: Cannot open document '")
                    + pdf_path.string()
                    + "'. "
                    + fz_caught_message(ctx)
            );
        }

        auto page_count = fz_count_pages(ctx, doc);
        auto zoom = 1200.0 / 72.0;
        fz_matrix transform = fz_scale(zoom, zoom);

        for (int i = 0; i < page_count; i += 1) {
            fz_page *page = fz_load_page(ctx, doc, i);
            fz_pixmap *pix = fz_new_pixmap_from_page(
                ctx, page, transform, fz_device_gray(ctx), 0
            );

            unsigned char *samples = fz_pixmap_samples(ctx, pix);
            int width = fz_pixmap_width(ctx, pix);
            int height = fz_pixmap_height(ctx, pix);
            int bands = fz_pixmap_components(ctx, pix);

            vips::VImage img = vips::VImage::new_from_memory_copy(
                samples, (size_t) width * height * bands, width, height, bands,
                VIPS_FORMAT_UCHAR
            );

            char base_name[512];
            snprintf(
                base_name, sizeof(base_name), "%s_page_%04d",
                pdf_path.stem().c_str(), i + 1
            );

            process_vimage(img, output_dir, std::string(base_name), log);

            fz_drop_pixmap(ctx, pix);
            fz_drop_page(ctx, page);
        }
    }
    catch (const std::exception& e) {
        log("Error processing PDF: " + std::string(e.what()));
    }

    if (doc) fz_drop_document(ctx, doc);
    if (ctx) fz_drop_context(ctx);
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

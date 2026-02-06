#include <algorithm>
#include <archive.h>
#include <archive_entry.h>
#include <filesystem>
#include <vips/image.h>
#if defined(PDFIUM_ENABLED)
#include <fpdfview.h>
#endif
#include <functional>
#include <stdexcept>
#include <string>
#include <vips/vips8>

#include "../include/task.hpp"
#include "include/processing.hpp"

using Logger = const std::function<void(const std::string &)> &;

namespace fs = std::filesystem;

#if defined(PDFIUM_ENABLED)
static vips::VImage get_vips_img_from_pdf_page(
    FPDF_DOCUMENT doc,
    FPDF_PAGE page,
    int page_number,
    int colour_mode,
    int bands,
    double ppi,
    unsigned int render_flags
);

static bool
is_preview_greyscale(FPDF_DOCUMENT doc, FPDF_PAGE page, int page_number);
#endif

static vips::VImage remove_uniform_middle_columns(const vips::VImage &img);
static bool is_greyscale(vips::VImage img, double threshold);
static bool should_image_rotate(
    double image_width,
    double image_height,
    double display_width,
    double display_height
);
static bool
is_uniform_column(const vips::VImage &img, int col, double threshold);
static bool should_image_stretch_contrast(vips::VImage img, PageTask task);

static vips::VImage
rotate_image(vips::VImage img, RotationDirection rotation_direction);

static vips::VImage scale_image(
    vips::VImage img,
    double source_width,
    double source_height,
    double target_width,
    double target_height,
    VipsKernel resampler,
    bool linear_resample
);

static vips::VImage stretch_image_contrast(vips::VImage img);

#if defined(PDFIUM_ENABLED)
const auto PDF_DEFAULT_RENDER_FLAGS = FPDF_ANNOT | FPDF_NO_NATIVETEXT;

LoadPageReturn load_pdf_page(const PageTask &task) {
    FPDF_DOCUMENT doc
        = FPDF_LoadDocument(task.source_file.string().c_str(), nullptr);
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

    auto render_flags = PDF_DEFAULT_RENDER_FLAGS;

    auto render_page_greyscale = false;
    if (task.convert_pages_to_greyscale) {
        render_page_greyscale
            = is_preview_greyscale(doc, page, task.page_number);
    }

    auto colour_mode = FPDFBitmap_BGR;
    auto bands = 3;
    if (render_page_greyscale) {
        colour_mode = FPDFBitmap_Gray;
        bands = 1;
        render_flags |= FPDF_GRAYSCALE;
    }
    else {
        render_flags |= FPDF_REVERSE_BYTE_ORDER;
    }

    auto img = get_vips_img_from_pdf_page(
        doc,
        page,
        task.page_number,
        colour_mode,
        bands,
        task.pdf_pixel_density,
        render_flags
    );

    auto stretch_page_contrast = should_image_stretch_contrast(img, task);
    if (task.convert_pages_to_greyscale && !render_page_greyscale) {
        img = img.colourspace(VIPS_INTERPRETATION_B_W);
    }

    FPDF_ClosePage(page);
    FPDF_CloseDocument(doc);

    return LoadPageReturn{
        .image = img, .stretch_page_contrast = stretch_page_contrast
    };
}
#endif

LoadPageReturn load_archive_image(const PageTask &task) {
    auto archive = archive_read_new();
    archive_read_support_filter_all(archive);
    archive_read_support_format_all(archive);

    auto archive_open = archive_read_open_filename(
        archive, task.source_file.string().c_str(), 10240
    );

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

    vips::VImage img
        = vips::VImage::new_from_buffer(buffer.data(), buffer.size(), "");
    img = img.copy_memory();

    auto stretch_page_contrast = should_image_stretch_contrast(img, task);
    if (task.convert_pages_to_greyscale) {
        img = img.colourspace(VIPS_INTERPRETATION_B_W);
    }

    return LoadPageReturn{
        .image = img, .stretch_page_contrast = stretch_page_contrast
    };
}

void process_vimage(LoadPageReturn page_info, PageTask task, Logger log) {
    try {
        auto base_path = task.output_dir / task.output_base_name;
        auto png_path = base_path.string() + ".png";
        fs::create_directories(base_path.parent_path());

        bool rotate_option;
        switch (task.double_page_spread_action) {
        case ROTATE:
            rotate_option = true;
            break;
        case BOTH:
            rotate_option = true;
            break;
        default:
            rotate_option = false;
            break;
        }

        auto img = page_info.image;

        auto image_should_rotate = should_image_rotate(
            img.width(), img.height(), task.page_width, task.page_height
        );

        if (image_should_rotate) {
            if (task.remove_spine) {
                img = remove_uniform_middle_columns(img);
            }
            if (rotate_option) {
                img = rotate_image(img, task.rotation_direction);
            }
        }

        if (task.scale_pages) {
            img = scale_image(
                img,
                img.width(),
                img.height(),
                task.page_width,
                task.page_height,
                task.page_resampler,
                task.linear_light_resampling
            );
        }

        if (page_info.stretch_page_contrast) {
            img = stretch_image_contrast(img);
        }

        auto png_palette_options = vips::VImage::option()
                                       ->set("palette", true)
                                       ->set("bitdepth", task.bit_depth)
                                       ->set("dither", task.dither)
                                       ->set("effort", 10);

        if (task.image_format == "PNG") {
            auto base_options = task.quantize_pages ? png_palette_options
                                                    : vips::VImage::option();
            img.pngsave(
                png_path.c_str(),
                base_options->set("compression", task.compression_effort)
            );
            return;
        }

        VipsBlob *png_blob = nullptr;
        if (task.quantize_pages) {
            png_blob = img.pngsave_buffer(
                png_palette_options->set("compression", 0)
            );
            size_t buffer_size = 0;
            const void *buffer_data = vips_blob_get(png_blob, &buffer_size);
            img = vips::VImage::new_from_buffer(buffer_data, buffer_size, "");
        }

        auto options = vips::VImage::option();
        if (task.image_format == "AVIF") {
            auto output_path = base_path.string() + ".avif";
            options
                = options->set("compression", VIPS_FOREIGN_HEIF_COMPRESSION_AV1)
                      ->set("effort", task.compression_effort)
                      ->set("subsample_mode", VIPS_FOREIGN_SUBSAMPLE_ON);
            if (task.is_lossy) {
                options = options->set("Q", task.quality);
            }
            else {
                options = options->set("lossless", true);
            }
            img.heifsave(output_path.c_str(), options);
        }
        else if (task.image_format == "JPEG") {
            auto output_path = base_path.string() + ".jpg";
            img.jpegsave(output_path.c_str(), options->set("Q", task.quality));
        }
        else if (task.image_format == "JPEG XL") {
            auto output_path = base_path.string() + ".jxl";
            options = options->set("effort", task.compression_effort);
            if (!task.is_lossy) {
                options = options->set("distance", 0.0);
            }
            else if (task.quality_type_is_distance) {
                options = options->set("distance", task.quality);
            }
            else {
                options = options->set("Q", task.quality);
            }
            img.jxlsave(output_path.c_str(), options);
        }
        else if (task.image_format == "WebP") {
            auto output_path = base_path.string() + ".webp";
            options = options->set("effort", task.compression_effort);
            if (task.is_lossy) {
                options = options->set("Q", task.quality);
            }
            else {
                options = options->set("lossless", true);
            }
            img.webpsave(output_path.c_str(), options);
        }

        if (png_blob != nullptr) {
            vips_area_unref(VIPS_AREA(png_blob));
        }
    }
    catch (const vips::VError &e) {
        log("  -> VIPS Error processing in-memory image "
            + task.output_base_name + ": " + e.what());
    }
}

#if defined(PDFIUM_ENABLED)
vips::VImage get_vips_img_from_pdf_page(
    FPDF_DOCUMENT doc,
    FPDF_PAGE page,
    int page_number,
    int colour_mode,
    int bands,
    double ppi,
    unsigned int render_flags
) {
    auto width_pt = FPDF_GetPageWidth(page);
    auto height_pt = FPDF_GetPageHeight(page);
    auto scale = ppi / 72.0;
    auto width = static_cast<int>(width_pt * scale);
    auto height = static_cast<int>(height_pt * scale);

    FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(
        width, height, colour_mode, nullptr, width * bands
    );
    if (!bitmap) {
        FPDF_CloseDocument(doc);
        throw std::runtime_error(
            "PDFium: Failed to create bitmap for page "
            + std::to_string(page_number)
        );
    }

    // Fill with white background.
    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);

    // Render page to bitmap.
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, render_flags);

    void *buffer = FPDFBitmap_GetBuffer(bitmap);

    // Create VImage by copying the buffer. The buffer is owned by the bitmap,
    // which is destroyed before the function returns, so we must copy.
    vips::VImage img = vips::VImage::new_from_memory_copy(
        buffer,
        static_cast<size_t>(width) * height * bands,
        width,
        height,
        bands,
        VIPS_FORMAT_UCHAR
    );

    // Cleanup PDFium objects.
    FPDFBitmap_Destroy(bitmap);

    return img;
}
#endif

vips::VImage remove_uniform_middle_columns(const vips::VImage &img) {
    double max_fraction = 0.1;
    int width = img.width();
    int height = img.height();
    int mid = width / 2;

    double global_range = img.max() - img.min();
    if (global_range == 0) {
        return img.copy();
    }

    // Helper lambda to find the bounds of the uniform middle section
    auto get_uniform_bounds = [&](double threshold) {
        int left = mid;
        while (left >= 0 && is_uniform_column(img, left, threshold)) {
            left -= 1;
        }

        int right = mid + 1;
        while (right < width && is_uniform_column(img, right, threshold)) {
            right += 1;
        }
        return std::make_pair(left, right);
    };

    int best_left_bound = mid;
    int best_right_bound = mid + 1;

    // First, attempt with the highest possible threshold as you requested.
    auto bounds_at_max = get_uniform_bounds(global_range);
    int remove_width_at_max = bounds_at_max.second - bounds_at_max.first - 1;
    double frac_at_max = static_cast<double>(remove_width_at_max) / width;

    if (frac_at_max <= max_fraction) {
        // The max threshold is acceptable, so we use its results directly.
        best_left_bound = bounds_at_max.first;
        best_right_bound = bounds_at_max.second;
    }
    else {
        // Max threshold was too aggressive. Fall back to binary search to find
        // a smaller, acceptable threshold.
        double low = 0.0;
        double high = global_range;
        const int max_iter = 20;

        for (int iter = 0; iter < max_iter; ++iter) {
            double mid_th = (low + high) / 2.0;
            auto bounds = get_uniform_bounds(mid_th);
            int remove_width = bounds.second - bounds.first - 1;
            double frac = static_cast<double>(remove_width) / width;

            if (frac > max_fraction) {
                high = mid_th;
            }
            else {
                low = mid_th;
                best_left_bound = bounds.first;
                best_right_bound = bounds.second;
            }
        }
    }

    int remove_width = best_right_bound - best_left_bound - 1;

    if (remove_width <= 0) {
        return img.copy();
    }

    int left_width = best_left_bound + 1;
    int right_width = width - best_right_bound;

    if (left_width <= 0) {
        return img.extract_area(best_right_bound, 0, right_width, height);
    }
    else if (right_width <= 0) {
        return img.extract_area(0, 0, left_width, height);
    }
    else {
        vips::VImage left_part = img.extract_area(0, 0, left_width, height);
        vips::VImage right_part
            = img.extract_area(best_right_bound, 0, right_width, height);
        return left_part.join(right_part, VIPS_DIRECTION_HORIZONTAL);
    }
}

#if defined(PDFIUM_ENABLED)
bool is_preview_greyscale(FPDF_DOCUMENT doc, FPDF_PAGE page, int page_number) {
    auto render_flags = PDF_DEFAULT_RENDER_FLAGS | FPDF_RENDER_NO_SMOOTHTEXT
                      | FPDF_RENDER_NO_SMOOTHIMAGE | FPDF_RENDER_NO_SMOOTHPATH;
    auto preview_img = get_vips_img_from_pdf_page(
        doc, page, page_number, FPDFBitmap_BGR, 3, 10.0, render_flags
    );
    return is_greyscale(preview_img, 10);
}
#endif

bool is_greyscale(vips::VImage img, double threshold) {
    if (img.bands() < 3) {
        return true;
    }
    // Convert to LCh colourspace. The second band is Chroma.
    auto chroma = img.colourspace(VIPS_INTERPRETATION_LCH)[1];
    return chroma.max() <= threshold;
}

bool should_image_rotate(
    double image_width,
    double image_height,
    double display_width,
    double display_height
) {
    auto image_aspect = image_width / image_height;
    auto rotated_image_aspect = image_height / image_width;
    auto display_aspect = display_width / display_height;

    auto original_diff = std::abs(display_aspect - image_aspect);
    auto rotated_diff = std::abs(display_aspect - rotated_image_aspect);

    return rotated_diff < original_diff;
}

bool should_image_stretch_contrast(vips::VImage img, PageTask task) {
    return task.stretch_page_contrast
        && (!task.convert_pages_to_greyscale || is_greyscale(img, 10.0));
}

bool is_uniform_column(const vips::VImage &img, int col, double threshold) {
    vips::VImage column = img.extract_area(col, 0, 1, img.height());
    return column.max() - column.min() < threshold;
}

vips::VImage
rotate_image(vips::VImage img, RotationDirection rotation_direction) {
    double angle = 0.0;
    switch (rotation_direction) {
    case CLOCKWISE:
        angle = 90.0;
        break;
    case COUNTERCLOCKWISE:
        angle = -90.0;
        break;
    }
    return img.rotate(angle);
}

vips::VImage scale_image(
    vips::VImage img,
    double source_width,
    double source_height,
    double target_width,
    double target_height,
    VipsKernel resampler,
    bool linear_resample
) {
    auto gap = 0.0;
    auto width_ratio = target_width / source_width;
    auto height_ratio = target_height / source_height;
    double scale = std::min(width_ratio, height_ratio);

    if (!linear_resample) {
        return img.resize(
            scale,
            vips::VImage::option()->set("kernel", resampler)->set("gap", gap)
        );
    }

    VipsInterpretation interpretation_linear;
    VipsInterpretation interpretation_gamma;
    std::string colourspace_gamma;
    if (img.bands() < 3) {
        interpretation_linear = VIPS_INTERPRETATION_GREY16;
        interpretation_gamma = VIPS_INTERPRETATION_B_W;
        colourspace_gamma = "sgrey";
    }
    else {
        interpretation_linear = VIPS_INTERPRETATION_scRGB;
        interpretation_gamma = VIPS_INTERPRETATION_sRGB;
        colourspace_gamma = "srgb";
    }

    auto has_icc = img.get_typeof(VIPS_META_ICC_NAME) != 0;
    if (has_icc) {
        img = img.icc_import(vips::VImage::option()->set("pcs", VIPS_PCS_XYZ));
    }
    else {
        img = img.colourspace(interpretation_linear);
    }

    if (img.has_alpha()) {
        img.premultiply();
    }

    img = img.resize(
        scale, vips::VImage::option()->set("kernel", resampler)->set("gap", gap)
    );

    if (img.has_alpha()) {
        img.unpremultiply();
    }

    if (has_icc) {
        std::string colourspace;
        img = img.icc_export(
            vips::VImage::option()
                ->set("pcs", VIPS_PCS_XYZ)
                ->set("output_profile", colourspace_gamma.c_str())
        );
    }
    else {
        img = img.colourspace(interpretation_gamma);
    }

    return img;
}

vips::VImage stretch_image_contrast(vips::VImage img) {
    auto min = static_cast<double>(img.percent(5.0));
    auto max = static_cast<double>(img.percent(95.0));
    if (max - min != 0) {
        auto scale = 255.0 / (max - min);
        auto offset = -min * scale + 0.5;
        img = img.linear(scale, offset);
        img = img.cast(VIPS_FORMAT_UCHAR);
    }
    return img;
}

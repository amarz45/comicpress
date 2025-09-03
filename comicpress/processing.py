import os
import zipfile
import rarfile
import pymupdf
import pyvips

def process_task(
    task, dpi, display, resample, img_format, webp_method, png_compression_level
):
    kind, source, data, output_dir = task
    if kind == "pdf":
        pdf_path, i = source, data
        return process_pdf_page(
            pdf_path, img_format, i, output_dir, dpi, display, resample,
            webp_method, png_compression_level
        )
    elif kind == "cbz":
        cbz_path, (i, filename) = source, data
        return process_archive_image(
            cbz_path, filename, img_format, i, output_dir, display, resample,
            zipfile.ZipFile, webp_method, png_compression_level
        )
    elif kind == "cbr":
        cbr_path, (i, filename) = source, data
        return process_archive_image(
            cbr_path, filename, img_format, i, output_dir, display, resample,
            rarfile.RarFile, webp_method, png_compression_level
        )

def process_pdf_page(
    pdf_path, img_format, index, output_dir, dpi, display, resample,
    webp_method, png_compression_level
):
    doc = pymupdf.open(pdf_path)
    page = doc[index]
    zoom = dpi / 72
    matrix = pymupdf.Matrix(zoom, zoom)
    pix = page.get_pixmap(matrix = matrix, colorspace = pymupdf.csGRAY)
    doc.close()

    img = pyvips.Image.new_from_memory(pix.samples, pix.width, pix.height, 1, "uchar")
    return save_processed_image(
        img, output_dir, img_format, index, display, resample,
        is_mostly_greyscale(img), webp_method, png_compression_level
    )

def process_archive_image(
    archive_path, filename, img_format, index, output_dir, display, resample,
    opener, webp_method, png_compression_level
):
    with opener(archive_path, "r") as archive:
        data = archive.read(filename)
    img = pyvips.Image.new_from_buffer(data, "")
    is_originally_greyscale = is_mostly_greyscale(img)

    img = img.colourspace(pyvips.enums.Interpretation.B_W)
    return save_processed_image(
        img, output_dir, img_format, index, display, resample,
        is_originally_greyscale, webp_method, png_compression_level
    )

def save_processed_image(
    img, output_dir, img_format, index, display, resample,
    is_originally_greyscale, webp_method, png_compression_level
):
    if is_originally_greyscale:
        low = img.min()
        high = img.max()
        if high != low:
            scale = 255.0 / (high - low)
            offset = -low * scale
            img = img.linear(scale, offset)

    if display:
        scale = min(1440 / img.width, 1920 / img.height)
        img = img.resize(scale, kernel = resample)

    output_path = output_dir / f"{index + 1:03d}"

    if img_format == "JPEG":
        img.jpegsave(f"{output_path}.jpg")
    else:
        png_path = f"{output_path}.png"
        img.pngsave(
            png_path,
            compression = png_compression_level if img_format == "PNG" else 0,
            palette = True,
            bitdepth = 4,
            dither = 1.0,
            effort = 10
        )

        if img_format != "PNG":
            img = pyvips.Image.new_from_file(png_path)

            if img_format == "AVIF":
                img.heifsave(
                    f"{output_path}.avif",
                    compression = pyvips.enums.ForeignHeifCompression.AV1,
                    subsample_mode = pyvips.enums.ForeignSubsample.ON,
                    Q = 50
                )
            elif img_format == "JPEG XL":
                img.jxlsave(f"{output_path}.jxl", distance = 0)
            else:
                img.webpsave(
                    f"{output_path}.webp",
                    lossless = True,
                    effort = webp_method
                )

            os.remove(png_path)

def is_mostly_greyscale(img_orig, threshold = 10):
    if img_orig.bands < 3:
        return True

    # Convert to LCh colorspace. The second band is Chroma.
    chroma = img_orig.colourspace("lch")[1]
    max_chroma = chroma.max()

    return max_chroma < threshold

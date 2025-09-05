import pymupdf
import pyvips
import zipfile
import rarfile
import os

def process_task(
    task, dpi, display, resample, bit_depth, img_format, webp_method,
    png_compression_level
):
    kind, source, data, output_dir = task
    if kind == "pdf":
        pdf_path, i = source, data
        return process_pdf_page(
            pdf_path, img_format, i, output_dir, dpi, display, resample,
            bit_depth, webp_method, png_compression_level
        )
    elif kind == "cbz":
        cbz_path, (i, filename) = source, data
        return process_archive_image(
            cbz_path, filename, img_format, i, output_dir, display, resample,
            bit_depth, zipfile.ZipFile, webp_method, png_compression_level
        )
    elif kind == "cbr":
        cbr_path, (i, filename) = source, data
        return process_archive_image(
            cbr_path, filename, img_format, i, output_dir, display, resample,
            bit_depth, rarfile.RarFile, webp_method, png_compression_level
        )

def process_pdf_page(
    pdf_path, img_format, index, output_dir, dpi, display, resample,
    bit_depth, webp_method, png_compression_level
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
        bit_depth, is_mostly_greyscale(img), webp_method, png_compression_level
    )

def process_archive_image(
    archive_path, filename, img_format, index, output_dir, display, resample,
    bit_depth, opener, webp_method, png_compression_level
):
    with opener(archive_path, "r") as archive:
        data = archive.read(filename)
    img = pyvips.Image.new_from_buffer(data, "")
    is_originally_greyscale = is_mostly_greyscale(img)

    img = img.colourspace(pyvips.enums.Interpretation.B_W)
    return save_processed_image(
        img, output_dir, img_format, index, display, resample,
        bit_depth, is_originally_greyscale, webp_method, png_compression_level
    )

def save_processed_image(
    img, output_dir, img_format, index, display, resample,
    bit_depth, is_originally_greyscale, webp_method, png_compression_level
):
    if is_originally_greyscale:
        low = img.min()
        high = img.max()
        if high != low:
            scale = 255.0 / (high - low)
            offset = -low * scale
            img = img.linear(scale, offset)

    if display:
        scale = min(display.width / img.width, display.height / img.height)
        img = img.resize(scale, kernel = resample)

    output_path = output_dir / f"{index + 1:03d}"
    png_compression_level = png_compression_level if img_format == "PNG" else 0
    png_path = f"{output_path}.png"

    if bit_depth:
        img.pngsave(
            png_path,
            compression = png_compression_level,
            palette = True,
            bitdepth = bit_depth,
            dither = 1.0,
            effort = 10
        )
        if img_format == "PNG":
            return f"Saved {png_path}."
        img = pyvips.Image.new_from_file(png_path)

    if img_format == "PNG":
        img.pngsave(png_path, compression = png_compression_level)
        return f"Saved {png_path}."

    if img_format == "AVIF":
        output_path = f"{output_path}.avif"
        img.heifsave(
            output_path,
            compression = pyvips.enums.ForeignHeifCompression.AV1,
            subsample_mode = pyvips.enums.ForeignSubsample.ON,
            Q = 50
        )
    elif img_format == "JPEG":
        output_path = f"{output_path}.jpg"
        img.jpegsave(output_path)
    elif img_format == "JPEG XL":
        output_path = f"{output_path}.jxl"
        if bit_depth:
            img.jxlsave(output_path, distance = 0)
        else:
            img.jxlsave(output_path)
    else:
        output_path = f"{output_path}.webp"
        img.webpsave(
            output_path,
            lossless = bit_depth != None,
            effort = webp_method
        )

    os.remove(png_path)

    return f"Saved {output_path}."

def is_mostly_greyscale(img_orig, threshold = 10):
    if img_orig.bands < 3:
        return True

    # Convert to LCh colorspace. The second band is Chroma.
    chroma = img_orig.colourspace("lch")[1]
    max_chroma = chroma.max()

    return max_chroma < threshold

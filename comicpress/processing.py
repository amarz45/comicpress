import io
import zipfile
import rarfile
import pymupdf
from PIL import Image, ImageOps

def process_task(task, dpi, display, resample, img_format, webp_method, png_compression_level):
    kind, source, data, output_dir = task
    if kind == "pdf":
        pdf_path, i = source, data
        return process_pdf_page(pdf_path, img_format, i, output_dir, dpi, display, resample, webp_method, png_compression_level)
    elif kind == "cbz":
        cbz_path, (i, filename) = source, data
        return process_archive_image(cbz_path, filename, img_format, i, output_dir, display, resample, zipfile.ZipFile, webp_method, png_compression_level)
    elif kind == "cbr":
        cbr_path, (i, filename) = source, data
        return process_archive_image(cbr_path, filename, img_format, i, output_dir, display, resample, rarfile.RarFile, webp_method, png_compression_level)

def process_pdf_page(pdf_path, img_format, index, output_dir, dpi, display, resample, webp_method, png_compression_level):
    doc = pymupdf.open(pdf_path)
    page = doc[index]
    zoom = dpi / 72
    matrix = pymupdf.Matrix(zoom, zoom)
    pix = page.get_pixmap(matrix=matrix, colorspace=pymupdf.csGRAY)
    doc.close()

    img = Image.frombytes("L", [pix.width, pix.height], pix.samples)
    return save_processed_image(img, output_dir, img_format, index, display, resample, webp_method, png_compression_level)

def process_archive_image(
    archive_path,
    filename,
    img_format,
    index,
    output_dir,
    display,
    resample,
    opener,
    webp_method,
    png_compression_level,
):
    with opener(archive_path, "r") as archive:
        data = archive.read(filename)
    img = Image.open(io.BytesIO(data)).convert("L")
    return save_processed_image(img, output_dir, img_format, index, display, resample, webp_method, png_compression_level)

def save_processed_image(img, output_dir, img_format, index, display, resample, webp_method, png_compression_level):
    img = ImageOps.autocontrast(img, preserve_tone=True)
    if display:
        img.thumbnail((display.width, display.height), resample)
    else:
        img.thumbnail((display.width, display.height))

    img = img.quantize(
        colors=16,
        method=Image.Quantize.LIBIMAGEQUANT,
        dither=Image.Dither.FLOYDSTEINBERG,
    )

    output_file = output_dir / f"{index + 1:03d}"

    if img_format == "AVIF":
        output_file = f"{output_file}.avif"
        img.save(output_file)
    elif img_format == "JPEG":
        output_file = f"{output_file}.jpg"
        img.save(output_file)
    elif img_format == "JPEG XL":
        img = img.convert("L")
        output_file = f"{output_file}.jxl"
        img.save(output_file)
    elif img_format == "PNG":
        output_file = f"{output_file}.png"
        img.save(output_file, compress_level = png_compression_level)
    elif img_format == "WebP":
        output_file = f"{output_file}.webp"
        img.save(output_file, lossless = True, method = webp_method)

    return f"Saved {output_file}"

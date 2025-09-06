import pymupdf
import pyvips
import os
import rarfile
import zipfile

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .config import Config
    from pathlib import Path

def process_task(
    task: tuple[str, str, int | tuple[int, str], "Path"],
    config: "Config"
) \
-> str | None:
    kind, source, data, output_dir = task
    if kind == "pdf":
        pdf_path, i = source, data
        return process_pdf_page(pdf_path, i, output_dir, config)
    elif kind == "cbz":
        cbz_path, (i, filename) = source, data
        return process_archive_image(
            cbz_path, filename, i, output_dir, zipfile.ZipFile, config
        )
    elif kind == "cbr":
        cbr_path, (i, filename) = source, data
        return process_archive_image(
            cbr_path, filename, i, output_dir, rarfile.RarFile, config
        )

def process_pdf_page(
    pdf_path: str,
    index: int,
    output_dir: "Path",
    config: "Config"
) \
-> str:
    doc = pymupdf.open(pdf_path)
    page = doc[index]
    zoom = config.dpi / 72
    matrix = pymupdf.Matrix(zoom, zoom)
    if config.display and config.display.colour:
        pix = page.get_pixmap(matrix = matrix)
    else:
        pix = page.get_pixmap(matrix = matrix, colorspace = pymupdf.csGRAY)
    doc.close()

    img = pyvips.Image.new_from_memory(
        pix.samples, pix.width, pix.height, pix.n, "uchar"
    )
    output_path_base = output_dir / f"{index + 1:03d}"
    return save_processed_image(
        img, output_path_base, is_mostly_greyscale(img), config
    )

def process_archive_image(
    archive_path: str,
    filename: str,
    index: int,
    output_dir: "Path",
    opener: type[zipfile.ZipFile] | type[rarfile.RarFile],
    config: "Config"
) \
-> str:
    with opener(archive_path, "r") as archive:
        data = archive.read(filename)
    img = pyvips.Image.new_from_buffer(data, "")
    is_originally_greyscale = is_mostly_greyscale(img)

    if not config.display or not config.display.colour:
        img = img.colourspace(pyvips.enums.Interpretation.B_W)

    # Get the directory part of the filename from the archive.
    internal_dir = os.path.dirname(filename)
    # Create the corresponding subdirectory in the output.
    full_output_dir = output_dir / internal_dir
    full_output_dir.mkdir(parents = True, exist_ok = True)
    # The base path for the output file, preserving the subdirectory.
    output_path_base = full_output_dir / f"{index + 1:03d}"

    return save_processed_image(
        img, output_path_base, is_originally_greyscale, config
    )

def save_processed_image(
    img: pyvips.Image,
    output_path_base: "Path",
    is_originally_greyscale: bool,
    config: "Config"
) \
-> str:
    if config.stretch_contrast and is_originally_greyscale:
        low = img.min()
        high = img.max()
        if high != low:
            scale = 255.0 / (high - low)
            offset = -low * scale
            img = img.linear(scale, offset)

    if config.display:
        width_ratio = config.display.width / img.width
        height_ratio = config.display.height / img.height
        scale = min(width_ratio, height_ratio)
        img = img.resize(scale, kernel = config.resample)

    png_compression_level = (
        config.png_compression_level if config.img_format == "PNG" else 0
    )
    png_path = f"{output_path_base}.png"

    if config.bit_depth:
        img.pngsave(
            png_path,
            compression = png_compression_level,
            palette = True,
            bitdepth = config.bit_depth,
            dither = config.dither,
            effort = 10
        )
        if config.img_format == "PNG":
            return f"Saved {png_path}."
        img = pyvips.Image.new_from_file(png_path)

    if config.img_format == "PNG":
        img.pngsave(png_path, compression = png_compression_level)
        return f"Saved {png_path}."

    if config.img_format == "AVIF":
        output_path = f"{output_path_base}.avif"
        img.heifsave(
            output_path,
            compression = pyvips.enums.ForeignHeifCompression.AV1,
            subsample_mode = pyvips.enums.ForeignSubsample.ON,
            Q = 50
        )
    elif config.img_format == "JPEG":
        output_path = f"{output_path_base}.jpg"
        img.jpegsave(output_path)
    elif config.img_format == "JPEG XL":
        output_path = f"{output_path_base}.jxl"
        if config.bit_depth:
            img.jxlsave(output_path, distance = 0)
        else:
            img.jxlsave(output_path)
    else:
        output_path = f"{output_path_base}.webp"
        img.webpsave(
            output_path,
            lossless = config.bit_depth != None,
            effort = config.webp_method
        )

    os.remove(png_path)

    return f"Saved {output_path}."

def is_mostly_greyscale(img_orig: pyvips.Image, threshold: int = 10) -> bool:
    if img_orig.bands < 3:
        return True

    # Convert to LCh colorspace. The second band is Chroma.
    chroma = img_orig.colourspace("lch")[1]
    max_chroma = chroma.max()

    return max_chroma < threshold

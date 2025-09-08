from pyvips.enums import Kernel

TITLE = "Comicpress"

# PDF
PDF_LABEL = "PDF pixel density (PPI)"
PDF_TOOLTIP = """
Sets the rendering resolution for PDF files. Higher values
produce better quality but use more memory and take longer to
process. Values higher than the target display’s pixel density
still result in higher perceived quality on that display.
Generally, it’s exceedingly difficult to notice a difference in
quality for values greater than 1200 PPI. However, rasterizing a
page in a PDF file takes roughly four times as long at 1200 PPI
"than at 600 PPI.
"""

# Contrast
CONTRAST_LABEL = "Stretch contrast"
CONTRAST_TOOLTIP = """
Automatically adjusts the page’s black and white points to
maximize contrast. This is highly recommended when reading on an
ereader, as it makes the colours “pop.”
"""

# Scale
SCALE_LABEL = "Scale images"
SCALE_TOOLTIP = """
Scales pages to fit the target resolution. This is highly
recommended because it can potentially decrease file sizes
significantly. In addition, it improves quality since it prevents
the ereader from scaling pages itself. (Ereaders usually use
subpar scaling algorithms for speed.)
"""

# Resampling filter
RESAMPLING_FILTERS = [
    "Bicubic interpolation", "Bilinear interpolation", "Lanczos 2", "Lanczos 3",
    "Magic Kernel Sharp 2013", "Magic Kernel Sharp 2021", "Mitchell",
    "Nearest neighbour"
]
RESAMPLING_FILTER_DEFAULT = "Magic Kernel Sharp 2021"
RESAMPLING_FILTER_LABEL = "Resampling filter"
RESAMPLING_FILTER_TOOLTIP = """
The algorithm used to calculate pixel values when resizing an
image. <i>Magic Kernel Sharp 2021</i> is recommended for the best
quality and <i>Lanczos 3</i> is a close second. Other resampling
filters are faster but usually result in significantly lower
quality.
"""

RESAMPLE_FILTER_MAP = {
    "Bicubic interpolation": Kernel.CUBIC,
    "Bilinear interpolation": Kernel.LINEAR,
    "Lanczos 2": Kernel.LANCZOS2,
    "Lanczos 3": Kernel.LANCZOS3,
    "Magic Kernel Sharp 2013": Kernel.MKS2013,
    "Magic Kernel Sharp 2021": Kernel.MKS2021,
    "Mitchell": Kernel.MITCHELL,
    "Nearest": Kernel.NEAREST,
}

# Quantize
QUANTIZE_LABEL = "Quantize images"
QUANTIZE_TOOLTIP = """
Limits the colour palette of pages. This is highly recommended
because it decreases file size, and ereaders usually only have 16
shades of grey anyway. In addition, this usually improves quality
because it prevents the ereader from using its own (likely
subpar) quantization algorithm. If you enable this option, make
sure to disable dithering on your ereader! Otherwise, the quality
will be ruined."
"""

# Bit depth
BIT_DEPTH_LABEL = "Bit depth"
BIT_DEPTH_TOOLTIP = """
Determines the number of colours in the final images.
A bit depth of 4 (16 colours) is recommended in most cases
because most ereaders only have 16 colours.
<br><br>
<b>Bit depth → Colours</b><br>
1 → 2<br>
2 → 4<br>
4 → 16<br>
8 → 256<br>
16 → 65 536
"""

# Dither
DITHER_LABEL = "Dither"
DITHER_TOOLTIP = """
Sets the dither level, which is used to make quantized images
look better. It’s highly recommended recommended to leave this at
the default maximum value of 1.0 to significantly improve quality.
"""

# Image format
IMG_FORMATS = [
    "AVIF", "JPEG", "JPEG XL", "PNG", "WebP"
]
IMG_FORMAT_DEFAULT = "PNG"
IMG_FORMAT_LABEL = "Image format"
IMG_FORMAT_TOOLTIP = """
Sets the image format for each page.
<dl>
    <dt>AVIF</dt>
    <dd>Good for photographs, but not optimal for comics. Results in large file sizes.</dd>

    <dt>JPEG</dt>
    <dd>Worst option for both quality and file size. Only choose
    this if no other image format is supported.</dd>

    <dt>JPEG XL</dt>
    <dd>The absolute best for minimizing file size while
    maintaining the same quality. Choose this if your ereader
    software supports it.</dd>

    <dt>PNG</dt>
    <dd>Widely supported, lossless, and has decent compression.
    Choose this if your ereader software doesn’t support JPEG XL
    or WebP, or if you’re unsure about whether it does.</dd>

    <dt>WebP</dt>
    <dd>Excellent compression and quality: better than PNG, but
    not as good as JPEG XL. Choose this if your ereader software
    supports WebP but doesn’t support JPEG XL
    (e.g., KOReader).</dd>
</dl>
"""

# Compression
COMPRESSION_LABEL = "Compression type"
COMPRESSION_TYPES = ["Lossless", "Lossy"]
COMPRESSION_DEFAULT = "Lossless"
COMPRESSION_EFFORT_LABEL = "Compression effort"

# Quality
QUALITY_LABEL = "Quality"
JPEG_XL_QUALITY_TYPES = ["Distance", "Quality"]

static const char *PDF_TOOLTIP = R"(
    Sets the rendering resolution for PDF input files. Higher values produce
    better quality but use more memory and take longer to process. Values higher
    than the target display’s pixel density still result in higher perceived
    quality on that display. Generally, it’s exceedingly difficult to notice a
    difference in quality for values greater than 1200 PPI. However, rasterizing
    a page in a PDF file takes roughly four times as long at 1200 PPI than at
    600 PPI.
)";

static const char *GREYSCALE_TOOLTIP = R"(
    Converts coloured pages to greyscale. This option is recommended for
    ereaders that don’t support colour, since it reduces file size at no cost to
    quality.
)";

static const char *DOUBLE_PAGE_SPREAD_TOOLTIP = R"(
    What to do when a double-page spread (when two pages are put together into a
    single page).
)";

static const char *REMOVE_SPINE_TOOLTIP = R"(
    Remove spines which separate two pages in a double-page spread. This can
    help to unify double-page spreads, but note that it might remove parts of
    the actual pages.
)";

static const char *CONTRAST_TOOLTIP = R"(
    Automatically adjusts the page’s black and white points to maximize
    contrast. This is highly recommended when reading on an ereader, as it makes
    the colours “pop.”
)";

static const char *SCALE_TOOLTIP = R"(
    Scales pages to fit the target resolution. This is highly recommended
    because it can potentially decrease file sizes significantly. In addition,
    it improves quality since it prevents the ereader from scaling pages itself.
    (Ereaders usually use subpar scaling algorithms for speed.)
)";

static const char *RESAMPLER_TOOLTIP = R"(
    The algorithm used to calculate pixel values when resizing an image.
    <i>Magic Kernel Sharp 2021</i> is recommended for the best quality and
    <i>Lanczos 3</i> is a close second. Other resampling filters are faster but
    usually result in significantly lower quality.
)";

static const char *QUANTIZE_TOOLTIP = R"(
    Limits the colour palette of pages. This is highly recommended because it
    decreases file size, and ereaders usually only have 16 shades of grey
    anyway. In addition, this usually improves quality because it prevents the
    ereader from using its own (likely subpar) quantization algorithm. If you
    enable this option, make sure to disable dithering on your ereader!
    Otherwise, the quality will be ruined."
)";

static const char *BIT_DEPTH_TOOLTIP = R"(
    Determines the number of colours in the final images. A bit depth of 4 (16
    colours) is recommended in most cases because most ereaders only have 16
    colours.
)";

static const char *DITHERING_TOOLTIP = R"(
    Sets the dithering level, which is used to make quantized images look better.
    It’s highly recommended recommended to leave this at the default maximum
    value of 1.0 to significantly improve quality.
)";

static const char *IMG_FORMAT_TOOLTIP = R"(
    Sets the image format for each page.
    <dl>
        <dt>AVIF</dt>
        <dd>
            Good for photographs, but not optimal for comics. Results in large
            file sizes.
        </dd>

        <dt>JPEG</dt>
        <dd>
            Worst option for both quality and file size. Only choose this if no
            other image format is supported.
        </dd>

        <dt>JPEG XL</dt>
        <dd>
            The absolute best for minimizing file size while maintaining the
            same quality. Choose this if your ereader software supports it.
        </dd>

        <dt>PNG</dt>
        <dd>
            Widely supported, lossless, and has decent compression. Choose this
            if your ereader software doesn’t support JPEG XL or WebP, or if
            you’re unsure about whether it does.
        </dd>

        <dt>WebP</dt>
        <dd>
            Excellent compression and quality: better than PNG, but not as good
            as JPEG XL. Choose this if your ereader software supports WebP but
            doesn’t support JPEG XL (e.g., KOReader).
        </dd>
    </dl>
)";

static const char *IMAGE_COMPRESSION_TYPE_TOOLTIP = R"(
    Sets the image compression type. <i>Lossless</i> is strongly recommended
    when <i>Quantize pages</i> is enabled because it results in smaller file
    sizes as well as better quality. If <i>Quantize pages</i> is disabled,
    <i>Lossy</i> is recommended because it decreases file sizes significantly
    with only a minor negative impact on quality.
)";

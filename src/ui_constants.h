static const char *PDF_TOOLTIP = R"(
    Sets the rendering resolution for PDF files. Higher values produce better
    quality but use more memory and take longer to process. Values higher than
    the target display’s pixel density still result in higher perceived quality
    on that display. Generally, it’s exceedingly difficult to notice a
    difference in quality for values greater than 1200 PPI. However, rasterizing
    a page in a PDF file takes roughly four times as long at 1200 PPI than at
    600 PPI.
)";

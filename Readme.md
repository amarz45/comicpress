# Comicpress

Comicpress is an application that optimizes comics and manga for ereaders. It accepts CBR, CBZ, and PDF files as input formats, and converts them to optimized CBZ files. The two primary benefits of using Comicpress are that it sharply decreases file sizes while also significantly improving perceived quality. Comicpress applies image processing operations such as contrast stretching, high-quality scaling, quantization, and dithering to significantly improve the perceived quality on ereaders. These settings can customized or disabled. Comicpress also downscales pages to your specific ereader device’s display resolution, which not only significantly decreases file sizes but also likely improves the perceived quality on the ereader due to the high-quality default resampler (Magic Kernel Sharp 2021). _Comicpress compresses comics and mangas to as low as 20% their original size while improving visual fidelity._

![Screenshot](Screenshot.png)

## Comparison to Kindle Comic Converter

Comicpress is similar to [KCC](https://github.com/ciromattia/kcc), but different in important ways:

- When PDF files are used as input files, the final files produced by Comicpress are higher quality than those produced by KCC. This is because Comicpress lets you customize the pixel density. On the other hand, KCC always rasterizes PDFs at a pixel density dependent on the target and page heights, which usually results in lower pixel density values.
- KCC uses Pillow to process images, whereas Comicpress uses VIPS. VIPS is slightly faster, but most importantly it supports more features and image formats (see below).
- In addition to the widely used JPEG and PNG image formats, Comicpress supports AVIF, JPEG XL, and WebP for pages. WebP results in noticeably smaller files than PNG and is supported by KOReader.
- Comicpress uses a higher quality resampling filter by default (Magic Kernel Sharp 2021). KCC’s resampling is not customizable: it uses Lanczos for downscaling and bicubic interpolation for upscaling, resulting in worse quality than Comicpress.
- Comicpress’s image processing operations are more customizable than in KCC. Scaling, quantization, and dithering can easily be adjusted or turned off.
- Comicpress generally has an easier-to-use user interface than KCC.
- There are a few features present in KCC that are not in Comicpress, such as automatically cropping out margins. I plan to eventually add these in the future.

## Build Instructions

```console
$ git clone --depth=1 https://github.com/amarz45/comicpress
$ CXX=clang++ meson setup build --buildtype=release
$ ninja -C build
```

# Comicpress

_If you own a Kobo, Boox, Kindle, or other ereader and read DRM-free manga or
comics, Comicpress can shrink your file sizes by up to 80% and, as a bonus,
makes them look better on e-ink than the originals._

Comicpress can process DRM-free CBR, CBZ, and PDF files. PDF files are preferred since they
usually have higher quality, but CBR and CBZ file types are well-supported.
Comicpress only works on DRM-free files. It cannot process comics and manga
purchased from Kindle, Kobo, and other vendors unless it is explicitly
advertised as DRM-free.

Comicpress takes your comic book files (supported formats: CBR, CBZ, PDF) and compresses them to a fraction of the size while also significantly improving the visual quality on ereaders. It does so by applying image processing operations such as contrast stretching, high-quality scaling, quantization, and dithering. These can all be customized or disabled. Comicpress also downscales pages to your specific ereader device’s display resolution, which not only significantly decreases file sizes but also likely improves the perceived quality on the ereader using high-quality default resampling.

![Screenshot](screenshot.png)

## Comparison to Kindle Comic Converter

[KCC](https://github.com/ciromattia/kcc) is an older and popular alternative to Comicpress, but Comicpress has numerous advantages.

- When processing PDF files, the files produced by Comicpress are higher quality than those produced by KCC. KCC always renders PDFs at a pixel density dependent on the target and page heights, which usually results in lower pixel densities. Comicpress defaults to 300 PPI and allows you to choose a higher pixel density for better quality.
- Comicpress supports a wider variety of image formats including AVIF, JPEG XL, and WebP for the pages in the CBZ output files, in addition to the widely used PNG and JPEG formats.
- Comicpress uses a higher-quality image resampler by default (Magic Kernel Sharp 2021). KCC’s resampling is not customizable: it uses Lanczos for downscaling and bicubic interpolation for upscaling, resulting in worse quality than Comicpress.
- Comicpress’s image processing operations are more customizable than in KCC. Scaling, quantization, and dithering can easily be toggled and adjusted.
- Comicpress generally has an easier-to-use user interface than KCC.
- There are a few features present in KCC that are not in Comicpress, such as automatically cropping out margins. I plan to eventually add these in the future.

## Installation

## Building From Source

### Linux

#### Installing dependencies

##### Prerequisites

First, go to https://github.com/bblanchon/pdfium-binaries/releases/latest. Download the file that matches your operating system and CPU. For example, on x86-64 Linux systems this would be `pdfium-linux-x64.tar.gz`. `cd` into the directory containing the tarball (usually `~/Downloads`). Extract and install:

```console
$ mkdir pdfium
$ tar -xzf pdfium-linux-x64.tar.gz -C pdfium
$ cd pdfium
# cp -r include/ lib/ /usr/local/
# ldconfig
```

Now, we need to install the other dependencies.

##### Debian-based systems (Debian, Ubuntu, etc.)

```console
# apt install build-essential meson ninja-build pkgconf libvips-dev qt6-base-dev libarchive-dev
```

##### DNF-based systems (Fedora, RHEL, etc.)

```console
# dnf install gcc-c++ meson ninja-build pkgconf-pkg-config vips-devel qt6-qtbase-devel libarchive-devel
```

##### Compiling

```console
$ git clone --depth=1 https://github.com/amarz45/comicpress
$ cd comicpress
$ meson setup build --buildtype=release
$ meson compile -C build
```

## Copyright

Copyright (C) 2026 Amar Al-Zubaidi.

Comicpress is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version. See [`Licence.txt`](Licence.txt) for the full text.

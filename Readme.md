# Comicpress

_If you own a Kobo, Kindle, or other ereader and read DRM-free manga or comics,
Comicpress can shrink your file sizes by up to 80% and, as a bonus, makes them
look better on e-ink than the originals._

Comicpress can process DRM-free CBR, CBZ, and PDF files. PDF files are preferred
since they usually have higher quality, but CBR and CBZ file types are
well-supported. The output format is EPUB (default) or CBZ. Comicpress only
works on DRM-free files. It cannot process comics and manga purchased from
Kindle, Kobo, and other vendors unless it is explicitly advertised as DRM-free.

Comicpress takes your comic book files (supported formats: CBR, CBZ, PDF) and compresses them to a fraction of the size while also significantly improving the visual quality on ereaders. It does so by applying image processing operations such as contrast stretching, high-quality scaling, quantization, and dithering. These can all be customized or disabled. Comicpress also downscales pages to your specific ereader device’s display resolution, which not only significantly decreases file sizes but also likely improves the perceived quality on the ereader using high-quality default resampling.

https://github.com/user-attachments/assets/926ad4b2-05e5-4c97-9068-8cad95f18e53

## Comparison to Kindle Comic Converter

[KCC](https://github.com/ciromattia/kcc) is an older and popular alternative to Comicpress, but Comicpress has numerous advantages.

- When processing PDF files, the files produced by Comicpress are higher quality than those produced by KCC. KCC always renders PDFs at a pixel density dependent on the target and page heights, which usually results in lower pixel densities. Comicpress defaults to 300 PPI and allows you to choose a higher pixel density for better quality.
- Comicpress supports a wider variety of image formats including AVIF, JPEG XL, and WebP for the pages in the EPUB/CBZ output files, in addition to the widely used PNG and JPEG formats.
- Comicpress uses a higher-quality image resampler by default (Magic Kernel Sharp 2021). KCC’s resampling is not customizable: it uses Lanczos for downscaling and bicubic interpolation for upscaling, resulting in worse quality than Comicpress.
- Comicpress’s image processing operations are more customizable than in KCC. Scaling, quantization, and dithering can easily be toggled and adjusted.
- Comicpress generally has an easier-to-use user interface than KCC.
- There are a few features present in KCC that are not in Comicpress, such as automatically cropping out margins. I plan to eventually add these in the future.

## Installation

## Linux

Comicpress is available on Flathub:

1. Go to https://flathub.org/en/apps/io.github.amarz45.Comicpress.
1. Click _Install_.

Alternatively, you can install from the command-line:

```console
$ flatpak install flathub io.github.amarz45.Comicpress
```

Ensure that you have [set up Flathub](https://flathub.org/en/setup) for your
distribution before installing.

## Windows

Installation for Windows is coming soon.

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

`comicpress` will then be located in the `build` directory.

### Windows

Comicpress is built on Windows using [MSYS2](https://www.msys2.org) with the
UCRT64 environment.

#### Installing dependencies

##### MSYS2

Download the installer from https://www.msys2.org and run it. Open **MSYS2
UCRT64** from the Start menu (not _MSYS2 MSYS_ or _MSYS2 MINGW64_) and update:

```console
$ pacman -Syu
```

The terminal may close partway through. Reopen it and rerun `pacman -Syu` until
it reports there is nothing to do. Then install the dependencies:

```console
$ pacman -S --needed git curl tar \
    mingw-w64-ucrt-x86_64-toolchain \
    mingw-w64-ucrt-x86_64-meson \
    mingw-w64-ucrt-x86_64-ninja \
    mingw-w64-ucrt-x86_64-pkgconf \
    mingw-w64-ucrt-x86_64-libvips \
    mingw-w64-ucrt-x86_64-libheif \
    mingw-w64-ucrt-x86_64-libjxl \
    mingw-w64-ucrt-x86_64-qt6-base \
    mingw-w64-ucrt-x86_64-libarchive
```

##### Prerequisites

Go to https://github.com/bblanchon/pdfium-binaries/releases/latest. Download the
file that matches your CPU. For example, on x86-64 this would be
`pdfium-win-x64.tgz`. `cd` into the directory containing the tarball. Extract
and install:

```console
$ mkdir pdfium
$ tar -xzf pdfium-win-x64.tgz -C pdfium
$ cd pdfium
$ cp -r include/* /ucrt64/include/
$ cp bin/pdfium.dll /ucrt64/bin/
$ cp lib/pdfium.dll.lib /ucrt64/lib/libpdfium.dll.a
```

##### Compiling

```console
$ git clone --depth=1 https://github.com/amarz45/comicpress
$ cd comicpress
$ meson setup build --buildtype=release
$ meson compile -C build
```

`comicpress.exe` will then be located in the `build` directory.

##### Bundling

To run Comicpress outside MSYS2, collect it and its libraries into one folder:

```console
$ dist=~/comicpress-dist
$ mkdir -p "$dist/lib"
$ cp build/comicpress.exe "$dist"
$ cp -r /ucrt64/lib/vips-modules-* "$dist/lib/"
$ rm "$dist"/lib/vips-modules-*/vips-{magick,openslide,poppler}.dll
$ windeployqt.exe "$dist/comicpress.exe"
$ for f in "$dist/comicpress.exe" "$dist"/lib/vips-modules-*/*.dll; do
      ldd "$f" | awk '/\/ucrt64\/bin\//{print $3}'
  done | sort -u | xargs -r cp -n -t "$dist"
```

## Copyright

Copyright (C) 2026 Amar Al-Zubaidi.

Comicpress is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version. See [`Licence.txt`](Licence.txt) for the full text.

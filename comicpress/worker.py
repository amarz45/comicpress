from PyQt6 import QtCore

class ProcessThread(QtCore.QThread):
    total_pages_signal = QtCore.pyqtSignal(int)
    log_signal = QtCore.pyqtSignal(str)
    done_signal = QtCore.pyqtSignal()
    progress_signal = QtCore.pyqtSignal(int)

    def __init__(
        self, input_paths, output_root, dpi, display, resample, bit_depth,
        dither, stretch_contrast, img_format, num_workers, webp_method,
        png_compression_level
    ):
        super().__init__()
        self.input_paths = input_paths
        self.output_root = output_root
        self.dpi = dpi
        self.display = display
        self.resample = resample
        self.img_format = img_format
        self.bit_depth = bit_depth
        self.dither = dither
        self.stretch_contrast = stretch_contrast
        self.num_workers = num_workers
        self.webp_method = webp_method
        self.png_compression_level = png_compression_level

    def run(self):
        import os
        import pymupdf
        import rarfile
        import zipfile
        from concurrent.futures import ProcessPoolExecutor, as_completed
        from pathlib import Path
        from .processing import process_task

        output_root = Path(self.output_root)
        output_root.mkdir(parents = True, exist_ok = True)

        tasks = []
        output_dirs = {}

        for path in self.input_paths:
            ext = os.path.splitext(path)[1].lower()
            base_name = os.path.splitext(os.path.basename(path))[0]
            output_dir = output_root / base_name
            output_dir.mkdir(exist_ok = True)
            output_dirs[path] = output_dir

            if ext == ".pdf":
                try:
                    doc = pymupdf.open(path)
                    for i in range(len(doc)):
                        tasks.append(("pdf", path, i, output_dir))
                    doc.close()
                except Exception as e:
                    self.log_signal.emit(f"Error reading PDF {path}: {e}")
            elif ext == ".cbz":
                self.process_archive(
                    tasks, "cbz", path, output_dir, zipfile.ZipFile
                )
            elif ext == ".cbr":
                self.process_archive(
                    tasks, "cbr", path, output_dir, rarfile.RarFile
                )

        total_tasks = len(tasks)
        self.total_pages_signal.emit(total_tasks)
        self.log_signal.emit(f"Queued {total_tasks} tasks...")
        completed = 0

        with ProcessPoolExecutor(max_workers = self.num_workers) as executor:
            futures = [
                executor.submit(
                    process_task, task, self.dpi, self.display, self.resample,
                    self.bit_depth, self.dither, self.stretch_contrast,
                    self.img_format, self.webp_method,
                    self.png_compression_level
                )
                for task in tasks
            ]

            completed = 0
            for fut in as_completed(futures):
                msg = fut.result()
                if msg:
                    self.log_signal.emit(msg)
                completed += 1
                self.progress_signal.emit(completed)

        for _, temp_dir in output_dirs.items():
            cbz_name = output_root / f"{temp_dir.name}.cbz"
            with zipfile.ZipFile(
                cbz_name, "w", compression = zipfile.ZIP_STORED
            ) \
            as zipf:
                # Recursively find all files in the temporary directory
                all_files = sorted([p for p in temp_dir.rglob("*") if p.is_file()])
                for img_file in all_files:
                    # Create an archive name that is relative to the temp directory
                    arcname = img_file.relative_to(temp_dir)
                    zipf.write(img_file, arcname=arcname)
            self.log_signal.emit(f"Created CBZ: {cbz_name}")

        self.done_signal.emit()

    def process_archive(self, tasks, file_ext, path, output_dir, opener):
        try:
            with opener(path, "r") as archive:
                img_files = sorted(
                    f for f in archive.namelist()
                    if f.lower().endswith((
                        ".avif", ".bmp", ".heic", ".heif", ".jp2", ".jpg",
                        ".jpeg", ".jxl", ".png", ".ppm", ".tiff", ".webp"
                    ))
                )
                for i, name in enumerate(img_files):
                    tasks.append((file_ext, path, (i, name), output_dir))
        except Exception as e:
            self.log_signal.emit(
                f"Error reading {file_ext.upper()} {path}: {e}."
            )

import os
import zipfile
from pathlib import Path
from concurrent.futures import ProcessPoolExecutor
from PyQt6 import QtCore
import rarfile
import pymupdf
from .processing import process_task

class ProcessThread(QtCore.QThread):
    total_pages_signal = QtCore.pyqtSignal(int)
    log_signal = QtCore.pyqtSignal(str)
    done_signal = QtCore.pyqtSignal()
    progress_signal = QtCore.pyqtSignal(int)

    def __init__(
        self, input_paths, output_root, dpi, display, resample, img_format,
        num_workers, webp_method, png_compression_level
    ):
        super().__init__()
        self.input_paths = input_paths
        self.output_root = output_root
        self.dpi = dpi
        self.display = display
        self.resample = resample
        self.img_format = img_format
        self.num_workers = num_workers
        self.webp_method = webp_method
        self.png_compression_level = png_compression_level

    def run(self):
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

        from concurrent.futures import as_completed

        with ProcessPoolExecutor(max_workers = self.num_workers) as executor:
            futures = [
                executor.submit(
                    process_task, task, self.dpi, self.display, self.resample,
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
                for img_file in sorted(temp_dir.iterdir()):
                    zipf.write(img_file, arcname = img_file.name)
            self.log_signal.emit(f"Created CBZ: {cbz_name}")

        self.done_signal.emit()

    def process_archive(self, tasks, file_ext, path, output_dir, opener):
        try:
            with opener(path, "r") as archive:
                img_files = sorted(
                    f for f in archive.namelist()
                    if f.lower().endswith(
                        ".jpg", ".jpeg", ".png", ".webp", ".bmp", ".tiff"
                    )
                )
                for i, name in enumerate(img_files):
                    tasks.append((file_ext, path, (i, name), output_dir))
        except Exception as e:
            self.log_signal.emit(
                f"Error reading {file_ext.upper()} {path}: {e}."
            )

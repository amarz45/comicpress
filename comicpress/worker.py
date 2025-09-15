from PySide6 import QtCore
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from pathlib import Path
    from rarfile import RarFile
    from zipfile import ZipFile
    from .config import Config

class ProcessThread(QtCore.QThread):
    total_pages_signal = QtCore.Signal(int)
    log_signal = QtCore.Signal(str)
    done_signal = QtCore.Signal()
    progress_signal = QtCore.Signal(int)

    def __init__(
        self,
        input_paths: list[str],
        output_root: str,
        num_workers: int,
        config: "Config"
    ):
        super().__init__()
        self.input_paths = input_paths
        self.output_root = output_root
        self.config = config
        self.num_workers = num_workers
        self._is_running = True

    def stop(self):
        self._is_running = False

    def run(self):
        import os
        import pymupdf
        import rarfile
        import zipfile
        from concurrent.futures import ProcessPoolExecutor, as_completed
        from pathlib import Path
        from shutil import rmtree
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
                executor.submit(process_task, task, self.config)
                for task in tasks
            ]

            completed = 0
            for fut in as_completed(futures):
                if not self._is_running:
                    for f in futures:
                        f.cancel()
                    break

                msg = fut.result()
                if msg:
                    self.log_signal.emit(msg)
                completed += 1
                self.progress_signal.emit(completed)

        if not self._is_running:
            return

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
            rmtree(temp_dir)

        self.done_signal.emit()

    def process_archive(
        self,
        tasks: "list[tuple[str, str, tuple[int, str], Path]]",
        file_ext: str,
        path: str,
        output_dir: "Path",
        opener: "type[ZipFile] | type[RarFile]"
    ):
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

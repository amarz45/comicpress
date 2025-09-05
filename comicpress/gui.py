from PyQt6 import QtWidgets

class App(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.worker = None
        self.thread = None
        self.total_pages = 0
        self.processed_pages = 0
        self.start_time = None
        self.setWindowTitle("Comicpress")
        self.current_device = (None, "Custom")

        from PyQt6 import QtCore
        from collections import deque

        # Timer
        self.timer = QtCore.QTimer(self)
        self.timer.timeout.connect(self.update_time_labels)
        self.last_eta_now_time = None
        self.images_since_last_eta_now = 0
        self.last_progress_value = 0
        self.eta_now_intervals = deque(maxlen = 5)

        # Central widget
        self.central_widget = QtWidgets.QWidget()
        self.setCentralWidget(self.central_widget)
        self.main_layout = QtWidgets.QVBoxLayout(self.central_widget)

        self.setup_ui()
        self.connect_signals()
        self.on_device_changed()
        self.toggle_scaling_inputs(self.enable_scaling_check.checkState())
        self.toggle_quantization(self.enable_quantization_check.checkState())
        self.on_format_changed()

    def setup_ui(self):
        from os import getcwd, cpu_count

        # Input and output
        io_group = QtWidgets.QGroupBox()
        io_layout = QtWidgets.QVBoxLayout(io_group)
        self.file_list = QtWidgets.QListWidget()

        # Create file buttons
        file_buttons_layout = QtWidgets.QHBoxLayout()
        self.add_files_button = QtWidgets.QPushButton("Add files")
        self.remove_file_button = QtWidgets.QPushButton("Remove selected")
        self.remove_file_button.setEnabled(False)
        self.clear_files_button = QtWidgets.QPushButton("Clear all")
        self.clear_files_button.setEnabled(False)

        # Add file buttons
        file_buttons_layout.addWidget(self.add_files_button)
        file_buttons_layout.addWidget(self.remove_file_button)
        file_buttons_layout.addWidget(self.clear_files_button)

        # Add input widgets.
        io_layout.addWidget(QtWidgets.QLabel("Input files"))
        io_layout.addWidget(self.file_list)
        io_layout.addLayout(file_buttons_layout)

        # Create output buttons.
        output_layout = QtWidgets.QHBoxLayout()
        self.output_dir_edit = QtWidgets.QLineEdit(getcwd())
        self.browse_output_button = QtWidgets.QPushButton("Browse")

        # Add output buttons.
        output_layout.addWidget(QtWidgets.QLabel("Output folder"))
        output_layout.addWidget(self.output_dir_edit)
        output_layout.addWidget(self.browse_output_button)

        io_layout.addLayout(output_layout)
        self.main_layout.addWidget(io_group)

        # Settings
        settings_group = QtWidgets.QGroupBox("Processing parameters")
        settings_layout = QtWidgets.QFormLayout(settings_group)

        # Pixel density
        self.density_spin = QtWidgets.QSpinBox()
        self.density_spin.setRange(300, 2 ** 31 - 1)
        self.density_spin.setValue(1200)
        self.density_spin.setSingleStep(300)
        settings_layout.addRow("PDF pixel density (PPI)", self.density_spin)

        # Stretch contrast
        contrast_widget = QtWidgets.QWidget()
        contrast_layout = QtWidgets.QHBoxLayout(contrast_widget)
        contrast_layout.setContentsMargins(0, 0, 0, 0)
        contrast_layout.setSpacing(20)

        # Enable contrast stretching.
        self.enable_contrast_check = QtWidgets.QCheckBox("Stretch contrast")
        self.enable_contrast_check.setChecked(True)
        contrast_layout.addWidget(self.enable_contrast_check)
        settings_layout.addRow(contrast_widget)

        # Display presets
        self.display_button = QtWidgets.QPushButton(self.current_device[1])
        display_menu = QtWidgets.QMenu(self)

        from .displays import DISPLAYS
        custom_action = display_menu.addAction("Custom")
        custom_action.triggered.connect(lambda: self.set_device(None, "Custom"))
        display_menu.addSeparator()

        # Add brand submenus.
        for brand, models in DISPLAYS.items():
            if isinstance(models, dict):
                brand_menu = display_menu.addMenu(brand)
                for model_name in models.keys():
                    model_action = brand_menu.addAction(model_name)
                    # Use a lambda to capture the correct model_name.
                    model_action.triggered.connect(
                        lambda checked = False, b = brand, m = model_name: self.set_device(b, m)
                    )

        self.display_button.setMenu(display_menu)
        settings_layout.addRow("Display preset", self.display_button)

        # Image scaling
        scaling_widget = QtWidgets.QWidget()
        scaling_layout = QtWidgets.QHBoxLayout(scaling_widget)
        scaling_layout.setContentsMargins(0, 0, 0, 0)
        scaling_layout.setSpacing(20)

        # Enable scaling checkbox
        self.enable_scaling_check = QtWidgets.QCheckBox("Scale images")
        scaling_layout.addWidget(self.enable_scaling_check)

        # Width label + spin
        self.width_spin = QtWidgets.QSpinBox()
        self.width_spin.setRange(100, 4000)
        width_container = QtWidgets.QWidget()
        width_layout = QtWidgets.QHBoxLayout(width_container)
        width_layout.setContentsMargins(0, 0, 0, 0)
        width_layout.setSpacing(4)
        width_layout.addWidget(QtWidgets.QLabel("Width"))
        width_layout.addWidget(self.width_spin)
        scaling_layout.addWidget(width_container)

        # Height label + spin
        self.height_spin = QtWidgets.QSpinBox()
        self.height_spin.setRange(100, 4000)
        height_container = QtWidgets.QWidget()
        height_layout = QtWidgets.QHBoxLayout(height_container)
        height_layout.setContentsMargins(0, 0, 0, 0)
        height_layout.setSpacing(4)
        height_layout.addWidget(QtWidgets.QLabel("Height"))
        height_layout.addWidget(self.height_spin)
        scaling_layout.addWidget(height_container)

        # Create resampling filter widgets.
        filter_widget = QtWidgets.QWidget()
        filter_layout = QtWidgets.QHBoxLayout(filter_widget)
        filter_layout.setContentsMargins(0, 0, 0, 0)
        filter_layout.setSpacing(4)
        self.filter_combo = QtWidgets.QComboBox()
        self.filter_combo.addItems([
            "Bicubic interpolation", "Bilinear interpolation", "Lanczos 2",
            "Lanczos 3", "Magic Kernel Sharp 2013", "Magic Kernel Sharp 2021",
            "Mitchell", "Nearest neighbour"
        ])
        self.filter_combo.setCurrentText("Magic Kernel Sharp 2021")

        # Add resampling filter widgets.
        filter_layout.addWidget(QtWidgets.QLabel("Resampling filter"))
        filter_layout.addWidget(self.filter_combo)
        scaling_layout.addWidget(filter_widget)

        # Stretch at the end to align left neatly.
        scaling_layout.addStretch()

        # Add to form layout
        settings_layout.addRow(scaling_widget)

        # Quantization
        quantization_widget = QtWidgets.QWidget()
        quantization_layout = QtWidgets.QHBoxLayout(quantization_widget)
        quantization_layout.setContentsMargins(0, 0, 0, 0)
        quantization_layout.setSpacing(20)

        # Enable quantization
        self.enable_quantization_check = QtWidgets.QCheckBox("Quantize images")
        self.enable_quantization_check.setChecked(True)
        quantization_layout.addWidget(self.enable_quantization_check)

        # Colours label + spin
        self.colours_combo = QtWidgets.QComboBox()
        self.colours_combo.addItems(["1", "2", "4", "8", "16"])
        self.colours_combo.setCurrentText("4")
        colours_container = QtWidgets.QWidget()
        colours_layout = QtWidgets.QHBoxLayout(colours_container)
        colours_layout.setContentsMargins(0, 0, 0, 0)
        colours_layout.setSpacing(4)
        colours_layout.addWidget(QtWidgets.QLabel("Bit depth"))
        colours_layout.addWidget(self.colours_combo)
        quantization_layout.addWidget(colours_container)

        # Dither
        self.dither_spin = QtWidgets.QDoubleSpinBox()
        self.dither_spin.setRange(0.0, 1.0)
        self.dither_spin.setSingleStep(0.1)
        self.dither_spin.setValue(1.0)
        dither_container = QtWidgets.QWidget()
        dither_layout = QtWidgets.QHBoxLayout(dither_container)
        dither_layout.setContentsMargins(0, 0, 0, 0)
        dither_layout.setSpacing(4)
        dither_layout.addWidget(QtWidgets.QLabel("Dither"))
        dither_layout.addWidget(self.dither_spin)
        quantization_layout.addWidget(dither_container)

        # Stretch at the end to align left neatly.
        quantization_layout.addStretch()

        # Add to form layout
        settings_layout.addRow(quantization_widget)

        # Image format
        self.img_format_combo = QtWidgets.QComboBox()
        self.img_format_combo.addItems([
            "AVIF", "JPEG", "JPEG XL", "PNG", "WebP"
        ])
        self.img_format_combo.setCurrentText("PNG")
        settings_layout.addRow("Image format", self.img_format_combo)

        # WebP-specific options
        self.webp_method_label = QtWidgets.QLabel("Compression effort")
        self.webp_method_spin = QtWidgets.QSpinBox()
        self.webp_method_spin.setRange(0, 6)
        self.webp_method_spin.setValue(4)
        settings_layout.addRow(self.webp_method_label, self.webp_method_spin)

        # PNG-specific options
        self.png_compression_level_label = QtWidgets.QLabel("Compression effort")
        self.png_compression_level_spin = QtWidgets.QSpinBox()
        self.png_compression_level_spin.setRange(0, 9)
        self.png_compression_level_spin.setValue(9)
        settings_layout.addRow(
            self.png_compression_level_label, self.png_compression_level_spin
        )

        # Parallel jobs
        self.jobs_spin = QtWidgets.QSpinBox()
        num_cpus = cpu_count() or 1
        self.jobs_spin.setRange(1, num_cpus)
        self.jobs_spin.setValue(num_cpus)
        settings_layout.addRow("Parallel jobs", self.jobs_spin)

        # Todo: add memory limiter widget.

        self.main_layout.addWidget(settings_group)

        # Logging
        log_group = QtWidgets.QGroupBox()
        log_layout = QtWidgets.QVBoxLayout(log_group)

        # Progress bar
        self.progress_bar = QtWidgets.QProgressBar()
        self.progress_bar.setValue(0)
        self.progress_bar.setTextVisible(True)
        self.progress_bar.setFormat("%p %")
        log_layout.addWidget(self.progress_bar)

        # Time elapsed and ETA
        time_layout = QtWidgets.QHBoxLayout()
        self.elapsed_label = QtWidgets.QLabel("Elapsed: –")
        self.eta_label = QtWidgets.QLabel("ETA (overall): –")
        self.eta_now_label = QtWidgets.QLabel("ETA (recent): –")
        time_layout.addWidget(self.elapsed_label)
        time_layout.addWidget(self.eta_label)
        time_layout.addWidget(self.eta_now_label)
        log_layout.addLayout(time_layout)

        # Log output
        self.log_output = QtWidgets.QTextEdit()
        self.log_output.setReadOnly(True)
        self.log_output.setFontFamily("monospace")
        log_layout.addWidget(self.log_output)

        # Create action widgets.
        action_layout = QtWidgets.QHBoxLayout()
        self.start_button = QtWidgets.QPushButton("Start")
        self.start_button.setEnabled(False)
        self.cancel_button = QtWidgets.QPushButton("Cancel")
        self.cancel_button.setEnabled(False)

        # Add action widgets.
        action_layout.addWidget(self.start_button)
        action_layout.addWidget(self.cancel_button)
        log_layout.addLayout(action_layout)

        self.main_layout.addWidget(log_group)

    def connect_signals(self):
        self.add_files_button.clicked.connect(self.add_files)
        self.remove_file_button.clicked.connect(self.remove_file)
        self.clear_files_button.clicked.connect(self.file_list.clear)
        self.browse_output_button.clicked.connect(self.browse_output_dir)
        self.img_format_combo.currentTextChanged.connect(self.on_format_changed)
        self.start_button.clicked.connect(self.start_conversion)
        self.clear_files_button.clicked.connect(self.update_start_button_state)
        self.cancel_button.clicked.connect(self.cancel_conversion)
        self.enable_scaling_check.stateChanged.connect(
            self.toggle_scaling_inputs
        )
        self.enable_quantization_check.stateChanged.connect(
            self.toggle_quantization
        )
        self.file_list.itemSelectionChanged.connect(lambda:
            self.remove_file_button.setEnabled(
                bool(self.file_list.selectedItems())
            )
        )
        self.file_list.model().rowsInserted.connect(lambda: self.update_start_button_state())
        self.file_list.model().rowsRemoved.connect(lambda: self.update_start_button_state())
        #self.enable_mem_limit_check.stateChanged.connect(self.toggle_mem_limit_inputs)

    def update_start_button_state(self):
        self.start_button.setEnabled(self.file_list.count() > 0)
        self.clear_files_button.setEnabled(self.file_list.count() > 0)

    def set_progress_max(self, total_pages):
        self.progress_bar.setMaximum(total_pages)
        self.progress_bar.setFormat("%p % (%v / %m pages)")

    def toggle_scaling_inputs(self, state):
        from PyQt6 import QtCore
        enabled = (state == QtCore.Qt.CheckState.Checked.value)
        self.width_spin.setEnabled(enabled)
        self.height_spin.setEnabled(enabled)
        self.filter_combo.setEnabled(enabled)

    def toggle_quantization(self, state):
        from PyQt6 import QtCore
        # Handle both int (from signal) and CheckState (from checkState()).
        if isinstance(state, QtCore.Qt.CheckState):
            enabled = (state == QtCore.Qt.CheckState.Checked)
        else:
            enabled = (state == QtCore.Qt.CheckState.Checked.value)
        self.colours_combo.setEnabled(enabled)
        self.dither_spin.setEnabled(enabled)

    def toggle_filter_inputs(self, state):
        from PyQt6 import QtCore
        enabled = (state == QtCore.Qt.CheckState.Checked.value)
        self.filter_combo.setEnabled(enabled)

    def set_device(self, brand, name):
        """Updates the current device and triggers a UI refresh."""
        self.current_device  = (brand, name)
        if brand:
            self.display_button.setText(f"{brand} {name}")
        else:
            self.display_button.setText(name)
        self.on_device_changed()

    def on_device_changed(self):
        brand, name = self.current_device
        if brand == None:
            self.enable_scaling_check.setEnabled(True)
            self.enable_scaling_check.setChecked(False)
            self.width_spin.setEnabled(True)
            self.height_spin.setEnabled(True)
        else:
            from .displays import DISPLAYS
            specs = DISPLAYS[brand][name]
            self.width_spin.setValue(specs.width)
            self.height_spin.setValue(specs.height)
            self.enable_scaling_check.setChecked(True)
            self.enable_scaling_check.setEnabled(False)
            self.width_spin.setEnabled(False)
            self.height_spin.setEnabled(False)

    def on_format_changed(self):
        selected_format = self.img_format_combo.currentText()

        is_webp = selected_format == "WebP"
        self.webp_method_label.setVisible(is_webp)
        self.webp_method_spin.setVisible(is_webp)

        is_png = selected_format == "PNG"
        self.png_compression_level_label.setVisible(is_png)
        self.png_compression_level_spin.setVisible(is_png)

    def add_files(self):
        import os

        files, _ = QtWidgets.QFileDialog.getOpenFileNames(
            self, "Select input files", "",
            "Supported files (*.pdf *.cbz *.cbr)"
        )
        if not files:
            return

        from PyQt6 import QtCore
        existing_paths = [
            self.file_list.item(i).data(QtCore.Qt.ItemDataRole.UserRole)
            for i in range(self.file_list.count())
        ]

        all_paths = existing_paths + files
        base_names = [os.path.basename(p) for p in all_paths]

        for file in files:
            if file in existing_paths:
                continue
            item = QtWidgets.QListWidgetItem(os.path.basename(file))
            item.setData(QtCore.Qt.ItemDataRole.UserRole, file)
            self.file_list.addItem(item)

        for i in range(self.file_list.count()):
            item = self.file_list.item(i)
            path = item.data(QtCore.Qt.ItemDataRole.UserRole)
            base_name = os.path.basename(path)
            if base_names.count(base_name) > 1:
                item.setText(path)

        self.update_start_button_state()

    def remove_file(self):
        for item in self.file_list.selectedItems():
            self.file_list.takeItem(self.file_list.row(item))
        self.update_start_button_state()

    def browse_output_dir(self):
        directory = QtWidgets.QFileDialog.getExistingDirectory(
            self, "Select output folder"
        )
        if directory:
            self.output_dir_edit.setText(directory)

    def start_conversion(self):
        if self.file_list.count() == 0:
            QtWidgets.QMessageBox.warning(
                self, "Input required", "Please add at least one input file."
            )
            return

        from PyQt6 import QtCore
        input_paths = [
            self.file_list.item(i).data(QtCore.Qt.ItemDataRole.UserRole)
            for i in range(self.file_list.count())
        ]

        output_root = self.output_dir_edit.text()

        stretch_contrast = self.enable_contrast_check.isChecked()

        if self.enable_scaling_check.isChecked():
            from .displays import Display, DISPLAYS
            display = Display(
                self.width_spin.value(), self.height_spin.value(),
                DISPLAYS[self.current_device[0]][self.current_device[1]].colour
            )
        else:
            display = None

        if self.enable_quantization_check.isChecked():
            bit_depth = int(self.colours_combo.currentText())
            dither = self.dither_spin.value()
        else:
            bit_depth = dither = None

        filter_str = self.filter_combo.currentText()
        from pyvips.enums import Kernel

        if filter_str == "Bicubic interpolation":
            resample = Kernel.CUBIC
        elif filter_str == "Bilinear interpolation":
            resample = Kernel.LINEAR
        elif filter_str == "Lanczos 2":
            resample = Kernel.LANCZOS2
        elif filter_str == "Lanczos 3":
            resample = Kernel.LANCZOS3
        elif filter_str == "Magic Kernel Sharp 2013":
            resample = Kernel.MKS2013
        elif filter_str == "Magic Kernel Sharp 2021":
            resample = Kernel.MKS2021
        elif filter_str == "Mitchell":
            resample = Kernel.MITCHELL
        else:
            resample = Kernel.NEAREST

        dpi = self.density_spin.value()
        img_format = self.img_format_combo.currentText()
        num_workers = self.jobs_spin.value()
        webp_method = self.webp_method_spin.value()
        png_compression_level = self.png_compression_level_spin.value()

        self.log_output.append("Starting processing...")
        self.start_button.setEnabled(False)
        self.cancel_button.setEnabled(True)

        from .worker import ProcessThread

        self.thread = ProcessThread(
            input_paths, output_root, dpi, display, resample, bit_depth,
            dither, stretch_contrast, img_format, num_workers, webp_method,
            png_compression_level
        )

        self.thread.log_signal.connect(self.log_output.append)
        self.thread.done_signal.connect(
            lambda: self.log_output.append("Processing complete")
        )
        self.thread.progress_signal.connect(self.update_progress)
        self.thread.total_pages_signal.connect(self.set_progress_max)
        self.thread.finished.connect(lambda: self.start_button.setEnabled(True))
        self.thread.finished.connect(
            lambda: self.cancel_button.setEnabled(False)
        )
        self.thread.finished.connect(self.timer.stop)

        # Timer
        from time import time
        self.start_time = self.last_eta_now_time = time()
        self.images_since_last_eta_now = 0
        self.last_progress_value = 0
        self.elapsed_label.setText("Elapsed: –")
        self.eta_label.setText("ETA (overall): –")
        self.eta_now_label.setText("ETA (recent): –")
        self.timer.start(1000)

        self.thread.start()

    def update_progress(self, value: int):
        self.progress_bar.setValue(value)

        delta = value - self.last_progress_value
        if delta > 0:
            self.images_since_last_eta_now += delta
        self.last_progress_value = value

    def update_time_labels(self):
        if not self.start_time:
            return

        from time import time
        elapsed = time() - self.start_time
        self.elapsed_label.setText(f"Elapsed: {time_to_str(elapsed)}")

        # Overall ETA (average since start)
        value = self.progress_bar.value()
        if value > 0:
            total = self.progress_bar.maximum()
            per_unit = elapsed / value
            remaining = (total - value) * per_unit
            self.eta_label.setText(f"ETA (overall): {time_to_str(remaining)}")

        # ETA now (short-term estimate)
        now = time()
        if now - self.last_eta_now_time >= 1.0 and self.images_since_last_eta_now > 0:
            interval = now - self.last_eta_now_time
            speed = self.images_since_last_eta_now / interval  # images/sec
            total_remaining = self.progress_bar.maximum() - value
            remaining = total_remaining / speed
            eta_now_seconds = remaining

            # Add to moving average window.
            self.eta_now_intervals.append(eta_now_seconds)

            # Reset snapshot.
            self.last_eta_now_time = now
            self.images_since_last_eta_now = 0

        if self.eta_now_intervals:
            avg_eta = sum(self.eta_now_intervals) / len(self.eta_now_intervals)
            self.eta_now_label.setText(f"ETA (recent): {time_to_str(avg_eta)}")

    def cancel_conversion(self):
        if self.thread:
            self.thread.terminate()
            self.thread.wait()
        self.start_button.setEnabled(True)
        self.cancel_button.setEnabled(False)
        self.log_output.append("Cancelled")
        self.timer.stop()

def time_to_str(seconds):
    seconds = int(seconds)
    h, rem = divmod(seconds, 60 * 60)
    m, s = divmod(rem, 60)
    return (
        f"{s} s" if seconds < 60 else
        f"{m} min {s:02} s" if seconds < 60 * 60 else
        f"{h} h {m:02} min {s:02} s"
    )

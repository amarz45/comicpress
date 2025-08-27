import os
from PyQt6 import QtWidgets, QtCore
from PIL import Image
from .displays import Display, DISPLAYS
from .worker import ProcessThread

class App(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.worker = None
        self.thread = None
        self.total_pages = 0
        self.processed_pages = 0
        self.start_time = None
        self.setWindowTitle("Comicpress")

        # Central widget
        self.central_widget = QtWidgets.QWidget()
        self.setCentralWidget(self.central_widget)
        self.main_layout = QtWidgets.QVBoxLayout(self.central_widget)

        self.setup_ui()
        self.connect_signals()
        self.on_device_changed()
        self.toggle_scaling_inputs(self.enable_scaling_check.checkState())
        self.on_format_changed()

    def setup_ui(self):
        # Input and output
        io_group = QtWidgets.QGroupBox("Input and output")
        io_layout = QtWidgets.QVBoxLayout(io_group)
        self.file_list = QtWidgets.QListWidget()

        # Create file buttons
        file_buttons_layout = QtWidgets.QHBoxLayout()
        self.add_files_button = QtWidgets.QPushButton("Add files")
        self.remove_file_button = QtWidgets.QPushButton("Remove selected")
        self.clear_files_button = QtWidgets.QPushButton("Clear all")

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
        self.output_dir_edit = QtWidgets.QLineEdit(os.getcwd())
        self.browse_output_button = QtWidgets.QPushButton("Browse")

        # Add output buttons.
        output_layout.addWidget(QtWidgets.QLabel("Output folder"))
        output_layout.addWidget(self.output_dir_edit)
        output_layout.addWidget(self.browse_output_button)

        io_layout.addLayout(output_layout)
        self.main_layout.addWidget(io_group)

        # Settings
        settings_group = QtWidgets.QGroupBox("Processing settings")
        settings_layout = QtWidgets.QFormLayout(settings_group)

        # Pixel density
        self.density_spin = QtWidgets.QSpinBox()
        self.density_spin.setRange(72, 2400)
        self.density_spin.setValue(1200)
        settings_layout.addRow("PDF pixel density (PPI)", self.density_spin)

        # Display presets
        self.display_combo = QtWidgets.QComboBox()
        self.display_combo.addItems(DISPLAYS.keys())
        settings_layout.addRow("Display preset", self.display_combo)

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
            "Bicubic", "Bilinear", "Box", "Hamming", "Lanczos", "Nearest"
        ])
        self.filter_combo.setCurrentText("Lanczos")

        # Add resampling filter widgets.
        filter_layout.addWidget(QtWidgets.QLabel("Resampling filter"))
        filter_layout.addWidget(self.filter_combo)
        scaling_layout.addWidget(filter_widget)

        # Stretch at the end to align left neatly.
        scaling_layout.addStretch()

        # Add to form layout
        settings_layout.addRow(scaling_widget)

        # Image format
        self.img_format_combo = QtWidgets.QComboBox()
        self.img_format_combo.addItems([
            "AVIF", "JPEG", "JPEG XL", "PNG", "WebP"
        ])
        self.img_format_combo.setCurrentText("WebP")
        settings_layout.addRow("Image format", self.img_format_combo)

        # WebP-specific options
        self.webp_method_label = QtWidgets.QLabel("Compression level")
        self.webp_method_spin = QtWidgets.QSpinBox()
        self.webp_method_spin.setRange(0, 6)
        self.webp_method_spin.setValue(4)
        settings_layout.addRow(self.webp_method_label, self.webp_method_spin)

        # PNG-specific options
        self.png_compression_level_label = QtWidgets.QLabel("Compression level")
        self.png_compression_level_spin = QtWidgets.QSpinBox()
        self.png_compression_level_spin.setRange(0, 9)
        self.png_compression_level_spin.setValue(9)
        settings_layout.addRow(
            self.png_compression_level_label, self.png_compression_level_spin
        )

        # Parallel jobs
        self.jobs_spin = QtWidgets.QSpinBox()
        num_cpus = os.cpu_count() or 1
        self.jobs_spin.setRange(1, num_cpus)
        self.jobs_spin.setValue(num_cpus)
        settings_layout.addRow("Number of parallel jobs", self.jobs_spin)

        # Todo: add memory limiter widget.

        self.main_layout.addWidget(settings_group)

        # Logging
        log_group = QtWidgets.QGroupBox("Process output")
        log_layout = QtWidgets.QVBoxLayout(log_group)

        # Progress bar
        self.progress_bar = QtWidgets.QProgressBar()
        self.progress_bar.setValue(0)
        self.progress_bar.setTextVisible(True)
        log_layout.addWidget(self.progress_bar)

        # Log output
        self.log_output = QtWidgets.QTextEdit()
        self.log_output.setReadOnly(True)
        self.log_output.setFontFamily("monospace")
        log_layout.addWidget(self.log_output)

        # Create action widgets.
        action_layout = QtWidgets.QHBoxLayout()
        self.start_button = QtWidgets.QPushButton("Start conversion")
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
        self.display_combo.currentTextChanged.connect(self.on_device_changed)
        self.img_format_combo.currentTextChanged.connect(self.on_format_changed)
        self.start_button.clicked.connect(self.start_conversion)
        self.cancel_button.clicked.connect(self.cancel_conversion)
        self.enable_scaling_check.stateChanged.connect(
            self.toggle_scaling_inputs
        )
        #self.enable_mem_limit_check.stateChanged.connect(self.toggle_mem_limit_inputs)

    def toggle_scaling_inputs(self, state):
        enabled = (state == QtCore.Qt.CheckState.Checked.value)
        self.width_spin.setEnabled(enabled)
        self.height_spin.setEnabled(enabled)
        self.filter_combo.setEnabled(enabled)

    def toggle_filter_inputs(self, state):
        enabled = (state == QtCore.Qt.CheckState.Checked.value)
        self.filter_combo.setEnabled(enabled)

    def on_device_changed(self):
        device_name = self.display_combo.currentText()
        if device_name == "Custom":
            self.enable_scaling_check.setEnabled(True)
            self.enable_scaling_check.setChecked(False)
        else:
            specs = DISPLAYS[device_name]
            self.width_spin.setValue(specs.width)
            self.height_spin.setValue(specs.height)
            self.enable_scaling_check.setChecked(True)
            self.enable_scaling_check.setEnabled(False)

    def on_format_changed(self):
        selected_format = self.img_format_combo.currentText()

        is_webp = selected_format == "WebP"
        self.webp_method_label.setVisible(is_webp)
        self.webp_method_spin.setVisible(is_webp)

        is_png = selected_format == "PNG"
        self.png_compression_level_label.setVisible(is_png)
        self.png_compression_level_spin.setVisible(is_png)

    def add_files(self):
        files, _ = QtWidgets.QFileDialog.getOpenFileNames(
            self, "Select Input Files", "",
            "Supported Files (*.pdf *.cbz *.cbr)"
        )
        if not files:
            return

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

    def remove_file(self):
        for item in self.file_list.selectedItems():
            self.file_list.takeItem(self.file_list.row(item))

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

        input_paths = [
            self.file_list.item(i).data(QtCore.Qt.ItemDataRole.UserRole)
            for i in range(self.file_list.count())
        ]

        output_root = self.output_dir_edit.text()

        if self.enable_scaling_check.isChecked():
            display = Display(self.width_spin.value(), self.height_spin.value())
        else:
            display = None

        filter_str = self.filter_combo.currentText()
        if filter_str == "Bicubic":
            resample = Image.Resampling.BICUBIC
        elif filter_str == "Bilinear":
            resample = Image.Resampling.BILINEAR
        elif filter_str == "Box":
            resample = Image.Resampling.BOX
        elif filter_str == "Hamming":
            resample = Image.Resampling.HAMMING
        elif filter_str == "Lanczos":
            resample = Image.Resampling.LANCZOS
        else:
            resample = Image.Resampling.NEAREST

        dpi = self.density_spin.value()
        img_format = self.img_format_combo.currentText()
        num_workers = self.jobs_spin.value()
        webp_method = self.webp_method_spin.value()
        png_compression_level = self.png_compression_level_spin.value()

        self.log_output.append("Starting processing...")
        self.start_button.setEnabled(False)
        self.cancel_button.setEnabled(True)

        self.thread = ProcessThread(
            input_paths, output_root, dpi, display, resample, img_format,
            num_workers, webp_method, png_compression_level
        )

        self.thread.log_signal.connect(self.log_output.append)
        self.thread.done_signal.connect(
            lambda: self.log_output.append("Processing complete")
        )
        self.thread.progress_signal.connect(self.progress_bar.setValue)
        self.thread.finished.connect(lambda: self.start_button.setEnabled(True))
        self.thread.finished.connect(
            lambda: self.cancel_button.setEnabled(False)
        )

        self.thread.start()

    def cancel_conversion(self):
        if self.thread:
            self.thread.terminate()
            self.thread.wait()
        self.start_button.setEnabled(True)
        self.cancel_button.setEnabled(False)
        self.log_output.append("Cancelled")

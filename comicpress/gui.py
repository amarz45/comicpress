from PySide6 import QtWidgets
from typing import TYPE_CHECKING
from . import ui_constants

if TYPE_CHECKING:
    from .config import Config
    from .worker import ProcessThread

IMAGE_FORMAT_SETTINGS = {
    "AVIF": {
        "compression_type_configurable": False,
        "compression_effort": {
            "min": 0,
            "max": 9,
            "default": 4
        },
        "quality": {
            "min": 0,
            "max": 100,
            "default": 50
        }
    },
    "JPEG XL": {
        "compression_type_configurable": False,
        "compression_effort": {
            "min": 1,
            "max": 9,
            "default": 7
        },
        "quality": {
            "min": 0,
            "max": 100,
            "default": 100
        },
        "distance": {
            "min": 0,
            "max": 15,
            "default": 0
        },
    },
    "PNG": {
        "compression_type_configurable": False,
        "compression_effort": {
            "min": 0,
            "max": 9,
            "default": 9
        }
    },
    "WebP": {
        "compression_type_configurable": True,
        "compression_effort": {
            "min": 0,
            "max": 6,
            "default": 4
        }
    },
    "JPEG": {
        "compression_type_configurable": False,
        "quality": {
            "min": 0,
            "max": 100,
            "default": 75
        },
    }
}

class App(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()

        self.worker: ProcessThread | None = None
        self.process_thread: ProcessThread | None = None
        self.total_pages = 0
        self.processed_pages = 0
        self.start_time: float | None = None
        self.setWindowTitle(ui_constants.TITLE)
        self.current_device = (None, "Custom")

        from PySide6 import QtCore
        from collections import deque

        # Timer
        self.timer = QtCore.QTimer(self)
        self.timer.timeout.connect(self.update_time_labels)
        self.last_eta_now_time: float | None = None
        self.images_since_last_eta_now = 0
        self.last_progress_value = 0
        self.eta_now_intervals = deque(maxlen = 5)

        # Central widget
        self.central_widget = QtWidgets.QWidget()
        self.setCentralWidget(self.central_widget)

        self.setup_ui()
        self.connect_signals()
        self.on_device_changed()
        self.toggle_scaling_inputs(self.enable_scaling_check.checkState().value)
        self.toggle_quantization(self.enable_quantization_check.checkState().value)
        self.on_format_changed()

    def setup_ui(self):
        container_layout = QtWidgets.QHBoxLayout(self.central_widget)
        content_widget = QtWidgets.QWidget()

        self.main_layout = QtWidgets.QVBoxLayout(content_widget)
        self.main_layout.setContentsMargins(0, 0, 0, 0) # Optional: removes padding

        io_group = self._create_io_group()
        settings_group = self._create_settings_group() # The refactored version
        log_group = self._create_log_group()

        self.main_layout.addWidget(io_group)
        self.main_layout.addWidget(settings_group)
        self.main_layout.addWidget(log_group)
        self.main_layout.addItem(
            QtWidgets.QSpacerItem(1000, 0, QtWidgets.QSizePolicy.Policy.Preferred, QtWidgets.QSizePolicy.Policy.Fixed)
        )

        container_layout.addStretch(1)
        container_layout.addWidget(content_widget)
        container_layout.addStretch(1)

    def _create_io_group(self) -> QtWidgets.QGroupBox:
        from os import getcwd

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
        file_buttons_layout.addStretch(1)

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

        return io_group

    def _create_settings_group(self) -> QtWidgets.QGroupBox:
        settings_group = QtWidgets.QGroupBox("Processing parameters")
        self.settings_layout = QtWidgets.QFormLayout(settings_group)

        self._add_pdf_pixel_density_widget()
        self._add_contrast_widget()
        self._add_display_presets_widget()
        self._add_scaling_widgets()
        self._add_quantization_widget()
        self._add_img_format_widget()
        self._add_img_format_specific_options(self.settings_layout)
        self._add_parallel_jobs_widget()

        return settings_group

    def _create_log_group(self) -> QtWidgets.QGroupBox:
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
        action_layout.addStretch(1)
        action_layout.addWidget(self.start_button)
        action_layout.addWidget(self.cancel_button)
        log_layout.addLayout(action_layout)

        return log_group

    def _add_contrast_widget(self):
        contrast_widget = QtWidgets.QWidget()
        contrast_layout = QtWidgets.QHBoxLayout(contrast_widget)
        contrast_layout.setContentsMargins(0, 0, 0, 0)
        contrast_layout.setSpacing(20)

        self.enable_contrast_check = QtWidgets.QCheckBox(
            ui_constants.CONTRAST_LABEL
        )
        self.enable_contrast_check.setChecked(True)
        contrast_widget = self._create_widget_with_info(
            self.enable_contrast_check, ui_constants.CONTRAST_TOOLTIP
        )
        # We add a stretch to ensure it aligns neatly to the left.
        if (layout := contrast_widget.layout()) is not None:
            layout.addStretch() # type: ignore
        self.settings_layout.addRow(contrast_widget)

    def _add_display_presets_widget(self):
        self.display_button = QtWidgets.QPushButton(self.current_device[1])
        display_menu = QtWidgets.QMenu(self)

        from .displays import DISPLAYS
        custom_action = display_menu.addAction("Custom")
        if custom_action:
            custom_action.triggered.connect(
                lambda: self.set_device(None, "Custom")
            )
        display_menu.addSeparator()

        # Add brand submenus.
        for brand, models in DISPLAYS.items():
            if isinstance(models, dict):
                brand_menu = display_menu.addMenu(brand)
                if not brand_menu:
                    continue
                for model_name in models.keys():
                    model_action = brand_menu.addAction(model_name)
                    if not model_action:
                        continue
                    model_action.triggered.connect(lambda
                        _, b = brand, m = model_name: self.set_device(b, m)
                    )

        self.display_button.setMenu(display_menu)

        hbox = QtWidgets.QHBoxLayout()
        hbox.setContentsMargins(0, 0, 0, 0) # Remove padding
        hbox.addWidget(self.display_button)
        hbox.addStretch(1) # This spacer will absorb all extra horizontal space

        self.settings_layout.addRow("Display preset", hbox)

    def _add_scaling_widgets(self):
        # Enable scaling checkbox
        self.enable_scaling_check = QtWidgets.QCheckBox(
            ui_constants.SCALE_LABEL
        )
        scaling_label_widget = self._create_widget_with_info(
            self.enable_scaling_check, ui_constants.SCALE_TOOLTIP
        )

        if (layout := scaling_label_widget.layout()) is not None:
            layout.addStretch() # type: ignore

        self.settings_layout.addRow(scaling_label_widget)

        #scaling_layout.addWidget(scaling_label_widget)

        self.scaling_options_container = QtWidgets.QWidget()
        scaling_layout = QtWidgets.QHBoxLayout(self.scaling_options_container)
        scaling_layout.setContentsMargins(40, 0, 0, 0)
        scaling_layout.setSpacing(20)

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
        self.filter_combo.addItems(ui_constants.RESAMPLING_FILTERS)
        self.filter_combo.setCurrentText(ui_constants.RESAMPLING_FILTER_DEFAULT)

        # Add resampling filter widgets.
        filter_label_with_info = self._create_widget_with_info(
            QtWidgets.QLabel(ui_constants.RESAMPLING_FILTER_LABEL),
            ui_constants.RESAMPLING_FILTER_TOOLTIP
        )
        filter_layout.addWidget(filter_label_with_info)
        filter_layout.addWidget(self.filter_combo)
        scaling_layout.addWidget(filter_widget)

        # Stretch at the end to align left neatly.
        scaling_layout.addStretch()

        # Add to form layout
        #self.settings_layout.addRow(scaling_widget)
        self.settings_layout.addRow(self.scaling_options_container)

        self.enable_scaling_check.stateChanged.connect(
            self.scaling_options_container.setVisible
        )

        self.scaling_options_container.setVisible(self.enable_scaling_check.isChecked())

    def _add_quantization_widget(self):
        self.enable_quantization_check = QtWidgets.QCheckBox(
            ui_constants.QUANTIZE_LABEL
        )
        self.enable_quantization_check.setChecked(True)
        quantization_label_widget = self._create_widget_with_info(
            self.enable_quantization_check, ui_constants.QUANTIZE_TOOLTIP
        )

        if (layout := quantization_label_widget.layout()) is not None:
            layout.addStretch() # type: ignore

        self.settings_layout.addRow(quantization_label_widget)

        self.quantization_options_container = QtWidgets.QWidget()
        quantization_layout = QtWidgets.QHBoxLayout(self.quantization_options_container)
        quantization_layout.setContentsMargins(40, 0, 0, 0)
        quantization_layout.setSpacing(20)

        # Colours label + spin
        self.colours_combo = QtWidgets.QComboBox()
        self.colours_combo.addItems(["1", "2", "4", "8", "16"])
        self.colours_combo.setCurrentText("4")
        colours_container = QtWidgets.QWidget()
        colours_layout = QtWidgets.QHBoxLayout(colours_container)
        colours_layout.setContentsMargins(0, 0, 0, 0)
        colours_layout.setSpacing(4)

        # Add resampling filter widgets.
        colours_label_with_info = self._create_widget_with_info(
            QtWidgets.QLabel(ui_constants.BIT_DEPTH_LABEL),
            ui_constants.BIT_DEPTH_TOOLTIP
        )
        colours_layout.addWidget(colours_label_with_info)
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

        dither_widget_with_info = self._create_widget_with_info(
            QtWidgets.QLabel(ui_constants.DITHER_LABEL),
            ui_constants.DITHER_TOOLTIP
        )

        dither_layout.addWidget(dither_widget_with_info)
        dither_layout.addWidget(self.dither_spin)
        quantization_layout.addWidget(dither_container)

        # Stretch at the end to align left neatly.
        quantization_layout.addStretch()

        # Add to form layout
        self.settings_layout.addRow(self.quantization_options_container)

        self.enable_quantization_check.stateChanged.connect(
            self.quantization_options_container.setVisible
        )

        self.quantization_options_container.setVisible(self.enable_quantization_check.isChecked())

    def _add_img_format_widget(self):
        self.img_format_combo = QtWidgets.QComboBox()
        self.img_format_combo.addItems(ui_constants.IMG_FORMATS)
        self.img_format_combo.setCurrentText(ui_constants.IMG_FORMAT_DEFAULT)
        img_format_with_info = self._create_widget_with_info(
            QtWidgets.QLabel(ui_constants.IMG_FORMAT_LABEL),
            ui_constants.IMG_FORMAT_TOOLTIP
        )

        hbox = QtWidgets.QHBoxLayout()
        hbox.setContentsMargins(0, 0, 0, 0)
        hbox.addWidget(self.img_format_combo)
        hbox.addStretch(1)

        self.settings_layout.addRow(img_format_with_info, hbox)

    def _add_parallel_jobs_widget(self):
        from os import cpu_count
        self.jobs_spin = QtWidgets.QSpinBox()
        num_cpus = cpu_count() or 1
        self.jobs_spin.setRange(1, num_cpus)
        self.jobs_spin.setValue(num_cpus)
        hbox = QtWidgets.QHBoxLayout()
        hbox.setContentsMargins(0, 0, 0, 0) # Remove padding
        hbox.addWidget(self.jobs_spin)
        hbox.addStretch(1) # This spacer will absorb all extra horizontal space
        self.settings_layout.addRow("Threads", hbox)

    def _add_pdf_pixel_density_widget(self):
        self.density_spin = QtWidgets.QSpinBox()
        self.density_spin.setRange(300, 9999)
        self.density_spin.setValue(1200)
        self.density_spin.setSingleStep(300)
        pdf_label_widget = self._create_widget_with_info(
            QtWidgets.QLabel(ui_constants.PDF_LABEL), ui_constants.PDF_TOOLTIP
        )

        hbox = QtWidgets.QHBoxLayout()
        hbox.setContentsMargins(0, 0, 0, 0) # Remove padding
        hbox.addWidget(self.density_spin)
        hbox.addStretch(1) # This spacer will absorb all extra horizontal space

        self.settings_layout.addRow(pdf_label_widget, hbox)

    def _add_img_format_specific_options(self, layout: QtWidgets.QFormLayout):
        # Compression type
        self.compression_type_label = QtWidgets.QLabel(
            ui_constants.COMPRESSION_LABEL
        )
        self.compression_type_combo = QtWidgets.QComboBox()
        self.compression_type_combo.addItems(ui_constants.COMPRESSION_TYPES)
        self.compression_type_combo.setCurrentText(
            ui_constants.COMPRESSION_DEFAULT
        )
        hbox = QtWidgets.QHBoxLayout()
        hbox.setContentsMargins(0, 0, 0, 0) # Remove padding
        hbox.addWidget(self.compression_type_combo)
        hbox.addStretch(1) # This spacer will absorb all extra horizontal space
        layout.addRow(self.compression_type_label, hbox)

        # Compression effort
        self.compression_effort_label = QtWidgets.QLabel(
            ui_constants.COMPRESSION_EFFORT_LABEL
        )
        self.compression_effort_spin = QtWidgets.QSpinBox()
        hbox = QtWidgets.QHBoxLayout()
        hbox.setContentsMargins(0, 0, 0, 0) # Remove padding
        hbox.addWidget(self.compression_effort_spin)
        hbox.addStretch(1) # This spacer will absorb all extra horizontal space
        layout.addRow(self.compression_effort_label, hbox)

        # Quality/distance
        self.quality_label_container = QtWidgets.QWidget()
        quality_label_layout = QtWidgets.QHBoxLayout(self.quality_label_container)
        quality_label_layout.setContentsMargins(0, 0, 0, 0)
        quality_label_layout.setSpacing(0)

        self.quality_label = QtWidgets.QLabel(ui_constants.QUALITY_LABEL)
        self.jpeg_xl_quality_mode_combo = QtWidgets.QComboBox()
        self.jpeg_xl_quality_mode_combo.addItems(
            ui_constants.JPEG_XL_QUALITY_TYPES
        )

        quality_label_layout.addWidget(self.quality_label)
        quality_label_layout.addWidget(self.jpeg_xl_quality_mode_combo)

        self.quality_spin = QtWidgets.QSpinBox()
        self.quality_spin.setRange(0, 100)
        self.quality_spin.setValue(100)

        # Create a container widget
        self.quality_field_container = QtWidgets.QWidget()
        quality_field_layout = QtWidgets.QHBoxLayout(self.quality_field_container)
        quality_field_layout.setContentsMargins(0, 0, 0, 0)
        quality_field_layout.addWidget(self.quality_spin)
        quality_field_layout.addStretch(1)

        # Add the container widget to the form layout
        layout.addRow(self.quality_label_container, self.quality_field_container)

    def connect_signals(self):
        self.add_files_button.clicked.connect(self.add_files)
        self.remove_file_button.clicked.connect(self.remove_file)
        self.clear_files_button.clicked.connect(self.file_list.clear)
        self.browse_output_button.clicked.connect(self.browse_output_dir)
        self.img_format_combo.currentTextChanged.connect(self.on_format_changed)
        self.start_button.clicked.connect(self._start_conversion)
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
        self.jpeg_xl_quality_mode_combo.currentTextChanged.connect(
            self.on_jpeg_xl_mode_changed
        )
        file_list_model = self.file_list.model()
        if file_list_model:
            file_list_model.rowsInserted.connect(
                lambda: self.update_start_button_state()
            )
            file_list_model.rowsRemoved.connect(
                lambda: self.update_start_button_state()
            )

    def on_jpeg_xl_mode_changed(self, mode: str):
        settings = IMAGE_FORMAT_SETTINGS["JPEG XL"]
        quantize_enabled = self.enable_quantization_check.isChecked()

        if mode == "Distance":
            setting = settings["distance"]
            value = 0 if quantize_enabled else 1
        else:
            setting = settings["quality"]
            value = 100 if quantize_enabled else 75

        self.quality_spin.setRange(setting["min"], setting["max"])
        self.quality_spin.setValue(value)

    def update_start_button_state(self):
        self.start_button.setEnabled(self.file_list.count() > 0)
        self.clear_files_button.setEnabled(self.file_list.count() > 0)

    def set_progress_max(self, total_pages: int):
        self.progress_bar.setMaximum(total_pages)
        self.progress_bar.setFormat("%p % (%v / %m pages)")

    def toggle_scaling_inputs(self, state: int):
        from PySide6 import QtCore
        enabled = state == QtCore.Qt.CheckState.Checked.value
        self.width_spin.setEnabled(enabled)
        self.height_spin.setEnabled(enabled)
        self.filter_combo.setEnabled(enabled)

    def toggle_quantization(self, state: int):
        from PySide6 import QtCore
        enabled = state == QtCore.Qt.CheckState.Checked.value
        self.colours_combo.setEnabled(enabled)
        self.dither_spin.setEnabled(enabled)

        img_format = self.img_format_combo.currentText()
        settings = IMAGE_FORMAT_SETTINGS[img_format]
        compression_type_visible = settings["compression_type_configurable"]

        if compression_type_visible:
            compression_type = "Lossless" if enabled else "Lossy"
            self.compression_type_combo.setCurrentText(compression_type)
        elif img_format == "JPEG XL":
            mode = self.jpeg_xl_quality_mode_combo.currentText()
            if mode == "Distance":
                value = 0 if enabled else 1
            else:
                value = 100 if enabled else 75
            self.quality_spin.setValue(value)
        else:
            return

    def toggle_filter_inputs(self, state: int):
        from PySide6 import QtCore
        enabled = state == QtCore.Qt.CheckState.Checked.value
        self.filter_combo.setEnabled(enabled)

    def set_device(self, brand: str | None, name: str):
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
        img_format = self.img_format_combo.currentText()
        settings = IMAGE_FORMAT_SETTINGS[img_format]

        # Compression type
        compression_type_visible = settings["compression_type_configurable"]
        self.compression_type_label.setVisible(compression_type_visible)
        self.compression_type_combo.setVisible(compression_type_visible)

        # Compression effort
        compression_effort = settings.get("compression_effort")
        compression_effort_is_not_none = compression_effort is not None

        if compression_effort_is_not_none:
            self.compression_effort_spin.setRange(
                compression_effort["min"], compression_effort["max"]
            )
            self.compression_effort_spin.setValue(compression_effort["default"])

        self.compression_effort_label.setVisible(compression_effort_is_not_none)
        self.compression_effort_spin.setVisible(compression_effort_is_not_none)

        # Quality/distance
        quality = settings.get("quality")
        quality_visible = quality is not None

        self.quality_label_container.setVisible(quality_visible)
        self.quality_field_container.setVisible(quality_visible)
        #if (row := self.settings_layout.getWidgetPosition(self.quality_spin)[0]) != -1:
            #self.settings_layout.setRowVisible(row, quality_visible)

        if quality_visible:
            if img_format == "JPEG XL":
                self.quality_label.hide()
                self.jpeg_xl_quality_mode_combo.show()
                self.jpeg_xl_quality_mode_combo.setCurrentText("Distance")
                self.on_jpeg_xl_mode_changed(
                    self.jpeg_xl_quality_mode_combo.currentText()
                )
            else:
                self.jpeg_xl_quality_mode_combo.hide()
                self.quality_label.show()
                self.quality_spin.setRange(quality["min"], quality["max"])
                self.quality_spin.setValue(quality["default"])

        # Default to lossless compression for quantized images.
        quantize_enabled = self.enable_quantization_check.isChecked()
        if compression_type_visible:
            if quantize_enabled:
                compression_type = "Lossless"
            else:
                compression_type = "Lossy"
            self.compression_type_combo.setCurrentText(compression_type)

    def add_files(self):
        import os

        files, _ = QtWidgets.QFileDialog.getOpenFileNames(
            self, "Select input files", "",
            "Supported files (*.pdf *.cbz *.cbr)"
        )
        if not files:
            return

        from PySide6 import QtCore
        existing_paths = []
        for i in range(self.file_list.count()):
            item = self.file_list.item(i)
            if not item:
                continue
            path = item.data(QtCore.Qt.ItemDataRole.UserRole)
            if path:
                existing_paths.append(path)

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
            if not item:
                continue
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

    def _start_conversion(self):
        from .worker import ProcessThread

        if self.file_list.count() == 0:
            QtWidgets.QMessageBox.warning(
                self, "Input required", "Please add at least one input file."
            )
            return

        input_paths = self._get_input_paths()
        output_root = self.output_dir_edit.text()
        num_workers = self.jobs_spin.value()
        config = self._build_config_from_ui()

        self.process_thread = ProcessThread(
            input_paths, output_root, num_workers, config
        )

        self.process_thread.log_signal.connect(self.log_output.append)
        self.process_thread.done_signal.connect(
            lambda: self.log_output.append("Processing complete")
        )
        self.process_thread.progress_signal.connect(self.update_progress)
        self.process_thread.total_pages_signal.connect(self.set_progress_max)
        self.process_thread.finished.connect(lambda: self.start_button.setEnabled(True))
        self.process_thread.finished.connect(
            lambda: self.cancel_button.setEnabled(False)
        )
        self.process_thread.finished.connect(self.timer.stop)

        # Timer
        from time import time
        self.start_time = self.last_eta_now_time = time()
        self.images_since_last_eta_now = 0
        self.last_progress_value = 0
        self.elapsed_label.setText("Elapsed: –")
        self.eta_label.setText("ETA (overall): –")
        self.eta_now_label.setText("ETA (recent): –")
        self.timer.start(1000)

        self.process_thread.start()

    def _get_input_paths(self) -> list[str]:
        from PySide6 import QtCore
        input_paths = []

        for i in range(self.file_list.count()):
            item = self.file_list.item(i)
            if not item:
                continue
            path = item.data(QtCore.Qt.ItemDataRole.UserRole)
            if path:
                input_paths.append(path)

        return input_paths

    def _build_config_from_ui(self) -> "Config":
        from . import config

        stretch_contrast = self.enable_contrast_check.isChecked()

        if self.enable_scaling_check.isChecked():
            from .displays import Display, DISPLAYS
            brand, name = self.current_device
            if brand is not None:
                is_colour = DISPLAYS[brand][name].colour
            else:
                is_colour = False
            display = Display(
                self.width_spin.value(), self.height_spin.value(), is_colour
            )
        else:
            display = None

        if self.enable_quantization_check.isChecked():
            bit_depth = int(self.colours_combo.currentText())
            dither = self.dither_spin.value()
        else:
            bit_depth = dither = None

        filter_str = self.filter_combo.currentText()
        resample = ui_constants.RESAMPLE_FILTER_MAP[filter_str]

        dpi = self.density_spin.value()
        img_format = self.img_format_combo.currentText()

        if IMAGE_FORMAT_SETTINGS.get(img_format):
            compression_or_speed_level = self.compression_effort_spin.value()
        else:
            compression_or_speed_level = 0

        compression_type = config.CompressionType[
            self.compression_type_combo.currentText().upper()
        ]

        jpeg_xl_quality_mode = self.jpeg_xl_quality_mode_combo.currentText()
        if img_format == "JPEG XL":
            quality_type = config.QualityType[jpeg_xl_quality_mode.upper()]
        else:
            quality_type = config.QualityType["QUALITY"]

        quality = self.quality_spin.value()

        self.log_output.append("Starting processing...")
        self.start_button.setEnabled(False)
        self.cancel_button.setEnabled(True)

        return config.Config(
            dpi = dpi,
            display = display,
            resample = resample, # type: ignore
            bit_depth = bit_depth,
            dither = dither,
            stretch_contrast = stretch_contrast,
            img_format = img_format,
            compression_or_speed_level = compression_or_speed_level,
            compression_type = compression_type,
            quality_type = quality_type,
            img_quality = quality
        )

    def update_progress(self, value: int):
        self.progress_bar.setValue(value)

        delta = value - self.last_progress_value
        if delta > 0:
            self.images_since_last_eta_now += delta
        self.last_progress_value = value

    def _create_widget_with_info(
        self,
        main_widget: QtWidgets.QWidget,
        tooltip_text: str
    ) \
    -> QtWidgets.QWidget:
            """Creates a container widget with an info icon on the left."""
            container = QtWidgets.QWidget()
            layout = QtWidgets.QHBoxLayout(container)
            layout.setContentsMargins(0, 0, 0, 0)
            layout.setSpacing(5)

            # Create and configure the info icon label.
            info_icon_label = QtWidgets.QLabel()

            if style := self.style():
                icon = style.standardIcon(
                    QtWidgets.QStyle.StandardPixmap.SP_MessageBoxInformation
                )
                info_icon_label.setPixmap(icon.pixmap(16, 16))
                formatted_tooltip = f"<p>{tooltip_text}</p>"
                info_icon_label.setToolTip(formatted_tooltip)

                # Add the icon first (for left alignment), then the main widget.
                layout.addWidget(info_icon_label)

            layout.addWidget(main_widget)

            return container

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
        if (
            self.last_eta_now_time is not None
            and now - self.last_eta_now_time >= 1.0
            and self.images_since_last_eta_now > 0
        ):
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
        if self.process_thread:
            self.process_thread.stop()
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

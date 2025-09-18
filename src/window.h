#pragma once

#include <atomic>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <string>

#include "qpushbutton.h"

namespace vips {
    class VImage;
}

struct DisplayPreset {
    std::string brand;
    std::string model;
};

class Window : public QMainWindow {
    Q_OBJECT

public:
    DisplayPreset display_preset = DisplayPreset {
        .brand = "Custom",
        .model = "",
    };
    explicit Window(QWidget* parent = nullptr);
    ~Window();

private slots:
    void on_start_button_clicked();
    void handle_log_message(const QString& message);
    void handle_task_finished();

private:
    QWidget* central_widget;
    QVBoxLayout* main_layout;

    // Input and output
    QListWidget* file_list;
    QPushButton* add_files_button;
    QPushButton* remove_selected_button;
    QPushButton* clear_all_button;
    QLineEdit* output_dir_field;
    QPushButton* browse_output_button;

    // Settings
    QFormLayout* settings_layout;
    QSpinBox* pdf_pixel_density_spin_box;
    QCheckBox* contrast_check_box;
    QPushButton* display_preset_button;
    QCheckBox* enable_image_scaling_check_box;
    QSpinBox* width_spin_box;
    QSpinBox* height_spin_box;
    QComboBox* resampler_combo_box;

    // Progress
    QProgressBar* progress_bar;
    QLabel* elapsed_label;
    QLabel* eta_overall_label;
    QLabel* eta_recent_label;
    QTextEdit* log_output;
    QPushButton* start_button;
    QPushButton* cancel_button;

    // UI setup
    void setup_ui();
    QGroupBox* create_io_group();
    QGroupBox* create_settings_group();
    QGroupBox* create_log_group();

    // Settings group
    void add_pdf_pixel_density_widget();
    void add_contrast_widget();
    void add_display_presets_widget();
    void add_scaling_widgets();
    void add_img_format_widget();
    void add_img_format_specific_options();
    void add_parallel_jobs_widget();

    // UI updates
    void on_add_files_clicked();
    void on_display_preset_changed();
    void on_enable_image_scaling_changed(int state);

    // Helper methods
    QWidget* create_widget_with_info(
        QWidget* main_widget,
        const char* tooltip_text
    );
    void connect_signals();
    void set_display_preset(std::string brand, std::string model);

    int total_files_to_process;
    std::atomic<int> files_processed; // atomic is thread-safe!
};

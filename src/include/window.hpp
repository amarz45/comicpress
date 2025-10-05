#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMap>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QQueue>
#include <QRadioButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <deque>
#include <optional>
#include <string>

#include "qpushbutton.h"
#include "task.hpp"

class BoundedDeque {
    size_t max_size;

  public:
    std::deque<int64_t> dq;
    BoundedDeque(size_t n) : max_size(n) {
    }

    void push_back(int val) {
        if (dq.size() == max_size) {
            dq.pop_front();
        }
        dq.push_back(val);
    }

    void push_front(int val) {
        if (dq.size() == max_size) {
            dq.pop_back();
        }
        dq.push_front(val);
    }
};

struct FileTimer {
    std::optional<int64_t> start_time;
    std::optional<int64_t> last_eta_recent_time;
    int images_since_last_eta_recent = 0;
    BoundedDeque eta_recent_intervals;

    FileTimer() : eta_recent_intervals(5) {
    }
};

struct DisplayPreset {
    std::string brand;
    std::string model;
};

class Window : public QMainWindow {
    Q_OBJECT

  public:
    DisplayPreset display_preset = DisplayPreset{
        .brand = "Custom",
        .model = "",
    };
    explicit Window(QWidget *parent = nullptr);
    ~Window();

  private slots:
    void on_start_button_clicked();
    void on_cancel_button_clicked();
    void handle_log_message(const QString &message);
    void handle_task_finished();
    void start_next_task();
    void on_worker_finished(int exitCode, QProcess::ExitStatus exitStatus);
    void on_worker_output();
    void on_add_files_clicked();
    void on_remove_selected_clicked();
    void on_clear_all_clicked();
    void on_browse_output_clicked();
    void on_double_page_spread_changed(const QString &text);
    void on_display_preset_changed(bool first_time);
    void on_enable_image_scaling_changed(int state);
    void on_enable_image_quantization_changed(int state);
    void on_image_format_changed();
    void on_image_compression_changed(int state);

  private:
    QWidget *central_widget;
    QVBoxLayout *main_layout;

    // Timer
    QTimer *timer;
    std::optional<int64_t> start_time;
    std::optional<int64_t> last_eta_recent_time;
    int images_since_last_eta_recent;
    float last_progress_value;
    BoundedDeque eta_recent_intervals;

    // Input and output
    QListWidget *file_list;
    QPushButton *add_files_button;
    QPushButton *remove_selected_button;
    QPushButton *clear_all_button;
    QLineEdit *output_dir_field;
    QPushButton *browse_output_button;
    QGroupBox *progress_bars_group;
    QVBoxLayout *progress_bars_layout;

    // Settings
    QGroupBox *settings_group;
    QVBoxLayout *settings_layout;
    QSpinBox *pdf_pixel_density_spin_box;
    QComboBox *double_page_spread_combo_box;
    QCheckBox *remove_spine_check_box;
    QCheckBox *contrast_check_box;
    QPushButton *display_preset_button;
    QCheckBox *enable_image_scaling_check_box;
    QWidget *scaling_options_container;
    QSpinBox *width_spin_box;
    QSpinBox *height_spin_box;
    QComboBox *resampler_combo_box;
    QCheckBox *enable_image_quantization_check_box;
    QWidget *quantization_options_container;
    QComboBox *bit_depth_combo_box;
    QDoubleSpinBox *dithering_spin_box;
    QComboBox *image_format_combo_box;
    QWidget *image_format_options_container;
    QSpinBox *image_compression_spin_box;
    QSpinBox *workers_spin_box;
    QWidget *rotation_options_container;
    QRadioButton *clockwise_radio;
    QRadioButton *counter_clockwise_radio;

    // Progress
    QLabel *elapsed_label;
    QLabel *eta_overall_label;
    QLabel *eta_recent_label;
    QProgressBar *progress_bar;
    QTextEdit *log_output;
    QPushButton *start_button;
    QPushButton *cancel_button;

    // Process Management
    QQueue<PageTask> task_queue;
    QList<QProcess *> running_processes;
    QMap<QProcess *, PageTask> running_tasks;
    QMap<QString, int> archive_task_counts;
    QMap<QString, int> total_pages_per_archive;
    QMap<QString, int> pages_processed_per_archive;
    QMap<QString, QWidget *> active_file_widgets;
    QMap<QString, QProgressBar *> active_progress_bars;
    QMap<QString, QLabel *> file_elapsed_labels;
    QMap<QString, QLabel *> file_eta_overall_labels;
    QMap<QString, QLabel *> file_eta_recent_labels;
    QMap<QString, FileTimer> file_timers;
    int max_concurrent_workers;
    bool is_processing_cancelled;

    // Timer
    void update_time_labels();
    void update_overall_time_labels();
    void update_file_time_labels(const QString &file);

    // UI setup
    void setup_ui();
    QGroupBox *create_io_group();
    QGroupBox *create_settings_group();
    void create_log_group();
    QGroupBox *log_group;

    // Settings group
    void add_pdf_pixel_density_widget();
    void add_double_page_spread_widget();
    void add_remove_spine_widget();
    void add_contrast_widget();
    void add_display_presets_widget();
    void add_scaling_widgets();
    void add_quantization_widgets();
    void add_image_format_widgets();
    void add_parallel_workers_widget();

    // Helper methods
    PageTask
    create_task(fs::path source_file, fs::path output_dir, int page_num);
    QWidget *
    create_widget_with_info(QWidget *main_widget, const char *tooltip_text);
    void update_file_list_buttons();
    void connect_signals();
    void
    set_display_preset(std::string brand, std::string model, bool first_time);
    void create_archive(const QString &source_archive_path);

    int total_pages;
    int pages_processed;

    int avif_compression_effort = 4;
    int jpeg_xl_compression_effort = 7;
    int png_compression_effort = 6;
    int webp_compression_effort = 4;
};

std::string time_to_str(int64_t milliseconds);

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QQueue>
#include <QSpinBox>
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

    // void print() {
    //     for (int x : dq) std::cout << x << " ";
    //     std::cout << "\n";
    // }
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

    // Settings
    QFormLayout *settings_layout;
    QSpinBox *pdf_pixel_density_spin_box;
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
    QSpinBox *workers_spin_box;

    // Progress
    QProgressBar *progress_bar;
    QLabel *elapsed_label;
    QLabel *eta_overall_label;
    QLabel *eta_recent_label;
    QTextEdit *log_output;
    QPushButton *start_button;
    QPushButton *cancel_button;

    // Process Management
    QQueue<PageTask> task_queue;
    QList<QProcess *> running_processes;
    int max_concurrent_workers;
    bool is_processing_cancelled;

    // Timer
    void update_time_labels();

    // UI setup
    void setup_ui();
    QGroupBox *create_io_group();
    QGroupBox *create_settings_group();
    QGroupBox *create_log_group();

    // Settings group
    void add_pdf_pixel_density_widget();
    void add_contrast_widget();
    void add_display_presets_widget();
    void add_scaling_widgets();
    void add_quantization_widgets();
    void add_image_format_widgets();
    void add_parallel_workers_widget();

    // UI updates
    void on_add_files_clicked();
    void on_display_preset_changed();
    void on_enable_image_scaling_changed(int state);
    void on_enable_image_quantization_changed(int state);

    // Helper methods
    QWidget *create_widget_with_info(
        QWidget *main_widget, const char *tooltip_text, bool add_stretch
    );
    void connect_signals();
    void set_display_preset(std::string brand, std::string model);

    int total_files_to_process;
    int files_processed;
};

std::string time_to_str(int64_t milliseconds);

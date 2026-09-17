#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>

enum class OutputFormat : std::uint8_t { EPUB, CBZ };
enum class PdfQuality : std::uint16_t {
    STANDARD = 300,
    HIGH = 600,
    ULTRA = 1200,
    CUSTOM = 0,
};

struct Options {
    QGroupBox *settings_group;
    QFormLayout *settings_layout;
    QCheckBox *advanced_options_check_box;
#if defined(PDF_ENABLED)
    QLabel *pdf_pixel_density_label;
    QLabel *pdf_pixel_density_tooltip;
    QComboBox *pdf_pixel_density_combo_box;
    QWidget *pdf_options_container;
    QSpinBox *pdf_pixel_density_spin_box;
#endif
    QCheckBox *convert_to_greyscale;
    QComboBox *double_page_spread_combo_box;
    QLabel *linear_light_resampling_label;
    QCheckBox *linear_light_resampling_check_box;
    QWidget *linear_light_resampling_container;
    QCheckBox *remove_spine_check_box;
    QCheckBox *contrast_check_box;
    QPushButton *display_preset_button;
    QComboBox *output_format_combo_box;
    QLabel *scale_pages_label;
    QWidget *scale_pages_container;
    QCheckBox *enable_image_scaling_check_box;
    QWidget *scaling_options_container;
    QSpinBox *width_spin_box;
    QSpinBox *height_spin_box;
    QComboBox *resampler_combo_box;
    QCheckBox *enable_image_quantization_check_box;
    QLabel *quantize_pages_label;
    QWidget *quantize_pages_container;
    QWidget *quantization_options_container;
    QComboBox *bit_depth_combo_box;
    QDoubleSpinBox *dithering_spin_box;
    QLabel *image_format_label;
    QWidget *image_format_container;
    QWidget *image_format_options_container;
    QComboBox *image_format_combo_box;
    QLabel *image_compression_label;
    QSpinBox *image_compression_spin_box;
    QComboBox *image_compression_type_combo_box;
    QLabel *image_compression_type_label;
    QLabel *image_compression_type_tooltip;
    QLabel *image_quality_jpeg_xl_tooltip;
    QDoubleSpinBox *image_quality_spin_box;
    QWidget *image_quality_label;
    QLabel *image_quality_label_original;
    QComboBox *image_quality_label_jpeg_xl;
    QLabel *workers_label;
    QSpinBox *workers_spin_box;
    QWidget *output_format_container;
    QComboBox *reading_direction_combo_box;
    QWidget *rotation_options_container;
    QComboBox *rotation_direction_combo_box;
};

class DensitySpinBox : public QSpinBox {
    Q_OBJECT

  public:
    explicit DensitySpinBox(QWidget *parent = nullptr) : QSpinBox(parent) {
        setRange(1, 4'800);
        setValue(300);
        setSingleStep(300);
        setSuffix("\u202fPPI");
    }

  protected:
    void stepBy(int steps) override {
        if (steps == 0) {
            return;
        }

        const int step_size = 300;
        const bool up = steps > 0;
        const int num_steps = qAbs(steps);

        for (int i = 0; i < num_steps; i += 1) {
            int current = value();
            int target;

            if (up) {
                if (current % step_size == 0) {
                    target = current + step_size;
                }
                else {
                    target = ((current / step_size) + 1) * step_size;
                }
            }
            else {
                if (current % step_size == 0) {
                    target = current - step_size;
                }
                else {
                    target = (current / step_size) * step_size;
                }
            }

            target = qBound(minimum(), target, maximum());
            setValue(target);
        }
    }
};

void add_output_format_widget(QStyle *style, Options *options);
#if defined(PDF_ENABLED)
void add_pdf_pixel_density_widget(QStyle *style, Options *options);
#endif
void add_convert_to_greyscale_widget(QStyle *style, Options *options);
void add_double_page_spread_widget(QStyle *style, Options *options);
void add_linear_light_resampling_widget(QStyle *style, Options *options);
void add_remove_spine_widget(QStyle *style, Options *options);
void add_contrast_widget(QStyle *style, Options *options);
void add_scaling_widgets(QStyle *style, Options *options);
void add_quantization_widgets(QStyle *style, Options *options);
void add_image_format_widgets(QStyle *style, Options *options);
void add_parallel_workers_widget(QStyle *style, Options *options);

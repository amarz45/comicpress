#pragma once

#include "window.hpp"
#include <QSpinBox>
#include <QStyle>

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

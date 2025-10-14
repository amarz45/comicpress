#pragma once

#include "window.hpp"
#include <QStyle>

class DensitySpinBox : public QSpinBox {
    Q_OBJECT

  public:
    explicit DensitySpinBox(QWidget *parent = nullptr) : QSpinBox(parent) {
        setRange(1, 4'800);
        setValue(300);
        setSingleStep(300);
    }

  protected:
    void stepBy(int steps) override {
        if (steps == 0) {
            return;
        }

        const int stepSize = 300;
        const bool up = steps > 0;
        const int numSteps = qAbs(steps);

        for (int i = 0; i < numSteps; ++i) {
            int current = value();
            int target;

            if (up) {
                if (current % stepSize == 0) {
                    target = current + stepSize;
                }
                else {
                    target = ((current / stepSize) + 1) * stepSize;
                }
            }
            else {
                if (current % stepSize == 0) {
                    target = current - stepSize;
                }
                else {
                    target = (current / stepSize) * stepSize;
                }
            }

            target = qBound(minimum(), target, maximum());
            setValue(target);
        }
    }
};

void add_pdf_pixel_density_widget(QStyle *style, Options *options);
void add_convert_to_greyscale_widget(QStyle *style, Options *options);
void add_double_page_spread_widget(QStyle *style, Options *options);
void add_remove_spine_widget(QStyle *style, Options *options);
void add_contrast_widget(QStyle *style, Options *options);
void add_scaling_widgets(QStyle *style, Options *options);
void add_quantization_widgets(QStyle *style, Options *options);
void add_image_format_widgets(QStyle *style, Options *options);
void add_parallel_workers_widget(QStyle *style, Options *options);

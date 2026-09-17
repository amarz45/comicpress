#pragma once

#include <QSettings>
#include <Qt>
#include <string_view>

#include "options.hpp"

inline constexpr auto DISPLAY_PRESET_BRAND_SETTINGS_KEY
    = "display_preset/brand";
inline constexpr auto DISPLAY_PRESET_MODEL_SETTINGS_KEY
    = "display_preset/model";

template <typename Widget>
struct FieldToWidget {
    std::string_view field;
    Widget *widget;
};

inline auto fields_to_widgets(const Options &options) {
    return std::tuple{
        FieldToWidget{"output_format", options.output_format_combo_box},
        FieldToWidget{"reading_direction", options.reading_direction_combo_box},
#if defined(PDF_ENABLED)
        FieldToWidget{
            "pdf_pixel_density_preset", options.pdf_pixel_density_combo_box
        },
        FieldToWidget{
            "pdf_pixel_density_custom", options.pdf_pixel_density_spin_box
        },
#endif
        FieldToWidget{"convert_to_greyscale", options.convert_to_greyscale},
        FieldToWidget{"stretch_contrast", options.contrast_check_box},
        FieldToWidget{"two_page_spreads", options.double_page_spread_combo_box},
        FieldToWidget{
            "rotation_direction", options.rotation_direction_combo_box
        },
        FieldToWidget{"remove_spines", options.remove_spine_check_box},
        FieldToWidget{
            "linear_light_resampling", options.linear_light_resampling_check_box
        },
        FieldToWidget{
            "quantize_pages", options.enable_image_quantization_check_box
        },
        FieldToWidget{"bit_depth", options.bit_depth_combo_box},
        FieldToWidget{"dithering", options.dithering_spin_box},
        FieldToWidget{"scale_pages", options.enable_image_scaling_check_box},
        FieldToWidget{"width", options.width_spin_box},
        FieldToWidget{"height", options.height_spin_box},
        FieldToWidget{"resampler", options.resampler_combo_box},
        FieldToWidget{"image_format", options.image_format_combo_box},
        FieldToWidget{
            "image_compression_type", options.image_compression_type_combo_box
        },
        FieldToWidget{
            "image_compression_effort", options.image_compression_spin_box
        },
        FieldToWidget{
            "jpeg_xl_image_quality_type", options.image_quality_label_jpeg_xl
        },
        FieldToWidget{"image_quality", options.image_quality_spin_box},
    };
}

template <typename Apply>
void with_setting(QAnyStringView field, Apply &&apply) {
    QSettings settings;
    if (settings.contains(field)) {
        std::forward<Apply>(apply)(settings.value(field));
    }
}

QVariant settings_value_from_widget(const QComboBox *combo_box);
void set_widget_value_from_settings(QAnyStringView field, QComboBox *combo_box);

QVariant settings_value_from_widget(const QCheckBox *check_box);
void set_widget_value_from_settings(QAnyStringView field, QCheckBox *check_box);

QVariant settings_value_from_widget(const QSpinBox *spin_box);
void set_widget_value_from_settings(QAnyStringView field, QSpinBox *spin_box);

QVariant settings_value_from_widget(const QDoubleSpinBox *spin_box);
void set_widget_value_from_settings(
    QAnyStringView field, QDoubleSpinBox *spin_box
);

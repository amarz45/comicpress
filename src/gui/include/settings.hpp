#pragma once

#include <QSettings>
#include <Qt>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "options.hpp"

inline constexpr auto DISPLAY_PRESET_BRAND_SETTINGS_KEY
    = "display_preset/brand";
inline constexpr auto DISPLAY_PRESET_MODEL_SETTINGS_KEY
    = "display_preset/model";
inline constexpr auto IMAGE_FORMATS_SETTINGS_GROUP = "image_formats";
inline constexpr auto FORMAT_QUALITY_SETTINGS_KEY = "quality";
inline constexpr auto FORMAT_COMPRESSION_EFFORT_SETTINGS_KEY
    = "compression_effort";
inline constexpr auto FORMAT_DISTANCE_SETTINGS_KEY = "distance";

template <typename Widget>
struct FieldToWidget {
    std::string_view field;
    Widget widget;
};

// A widget whose value is derived from other settings when `is_derived` is
// true. It is then not persisted, since restoring those settings restores it.
template <typename Widget>
struct DerivableWidget {
    Widget *widget;
    bool is_derived;
};

inline auto
fields_to_widgets(const Options &options, bool is_display_preset_active) {
#if defined(PDF_ENABLED)
    auto is_pdf_pixel_density_preset
        = options.pdf_pixel_density_combo_box->currentData().value<PdfQuality>()
       != PdfQuality::CUSTOM;
#endif
    // These only show the current image format’s entry in
    // `Window::format_settings_`, which is persisted instead.
    auto is_per_format = true;

    return std::tuple{
        FieldToWidget{"output_format", options.output_format_combo_box},
        FieldToWidget{"reading_direction", options.reading_direction_combo_box},
#if defined(PDF_ENABLED)
        FieldToWidget{
            "pdf_pixel_density_preset", options.pdf_pixel_density_combo_box
        },
        FieldToWidget{
            "pdf_pixel_density_custom",
            DerivableWidget{
                options.pdf_pixel_density_spin_box, is_pdf_pixel_density_preset
            }
        },
#endif
        FieldToWidget{
            "convert_to_greyscale",
            DerivableWidget{
                options.convert_to_greyscale, is_display_preset_active
            }
        },
        FieldToWidget{"stretch_contrast", options.contrast_check_box},
        FieldToWidget{"two_page_spreads", options.double_page_spread_combo_box},
        FieldToWidget{
            "rotation_direction", options.rotation_direction_combo_box
        },
        FieldToWidget{"remove_spines", options.remove_spine_check_box},
        FieldToWidget{
            "show_advanced_options", options.advanced_options_check_box
        },
        FieldToWidget{
            "linear_light_resampling", options.linear_light_resampling_check_box
        },
        FieldToWidget{
            "quantize_pages", options.enable_image_quantization_check_box
        },
        FieldToWidget{
            "bit_depth",
            DerivableWidget{
                options.bit_depth_combo_box, is_display_preset_active
            }
        },
        FieldToWidget{"dithering", options.dithering_spin_box},
        FieldToWidget{
            "scale_pages",
            DerivableWidget{
                options.enable_image_scaling_check_box, is_display_preset_active
            }
        },
        FieldToWidget{
            "width",
            DerivableWidget{options.width_spin_box, is_display_preset_active}
        },
        FieldToWidget{
            "height",
            DerivableWidget{options.height_spin_box, is_display_preset_active}
        },
        FieldToWidget{"resampler", options.resampler_combo_box},
        FieldToWidget{"image_format", options.image_format_combo_box},
        FieldToWidget{
            "image_compression_type", options.image_compression_type_combo_box
        },
        FieldToWidget{
            "image_compression_effort",
            DerivableWidget{options.image_compression_spin_box, is_per_format}
        },
        FieldToWidget{
            "jpeg_xl_image_quality_type", options.image_quality_label_jpeg_xl
        },
        FieldToWidget{
            "image_quality",
            DerivableWidget{options.image_quality_spin_box, is_per_format}
        },
    };
}

// Returns `std::nullopt` if the stored value isn’t a valid `T`.
template <typename T>
std::optional<T> from_settings_value(const QVariant &variant);
template <>
std::optional<bool> from_settings_value(const QVariant &variant);
template <>
std::optional<int> from_settings_value(const QVariant &variant);
template <>
std::optional<double> from_settings_value(const QVariant &variant);

// Calls `apply` with the setting converted to `T`, if it exists and is a valid
// `T`. With the default `T`, the raw `QVariant` is passed unconverted.
template <typename T = QVariant, typename Apply>
void with_setting(QAnyStringView field, Apply &&apply) {
    QSettings settings;
    if (!settings.contains(field)) {
        return;
    }
    if constexpr (std::is_same_v<T, QVariant>) {
        std::forward<Apply>(apply)(settings.value(field));
    }
    else if (auto value = from_settings_value<T>(settings.value(field))) {
        std::forward<Apply>(apply)(*value);
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

// An invalid `QVariant` means there is nothing to save.
template <typename Widget>
QVariant settings_value_from_widget(DerivableWidget<Widget> derivable) {
    return derivable.is_derived ? QVariant{}
                                : settings_value_from_widget(derivable.widget);
}
template <typename Widget>
void set_widget_value_from_settings(
    QAnyStringView field, DerivableWidget<Widget> derivable
) {
    set_widget_value_from_settings(field, derivable.widget);
}

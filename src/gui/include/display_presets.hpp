#pragma once

#include "../../include/task.hpp"
#include <map>
#include <optional>
#include <string>
#include <vector>

struct DisplaySpec {
    const int width;
    const int height;
    const BitDepth bit_depth;
    const bool colour;
};

using ModelMap = std::vector<std::pair<std::string, DisplaySpec>>;
using BrandMap = std::map<std::string, std::optional<ModelMap>>;

const BrandMap DISPLAY_PRESETS = {
    {"None", std::nullopt},

    {"Kindle",
     ModelMap{
         {"1", {600, 670, BitDepth::TWO, false}},
         {"2", {600, 670, BitDepth::FOUR, false}},
         {"5", {600, 800, BitDepth::FOUR, false}},
         {"7", {600, 800, BitDepth::FOUR, false}},
         {"8", {600, 800, BitDepth::FOUR, false}},
         {"10", {600, 800, BitDepth::FOUR, false}},
         {"11", {1072, 1448, BitDepth::FOUR, false}},
         {"Colorsoft", {1264, 1680, BitDepth::FOUR, true}},
         {"DX", {824, 1000, BitDepth::FOUR, false}},
         {"DXG", {824, 1000, BitDepth::FOUR, false}},
         {"Keyboard", {600, 800, BitDepth::FOUR, false}},
         {"Oasis 1", {1072, 1448, BitDepth::FOUR, false}},
         {"Oasis 2", {1264, 1680, BitDepth::FOUR, false}},
         {"Oasis 3", {1264, 1680, BitDepth::FOUR, false}},
         {"Paperwhite 1", {758, 1024, BitDepth::FOUR, false}},
         {"Paperwhite 2", {758, 1024, BitDepth::FOUR, false}},
         {"Paperwhite 3", {1072, 1448, BitDepth::FOUR, false}},
         {"Paperwhite 4", {1072, 1448, BitDepth::FOUR, false}},
         {"Paperwhite 5", {1236, 1648, BitDepth::FOUR, false}},
         {"Paperwhite 12", {1264, 1680, BitDepth::FOUR, false}},
         {"Paperwhite Signature Edition", {1236, 1648, BitDepth::FOUR, false}},
         {"Scribe", {1860, 2480, BitDepth::FOUR, true}},
         {"Touch", {600, 800, BitDepth::FOUR, false}},
         {"Voyage", {1072, 1448, BitDepth::FOUR, false}},
     }},

    {"Kobo",
     ModelMap{
         {"Aura", {758, 1024, BitDepth::FOUR, false}},
         {"Aura H2O", {1080, 1430, BitDepth::FOUR, false}},
         {"Aura HD", {1080, 1440, BitDepth::FOUR, false}},
         {"Aura ONE", {1404, 1872, BitDepth::FOUR, false}},
         {"Clara 2E", {1072, 1448, BitDepth::FOUR, false}},
         {"Clara Colour", {1072, 1448, BitDepth::FOUR, true}},
         {"Clara HD", {1072, 1448, BitDepth::FOUR, false}},
         {"Elipsa", {1404, 1872, BitDepth::FOUR, false}},
         {"Forma", {1440, 1920, BitDepth::FOUR, false}},
         {"Glo", {768, 1024, BitDepth::FOUR, false}},
         {"Glo HD", {1072, 1448, BitDepth::FOUR, false}},
         {"Libra 2", {1264, 1680, BitDepth::FOUR, false}},
         {"Libra Colour", {1264, 1680, BitDepth::FOUR, true}},
         {"Libra H2O", {1264, 1680, BitDepth::FOUR, false}},
         {"Mini", {600, 800, BitDepth::FOUR, false}},
         {"Nia", {758, 1024, BitDepth::FOUR, false}},
         {"Sage", {1440, 1920, BitDepth::FOUR, false}},
         {"Touch", {600, 800, BitDepth::FOUR, false}},
     }},

    {"Meebook", ModelMap{{"M103", {1404, 1872, BitDepth::FOUR, false}}}},

    {"reMarkable",
     ModelMap{
         {"1", {1404, 1872, BitDepth::FOUR, false}},
         {"2", {1404, 1872, BitDepth::FOUR, false}},
         {"Paper Pro", {1620, 2160, BitDepth::FOUR, true}},
         {"Paper Pro Move", {954, 1696, BitDepth::FOUR, true}},
     }},
};

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

enum BitDepthIndex {
    ONE = 0,
    TWO = 1,
    FOUR = 2,
    EIGHT = 3,
    SIXTEEEN = 4,
};

struct Display {
    const unsigned int width;
    const unsigned int height;
    const BitDepthIndex bit_depth_index;
    const bool colour;
};

using ModelMap = std::vector<std::pair<std::string, Display>>;
using BrandMap = std::map<std::string, std::optional<ModelMap>>;

const BrandMap DISPLAY_PRESETS = {
    {"Custom", std::nullopt},

    {"Kindle",
     ModelMap{
         {"1", {600, 670, BitDepthIndex::TWO, false}},
         {"2", {600, 670, BitDepthIndex::FOUR, false}},
         {"5", {600, 800, BitDepthIndex::FOUR, false}},
         {"7", {600, 800, BitDepthIndex::FOUR, false}},
         {"8", {600, 800, BitDepthIndex::FOUR, false}},
         {"10", {600, 800, BitDepthIndex::FOUR, false}},
         {"11", {1072, 1448, BitDepthIndex::FOUR, false}},
         {"Colorsoft", {1264, 1680, BitDepthIndex::FOUR, true}},
         {"DX", {824, 1000, BitDepthIndex::FOUR, false}},
         {"DXG", {824, 1000, BitDepthIndex::FOUR, false}},
         {"Keyboard", {600, 800, BitDepthIndex::FOUR, false}},
         {"Oasis 1", {1072, 1448, BitDepthIndex::FOUR, false}},
         {"Oasis 2", {1264, 1680, BitDepthIndex::FOUR, false}},
         {"Oasis 3", {1264, 1680, BitDepthIndex::FOUR, false}},
         {"Paperwhite 1", {758, 1024, BitDepthIndex::FOUR, false}},
         {"Paperwhite 2", {758, 1024, BitDepthIndex::FOUR, false}},
         {"Paperwhite 3", {1072, 1448, BitDepthIndex::FOUR, false}},
         {"Paperwhite 4", {1072, 1448, BitDepthIndex::FOUR, false}},
         {"Paperwhite 5", {1236, 1648, BitDepthIndex::FOUR, false}},
         {"Paperwhite 12", {1264, 1680, BitDepthIndex::FOUR, false}},
         {"Paperwhite Signature Edition",
          {1236, 1648, BitDepthIndex::FOUR, false}},
         {"Scribe", {1860, 2480, BitDepthIndex::FOUR, true}},
         {"Touch", {600, 800, BitDepthIndex::FOUR, false}},
         {"Voyage", {1072, 1448, BitDepthIndex::FOUR, false}},
     }},

    {"Kobo",
     ModelMap{
         {"Aura", {758, 1024, BitDepthIndex::FOUR, false}},
         {"Aura H2O", {1080, 1430, BitDepthIndex::FOUR, false}},
         {"Aura HD", {1080, 1440, BitDepthIndex::FOUR, false}},
         {"Aura ONE", {1404, 1872, BitDepthIndex::FOUR, false}},
         {"Clara 2E", {1072, 1448, BitDepthIndex::FOUR, false}},
         {"Clara Colour", {1072, 1448, BitDepthIndex::FOUR, true}},
         {"Clara HD", {1072, 1448, BitDepthIndex::FOUR, false}},
         {"Elipsa", {1404, 1872, BitDepthIndex::FOUR, false}},
         {"Forma", {1440, 1920, BitDepthIndex::FOUR, false}},
         {"Glo", {768, 1024, BitDepthIndex::FOUR, false}},
         {"Glo HD", {1072, 1448, BitDepthIndex::FOUR, false}},
         {"Libra 2", {1264, 1680, BitDepthIndex::FOUR, false}},
         {"Libra Colour", {1264, 1680, BitDepthIndex::FOUR, true}},
         {"Libra H2O", {1264, 1680, BitDepthIndex::FOUR, false}},
         {"Mini", {600, 800, BitDepthIndex::FOUR, false}},
         {"Nia", {758, 1024, BitDepthIndex::FOUR, false}},
         {"Sage", {1440, 1920, BitDepthIndex::FOUR, false}},
         {"Touch", {600, 800, BitDepthIndex::FOUR, false}},
     }},

    {"reMarkable",
     ModelMap{
         {"1", {1404, 1872, BitDepthIndex::FOUR, false}},
         {"2", {1404, 1872, BitDepthIndex::FOUR, false}},
         {"Paper Pro", {1620, 2160, BitDepthIndex::FOUR, true}},
         {"Paper Pro Move", {954, 1696, BitDepthIndex::FOUR, true}},
     }},
};

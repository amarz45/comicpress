#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

struct Display {
    const unsigned int width;
    const unsigned int height;
    const unsigned int bit_depth;
    const bool colour;
};

using ModelMap = std::vector<std::pair<std::string, Display>>;
using BrandMap = std::map<std::string, std::optional<ModelMap>>;

const BrandMap DISPLAY_PRESETS = {
    {"Custom", std::nullopt},

    {"Kindle",
     ModelMap{
         {"1", {600, 670, 2, false}},
         {"2", {600, 670, 4, false}},
         {"DX", {824, 1000, 4, false}},
         {"DXG", {824, 1000, 4, false}},
         {"Keyboard", {600, 800, 4, false}},
         {"Touch", {600, 800, 4, false}},
         {"5", {600, 800, 4, false}},
         {"7", {600, 800, 4, false}},
         {"Paperwhite 1", {758, 1024, 4, false}},
         {"Paperwhite 2", {758, 1024, 4, false}},
         {"Voyage", {1072, 1448, 4, false}},

         {"Paperwhite 3", {1072, 1448, 4, false}},
         {"Paperwhite 4", {1072, 1448, 4, false}},
         {"Paperwhite Oasis", {1072, 1448, 4, false}},
         {"8", {600, 800, 4, false}},
         {"10", {600, 800, 4, false}},
         {"Oasis 2", {1264, 1680, 4, false}},
         {"Oasis 3", {1264, 1680, 4, false}},
         {"Paperwhite 12", {1264, 1680, 4, false}},
         {"Colorsoft", {1264, 1680, 4, true}},
         {"11", {1072, 1448, 4, false}},
         {"Paperwhite 5", {1236, 1648, 4, false}},
         {"Paperwhite Signature Edition", {1236, 1648, 4, false}},
         {"Scribe", {1860, 2480, 4, true}},
     }},

    {"Kobo",
     ModelMap{
         {"Mini", {600, 800, 4, false}},
         {"Touch", {600, 800, 4, false}},
         {"Glo", {768, 1024, 4, false}},
         {"Glo HD", {1072, 1448, 4, false}},
         {"Aura", {758, 1024, 4, false}},
         {"Aura HD", {1080, 1440, 4, false}},
         {"Aura H2O", {1080, 1430, 4, false}},
         {"Aura ONE", {1404, 1872, 4, false}},
         {"Nia", {758, 1024, 4, false}},
         {"Clara HD", {1072, 1448, 4, false}},
         {"Clara 2E", {1072, 1448, 4, false}},
         {"Clara Colour", {1072, 1448, 4, true}},
         {"Libra H2O", {1264, 1680, 4, false}},
         {"Libra 2", {1264, 1680, 4, false}},
         {"Libra Colour", {1264, 1680, 4, true}},
         {"Forma", {1440, 1920, 4, false}},
         {"Sage", {1440, 1920, 4, false}},
         {"Elipsa", {1404, 1872, 4, false}},
     }},

    {"reMarkable",
     ModelMap{
         {"1", {1404, 1872, 4, false}},
         {"2", {1404, 1872, 4, false}},
         {"Paper Pro", {1620, 2160, 4, true}},
         {"Paper Pro Move", {954, 1696, 4, true}},
     }},
};

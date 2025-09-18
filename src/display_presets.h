#pragma once

#include <map>
#include <optional>
#include <string>

struct Display {
    const int width;
    const int height;
    const bool colour;
};

using ModelMap = std::map<std::string, Display>;
using BrandMap = std::map<std::string, std::optional<ModelMap>>;

const BrandMap DISPLAY_PRESETS = {
    {"Custom", std::nullopt},

    {"Bigme", ModelMap {
        {"B1051C Pro", {1860, 2480, true}},
        {"B13", {1650, 2200, true}},
        {"B6", {1072, 1448, true}},
        {"B7", {1264, 1680, true}},
        {"B751C", {1264, 1680, true}},
        {"HiBreak Pro", {824, 1648, false}},
        {"Read", {758, 1024, false}},
    }},

    {"Bookeen", ModelMap {
        {"Cybook Gen3", {600, 800, false}},
        {"Cybook Muse HD", {1072, 1448, false}},
        {"Cybook Odyssey", {600, 800, false}},
        {"Cybook Odyssey HD FrontLight", {758, 1024, false}},
        {"Cybook Opus", {600, 800, false}},
        {"Cybook Orizon", {600, 800, false}},
    }},

    {"Boox", ModelMap {
        {"C65HD", {758, 1024, false}},
        {"C65ML", {758, 1024, false}},
        {"Go 6", {1072, 1448, false}},
        {"Go 7", {1264, 1680, false}},
        {"Go Color 7", {1264, 1680, true}},
        {"Go Color 7 Gen II", {1264, 1680, true}},
        {"Likebook Plus", {1404, 1872, false}},
        {"Note Air 4 C", {1404, 1872, true}},
        {"Note Air 3 C", {1404, 1872, true}},
        {"Note Max", {1650, 2200, false}},
        {"Page", {1264, 1680, false}},
        {"Palma", {824, 1648, false}},
        {"T62+", {758, 1024, false}},
        {"Tab Ultra C Pro", {1860, 2480, true}},
        {"Tab X", {1650, 2200, false}},
        {"X5", {1404, 1872, false}},
    }},

    {"EnTourage", ModelMap {
        {"eDGe", {825, 1200, false}}
    }},

    {"Hanlin", ModelMap {
        {"V3", {600, 800, false}}
    }},

    {"Icarus", ModelMap {
        {"Excel", {825, 1200, false}}
    }},

    {"Iriver", ModelMap {
        {"Story", {600, 800, false}},
        {"Story HD", {768, 1024, false}},
    }},

    {"JetBook", ModelMap {
        {"Color", {1200, 1600, true}}
    }},

    {"Kindle", ModelMap {
        {"(10th Gen)", {600, 800, false}},
        {"(11th Gen, 2022)", {1072, 1448, false}},
        {"(1st Gen)", {600, 800, false}},
        {"(2nd Gen)", {600, 800, false}},
        {"(4th Gen)", {600, 800, false}},
        {"(5th Gen)", {600, 800, false}},
        {"(7th Gen)", {600, 800, false}},
        {"(8th Gen)", {600, 800, false}},
        {"Colorsoft", {1264, 1680, true}},
        {"DX", {824, 1200, false}},
        {"Keyboard", {600, 800, false}},
        {"Oasis (1st Gen)", {1080, 1440, false}},
        {"Oasis (2nd Gen)", {1264, 1680, false}},
        {"Oasis (3rd Gen)", {1264, 1680, false}},
        {"Paperwhite (11th Gen)", {1236, 1648, false}},
        {"Paperwhite (12th Gen, 2024)", {1264, 1680, false}},
        {"Paperwhite (1st Gen)", {758, 1024, false}},
        {"Paperwhite (2nd Gen)", {758, 1024, false}},
        {"Paperwhite (3rd Gen)", {1080, 1430, false}},
        {"Paperwhite (4th Gen)", {1072, 1448, false}},
        {"Paperwhite (5th Gen)", {1236, 1648, false}},
        {"Paperwhite Signature Edition", {1236, 1648, false}},
        {"Scribe", {1860, 2480, false}},
        {"Touch", {600, 800, false}},
        {"Voyage", {1080, 1430, false}},
    }},

    {"Kobo", ModelMap {
        {"Aura", {758, 1024, false}},
        {"Aura Edition 2", {768, 1024, false}},
        {"Aura H2O", {1080, 1430, false}},
        {"Aura H2O Edition 2", {1080, 1440, false}},
        {"Aura HD", {1080, 1440, false}},
        {"Aura One", {1404, 1872, false}},
        {"Clara 2E", {1072, 1448, false}},
        {"Clara BW", {1072, 1448, false}},
        {"Clara Colour", {1072, 1448, true}},
        {"Clara HD", {1072, 1448, false}},
        {"Elipsa", {1404, 1872, false}},
        {"Elipsa 2E", {1404, 1872, false}},
        {"Forma", {1440, 1920, false}},
        {"Glo", {758, 1024, false}},
        {"Glo HD", {1072, 1448, false}},
        {"Libra 2", {1264, 1680, false}},
        {"Libra Colour", {1264, 1680, true}},
        {"Libra H2O", {1264, 1680, false}},
        {"Mini", {600, 800, false}},
        {"Nia", {758, 1024, false}},
        {"Original", {600, 800, false}},
        {"Sage", {1440, 1920, false}},
        {"Touch", {600, 800, false}},
        {"Touch 2.0", {600, 800, false}},
    }},

    {"Nook", ModelMap {
        {"1st Edition", {600, 800, false}},
        {"Color", {600, 1024, true}},
        {"GlowLight", {758, 1024, false}},
        {"GlowLight 3", {1072, 1448, false}},
        {"GlowLight 4", {1072, 1448, false}},
        {"GlowLight 4 Plus", {1404, 1872, false}},
        {"GlowLight Plus", {1080, 1430, false}},
        {"GlowLight Plus 7.8", {1404, 1872, false}},
        {"Simple Touch", {600, 800, false}},
        {"Simple Touch with GlowLight", {600, 800, false}},
    }},

    {"Pocketbook", ModelMap {
        {"Basic Touch 2", {600, 800, false}},
        {"Color", {1072, 1448, true}},
        {"Era", {1264, 1680, false}},
        {"Era Color", {1264, 1680, true}},
        {"InkPad 4", {1404, 1872, false}},
        {"InkPad Color 3", {1404, 1872, true}},
        {"InkPad Eo", {1860, 2480, false}},
        {"Touch Lux 4", {758, 1024, false}},
        {"Verse", {758, 1024, false}},
        {"Verse Lite", {758, 1024, false}},
        {"Verse Pro", {1072, 1448, false}},
        {"Verse Pro Color", {1072, 1448, true}},
    }},

    {"reMarkable", ModelMap {
        {"1", {1404, 1872, false}},
        {"2", {1404, 1872, false}},
        {"Paper Pro", {1620, 2160, true}},
    }},

    {"Sony", ModelMap {
        {"Librie EBR-1000", {600, 800, false}},
        {"PRS-350", {600, 800, false}},
        {"PRS-500", {600, 800, false}},
        {"PRS-505", {600, 800, false}},
        {"PRS-600", {600, 800, false}},
        {"PRS-650", {600, 800, false}},
        {"PRS-700", {600, 800, false}},
        {"PRS-900", {600, 1024, false}},
        {"PRS-T1", {600, 800, false}},
        {"PRS-T2", {600, 800, false}},
        {"PRS-T3", {758, 1024, false}},
    }},

    {"Supernote", ModelMap {
        {"A5 X", {1404, 1872, false}},
        {"A5 X2", {1920, 2560, false}},
        {"A6 X", {1404, 1872, false}},
        {"Manta", {1920, 2560, false}},
        {"Nomad", {1404, 1872, false}},
    }},
};

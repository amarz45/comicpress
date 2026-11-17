#include "include/task.hpp"

#include <bit>
#include <charconv>
#include <cstddef>
#include <filesystem>
#include <format>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// fs::path <-> std::string
static std::string encode_field(const fs::path &path) {
    return path.string();
}
static void decode_field(std::string_view path, fs::path &out) {
    out = fs::path(path);
}

// std::string <-> std::string
static std::string encode_field(std::string str) {
    return str;
}
static void decode_field(std::string_view str, std::string &out) {
    out = str;
}

// int <-> std::string
static std::string encode_field(int number) {
    return std::to_string(number);
}
static void decode_field(std::string_view number, int &out) {
    std::from_chars(number.data(), number.data() + number.size(), out);
}

// ProcessingSettings <-> std::string
static std::string encode_field(const ProcessingSettings &settings) {
    auto bytes = std::bit_cast<ProcessingSettingsBytes>(settings);
    auto hex = std::string{};
    hex.reserve(bytes.size() * 2);
    for (const auto byte : bytes) {
        hex += std::format("{:02x}", byte);
    }
    return hex;
}
static void
decode_field(std::string_view processing_settings, ProcessingSettings &out) {
    auto hex = processing_settings;
    auto bytes = ProcessingSettingsBytes{};
    if (hex.size() != bytes.size() * 2) {
        throw std::invalid_argument("settings size mismatch");
    }
    for (std::size_t i = 0; i < bytes.size(); i += 1) {
        auto hex_first = i * 2;
        auto *hex_start = hex.data() + hex_first;
        auto *hex_end = hex.data() + hex_first + 2;
        std::from_chars(hex_start, hex_end, bytes[i], 16);
    }
    out = std::bit_cast<ProcessingSettings>(bytes);
}

std::vector<std::string> encode_task(const PageTask &task) {
    auto result = std::vector<std::string>{"-worker"};
    for_each_task_field(task, [&](const auto &field) {
        result.push_back(encode_field(field));
    });
    return result;
}

PageTask decode_task(std::span<const char *const> encoded_fields) {
    if (encoded_fields.empty()
        || std::string_view(encoded_fields[0]) != "-worker") {
        throw std::invalid_argument("Missing `-worker` argument.");
    }

    auto task = PageTask{};
    std::size_t i = 1;
    for_each_task_field(task, [&](auto &field) {
        if (!(i < encoded_fields.size())) {
            throw std::out_of_range("Wrong field count.");
        }
        decode_field(encoded_fields[i], field);
        i += 1;
    });
    return task;
}

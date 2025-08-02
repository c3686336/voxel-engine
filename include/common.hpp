#ifndef COMMON_H
#define COMMON_H

#define DAG

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include <spdlog/spdlog.h>

#include <stb_image.h>

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

std::string load_file(const std::filesystem::path& path);

template <typename T> struct DeepHash {
    std::size_t operator()(const std::shared_ptr<const T>& t) const noexcept {
        return std::hash<T>(*t);
    }
};

inline std::tuple<std::vector<std::byte>, int, int>
load_image(const std::filesystem::path& path, int desired_channels = 4) {
    // stbi_set_flip_vertically_on_load(true);

    int width, height, channels;
    std::byte* data = (std::byte*)stbi_load(
        path.c_str(), &width, &height, &channels, desired_channels
    );

    if (data == NULL) {
        throw std::runtime_error(
            std::format("Could not load the image, {}", path.string())
        );
    }

    std::vector<std::byte> image{
        data, data + width * height * desired_channels
    };

    stbi_image_free(data);

    return {image, width, height};
}

#endif

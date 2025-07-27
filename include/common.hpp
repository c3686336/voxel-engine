#ifndef COMMON_H
#define COMMON_H

#define DAG

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <memory>
#include <utility>

#include <spdlog/spdlog.h>

#include <stb_image.h>

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

std::string load_file(const std::filesystem::path& path);

template <typename T>
struct DeepHash {
	std::size_t operator()(const std::shared_ptr<const T>& t) const noexcept {
		return std::hash<T>(*t);
	}
};

#endif

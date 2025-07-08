#ifndef COMMON_H
#define COMMON_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <spdlog/spdlog.h>

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

std::string load_file(const std::filesystem::path& path);

#endif

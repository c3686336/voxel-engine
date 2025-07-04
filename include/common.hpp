#ifndef COMMON_H
#define COMMON_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <spdlog/spdlog.h>

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

std::string load_file(const std::filesystem::path& path) {
	std::ifstream file(path);

	if (!file.is_open()) {
		SPDLOG_CRITICAL("Could not open the file {}", path.string());
		exit(1);
	}

	std::ostringstream contents;
	contents << file.rdbuf();

	SPDLOG_INFO("File loaded. Content: {}", contents.str());

	return contents.str();
}

#endif

#include "common.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <spdlog/spdlog.h>

std::string load_file(const std::filesystem::path& path) {
	std::ifstream file(path);

	if (!file.is_open()) {
		SPDLOG_CRITICAL("Could not open the file {}", path.string());
		exit(1);
	}

	std::ostringstream contents;
	contents << file.rdbuf();

	// SPDLOG_INFO("File loaded. Content: {}", contents.str());

	return contents.str();
}

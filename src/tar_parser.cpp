#include "tar_parser.hpp"
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

size_t parse_octal(const char *octal_str, size_t max_chars) {
  std::string str(octal_str, max_chars);
  try {
    return std::stoul(str, nullptr, 8);
  } catch (...) {
    return 0;
  }
}

void parse_tar(const fs::path &tar_path) {
  std::ifstream file(tar_path, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Failed to open archive " << tar_path << "\n";
    return;
  }

  while (1) {
    TarHeader header;

    // Read 512 Bytes into the space allocated for header
    file.read(reinterpret_cast<char *>(&header), sizeof(header));

    if (!file) {
      break;
    }

    // Check if we reached the end of archieve
    if (header.name[0] == '\0') {
      std::cout << "Reached the end of archive\n";
      break;
    }

    // This will convert the header.name (char array) into clean C++ string
    std::string file_name(header.name);

    // Get the file size (parse_octal (const char*, n) -> size
    size_t file_size = parse_octal(header.size, sizeof(header.size));

    std::cout << "Found File: " << file_name << " (" << file_size
              << " bytes)\n";

    size_t blocks_to_skip = (file_size + 511) / 512;

    // Move the file reading cursor forward on the disk
    file.seekg(blocks_to_skip * 512, std::ios::cur);
  }
}

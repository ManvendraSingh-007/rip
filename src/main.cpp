#include "../include/tar_parser.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

int main() {
  // File location
  fs::path archive_path = "test_env/sample.tar";

  std::ifstream file(archive_path, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Could not open " << archive_path << "\n";
    return 1;
  }

  parse_tar(archive_path);

  return 0;
}

#include "../include/colors.hpp"
#include "../include/tar_parser.hpp"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
  // File location

  if (argc == 2) {
    std::cerr << BOLD << RED << "fatal:" << RESET
              << " destination path required\n";
    return 1;
  }
  if (argc == 1) {
    std::cerr << BOLD << RED << "fatal:" << RESET
              << " source and destination path required\n";
    return 1;
  }

  // fs::path archive_path = "test_env/sample.tar";

  fs::path archive_path = argv[1];
  fs::path destination_path = argv[2];

  if (!fs::exists(archive_path)) {
    std::cerr << BOLD << RED << "fatal:" << RESET << " Target file '"
              << archive_path.string() << "' does not exist!\n";
    return 1;
  }

  parse_tar(archive_path, destination_path);

  return 0;
}

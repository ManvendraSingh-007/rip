#include "../include/arg_parser.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

ArgParser::ArgParser() : verbose(false), force(false), error(false) {
  destination_dir = "./"; // default destination if none is provided
};

bool isArchiveFile(const std::string &filename) {
  return filename.ends_with(".tar") || filename.ends_with(".tar.gz") ||
         filename.ends_with(".tar.bz2") || filename.ends_with(".tgz");
}

void ArgParser::long_flags(std::string &flag) {
  if (flag == "extract")
    extract = true;
  if (flag == "verbose")
    verbose = true;
  if (flag == "force")
    force = true;
}

void ArgParser::parse_arg(int argc, char *argv[]) {
  if (argc < 2) {
    error_msg = "Error: no arguments provided\n";
    error = true;
    return;
  }

  std::vector<std::string> args(argv + 1, argv + argc);

  for (auto &i : args) {
    // Check for target files
    if (isArchiveFile(i)) {
      source_file = i;
      if (!fs::exists(source_file)) {
        source_file = "";
        error_msg = "Error: source file not found\n";
        error = true;
      }
    }

    // Possible destination path - create if not present
    if (!fs::is_directory(i)) {
      try {
        fs::create_directories(i);
        destination_dir = i;
      } catch (fs::filesystem_error &e) {
        std::cerr << e.what();
      }
    }

    if (i[0] == '-') {

      if (i.length() == 1) {
        error_msg = "Error: invalid use of flag";
        error = true;
        return;
      }

      // long flags
      if (i[1] == '-') {
        std::string flag = i.substr(2, i.length() - 2);
        long_flags(flag);
      }

      else if (i[1] != '-') {
        for (int idx = 1; idx < i.length() - 1; ++idx) {
          if (i[idx] == 'x')
            extract = true;
          if (i[idx] == 'v')
            verbose = true;
          if (i[idx] == 'f')
            force = true;
        }
      }

      else {
        error_msg = "Error: invalid usage, try --help";
        error = true;
        return;
      }
    }
  }
}

void ArgParser::print_usage() const {
  std::cout << "Usage: rip [Options...] [files...]\n"
            << "Options:\n"
            << "  -x              Extract mode\n"
            << "  -v, --verbose   Verbose output\n"
            << "  -f, --force     Force overwrite\n";
}

std::string ArgParser::get_source_file() const { return source_file; }
std::string ArgParser::get_destination_dir() const { return destination_dir; }
bool ArgParser::is_verbose() const { return verbose; }
bool ArgParser::is_force() const { return force; }
bool ArgParser::is_extract() const { return extract; }
bool ArgParser::has_error() const { return error; }
std::string ArgParser::get_error() const { return error_msg; }

#include "../include/tar_parser.hpp"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <ostream>
#include <string>

namespace fs = std::filesystem;

void show_progress(std::string &task_name, double current, double total) {
  double percentage = 0.0;
  int bar_width = 50;
  int progress_block = 0;

  if (total > 0) {
    percentage = (current / total) * 100.0;
    progress_block = static_cast<int>((current / total) * bar_width);
  } else {
    percentage = 100.0;
    progress_block = bar_width;
  }

  std::string bar = "[";
  for (int i = 0; i < progress_block; ++i) {
    if (i < progress_block) {
      bar += "#";
    } else
      bar += " ";
  }

  bar += "]";

  std::cout << "\r" << task_name << " " << bar << " "
            << static_cast<int>(percentage) << "%" << std::flush;
}

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

  // Extracted destination path
  fs::path output = "test_env/output";
  fs::create_directories(output);

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

    fs::path out_file_path = output / file_name;
    fs::create_directories(out_file_path.parent_path());

    std::ofstream out_file(out_file_path, std::ios::binary);
    if (!out_file.is_open()) {
      std::cerr << "Error could not create output files!\n";
      return;
    }

    std::string task = "Extracting " + file_name;
    if (task.length() > 80) {
      task = task.substr(0, 77) + "...";
    } else {
      int padding = 80 - task.length();
      task += std::string(padding, ' ');
    }

    // Read 512 Bytes into the buffer
    char buffer[512];
    size_t bytes_left_to_read = file_size;

    // Total '512' Blocks occupied by file
    size_t total_blocks = (file_size + 511) / 512;

    if (file_size == 0) {
      show_progress(task, 0, 0);
      std::cout << '\n';
    }

    for (int i = 0; i < total_blocks; ++i) {
      file.read(buffer, 512);

      // Number of Actual bytes to write
      size_t bytes_to_write =
          (bytes_left_to_read > 512) ? 512 : bytes_left_to_read;

      // write from the buffer to file
      out_file.write(buffer, bytes_to_write);
      bytes_left_to_read -= bytes_to_write;

      // Calculate how many actual payload bytes have been written so far
      size_t bytes_written_so_far = file_size - bytes_left_to_read;

      show_progress(task, static_cast<double>(bytes_written_so_far),
                    static_cast<double>(file_size));

      if (i == total_blocks - 1) {
        std::cout << '\n';
      }
    }

    out_file.close();
  }
}

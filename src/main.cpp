#include "../include/colors.hpp"
#include "../include/tar_parser.hpp"
#include "arg_parser.hpp"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {

  ArgParser parser;

  engine(&parser);

  return 0;
}

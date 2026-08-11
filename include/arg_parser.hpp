#pragma once
#include "rip_archive.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

struct Options {
  bool extract;
  bool verbose;
  bool list;
  bool pack;
};

class Parser {
  std::vector<char> charFlags;
  std::vector<std::string> detailedFlags;
  std::vector<std::string> sourceFiles;
  std::vector<std::string> mislaneousFlags;

public:
  inline void argParser(int argc, char *argv[]);
  inline void execParsed();
  inline void help();
};

inline void Parser::help() {
  std::cout << "Usage: rip [OPTION]... [FILES]... \n\n"
               "-x, --extract : Extract (rip) file\n"
               "-l, --list    : List archive contents\n"
               "-v, --verbose : Verbose output\n"
               "-p, --pack    : Pack files into archive\n";
}

inline void Parser::argParser(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Invalid usgae. use --help \n";
    return;
  }

  int index = 1;
  while (index < argc) {

    std::string cOption = argv[index];
    std::size_t len = cOption.length();

    if (cOption[0] == '-' && argv[index][1] != '-') {
      // single flags like -a, -b, -x ...
      if (len == 2) {
        charFlags.push_back(argv[index][1]);
      } else if (len > 2) {
        int i = 0;
        while (i < (len - 1)) {
          charFlags.push_back(argv[index][i + 1]);
          ++i;
        }
      }
    }

    if (cOption.substr(0, 2) == "--" && argv[index][2] != '-') {
      detailedFlags.push_back(cOption.substr(2));
    }
    if (len > 4) {
      if (cOption.substr(len - 3) == "rip" && len > 4) {
        sourceFiles.push_back(cOption);
      }
    }

    else
      mislaneousFlags.push_back(cOption);

    ++index;
  }
  execParsed();
}

inline void Parser::execParsed() {
  // std::unordered_map<char, uint8_t> flagStatus;
  Options option;
  for (auto &f : charFlags) {
    if (f == 'x')
      option.extract = true;
    if (f == 'l')
      option.list = true;
    if (f == 'v')
      option.verbose = true;
  }
}

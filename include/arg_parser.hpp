#pragma once
#include "rip_archive.hpp"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

struct Options {
  bool extract = false;
  bool verbose = false;
  bool list = false;
  bool pack = false;
  bool help = false;
};

class Parser {
  std::vector<char> charFlags;
  std::vector<std::string> detailedFlags;
  std::vector<std::string> sourceFiles;
  std::vector<std::string> mislaneousFlags;
  Options opts;

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

    if (cOption.rfind("--", 0) == 0) {
      if (len > 2) {
        detailedFlags.push_back(cOption.substr(2));
      }
      continue; // Move to next argument safely
    }

    if (cOption[0] == '-') {
      // single flags like -a, -b, -x ...
      int i = 0;
      while (i < (len - 1)) {
        charFlags.push_back(argv[index][i + 1]);
        ++i;
      }
      continue;
    }

    if (len > 4 && cOption.substr(len - 4) == ".rip") {
      sourceFiles.push_back(cOption);
      continue;
    }

    mislaneousFlags.push_back(cOption);
    ++index;
  }
  execParsed();
}

inline void Parser::execParsed() {
  namespace fs = std::filesystem;

  std::vector<fs::path> directories;
  std::vector<std::string> dFiles;

  for (const auto &x : mislaneousFlags) {
    fs::path path(x);

    if (fs::is_directory(path)) {
      directories.push_back(path);
    }

    if (fs::exists(path)) {
      dFiles.push_back(x);
    } else {
      fs::create_directory(fs::path(x).parent_path());
    }
  }

  for (char flag : charFlags) {
    switch (flag) {
    case 'x':
      opts.extract = true;
      break;
    case 'v':
      opts.verbose = true;
      break;
    case 'l':
      opts.list = true;
      break;
    case 'p':
      opts.pack = true;
      break;
    case 'h':
      opts.help = true;
      break;
    }
  }

  for (const auto &flag : detailedFlags) {
    if (flag == "extract")
      opts.extract = true;
    if (flag == "verbose")
      opts.verbose = true;
    if (flag == "list")
      opts.list = true;
    if (flag == "pack")
      opts.pack = true;
    if (flag == "help")
      opts.help = true;
  }

  if (opts.help) {
    help();
    return;
  }

  if (opts.extract) {
    if (opts.pack) {
      std::cerr << "Invalid flag usage, see --help for more information\n";
      return;
    }
    if (sourceFiles.empty()) {
      std::cerr << "Error : no rip file provided\n";
      return;
    }

    for (const auto &a : sourceFiles) {
      if (opts.verbose) {
        extractArchive(a)
      }
    }
  }
}

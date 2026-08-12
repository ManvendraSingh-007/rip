#pragma once
#include "rip_archive.hpp"
#include <cstddef>
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
      ++index;
      continue; // Move to next argument safely
    }

    if (cOption[0] == '-') {
      // single flags like -a, -b, -x ...
      int i = 0;
      while (i < (len - 1)) {
        charFlags.push_back(argv[index][i + 1]);
        ++i;
      }
      ++index;
      continue;
    }

    if (len > 4 && cOption.substr(len - 4) == ".rip") {
      sourceFiles.push_back(cOption);
      ++index;
      continue;
    }

    mislaneousFlags.push_back(cOption);
    ++index;
  }
  execParsed();
}

inline void Parser::execParsed() {
  namespace fs = std::filesystem;

  std::vector<std::string> directories;
  std::vector<std::string> dFiles;

  for (const auto &x : mislaneousFlags) {
    fs::path path(x);
    fs::file_status fStatus = fs::status(path);

    if (fs::is_directory(fStatus)) {
      directories.push_back(x);
    } else if (fs::is_regular_file(fStatus)) {
      dFiles.push_back(x);
    } else {
      if (path.has_extension()) {
        dFiles.push_back(x);
      } else {
        directories.push_back(x);
      }
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
    if (directories.empty()) {
      for (const auto &a : sourceFiles) {
        if (opts.list) {
          listArchive(a);
          std::cout << '\n';
        }
        if (opts.verbose)
          extractArchive(a, {'v'});
        if (!opts.verbose) {
          extractArchive(a);
        }
      }
    } else if (!directories.empty()) {
      size_t n1 = sourceFiles.size();
      size_t n2 = directories.size();
      if (n2 > n1) {
        std::cerr << "Too many destination paths specified.\n";
      }
      if (n1 == n2) {
        for (int i = 0; i < n1; ++i) {
          if (opts.list) {
            listArchive(sourceFiles[i]);
            std::cout << '\n';
          }
          if (opts.verbose)
            extractArchive(sourceFiles[i], {'v'}, directories[i]);
          if (!opts.verbose) {
            extractArchive(sourceFiles[i], {}, directories[i]);
          }
        }
      }
      if (n1 > n2) {
        // loop n2 time (no of destination)
        for (int i = 0; i < n2; ++i) {
          // last iteration
          if (i == n2 - 1) {
            // extract the rest of source files into last destination path
            for (int j = i; j < sourceFiles.size(); ++j) {
              if (opts.list) {
                listArchive(sourceFiles[j]);
                std::cout << '\n';
              }
              if (opts.verbose)
                extractArchive(sourceFiles[j], {'v'}, directories[i]);
              if (!opts.verbose) {
                extractArchive(sourceFiles[j], {}, directories[i]);
              }
            }
          }
          // extract in respective destination
          if (opts.list) {
            listArchive(sourceFiles[i]);
            std::cout << '\n';
          }
          if (opts.verbose)
            extractArchive(sourceFiles[i], {'v'}, directories[i]);
          if (!opts.verbose) {
            extractArchive(sourceFiles[i], {}, directories[i]);
          }
        }
      }
    }
  }
}

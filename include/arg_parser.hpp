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

inline std::string get_short_timestamp() {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%H%M%S");
  return ss.str();
}

inline void Parser::execParsed() {
  namespace fs = std::filesystem;

  std::vector<std::string> directories;
  std::vector<std::string> dFiles;

  std::vector<std::string> argModes;

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

  if (!sourceFiles.empty() && opts.list && !opts.extract && !opts.pack) {
    for (const auto& a: sourceFiles) {
      listArchive(a, opts.verbose ? std::vector<char>{'v'} : std::vector<char>{});
      std::cout << '\n';
    }
  }

  if (opts.extract) {
    if (opts.pack) {
      std::cerr << "Invalid flag combination of x and p, see --help for more "
                   "information\n";
      return;
    }
    if (sourceFiles.empty()) {
      std::cerr << "Error : no rip file provided\n";
      return;
    }
    if (directories.empty()) {
      // no dirs provided
      // so extact them to same folder named as rip file
      // -x abc.rip -> extact to abc
      for (const auto &a : sourceFiles) {
        if (opts.list) {
          listArchive(a);
          std::cout << '\n';
        }
        if (opts.verbose)
          extractArchive(a, {'v'}, a.substr(0, a.length() - 4));
        if (!opts.verbose) {
          extractArchive(a, {}, a.substr(0, a.length() - 4));
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

  if (opts.pack) {
    if (opts.extract) {
      std::cerr << "Invalid flag combination of p and x, see --help for more "
                   "information\n";
      return;
    }

    if (dFiles.empty() && directories.empty()) {
      std::cerr << "Error : no files provided\n";
      return;
    }
    if (!directories.empty()) {
      // user might have provided dir to pack instead of files

      if (sourceFiles.empty()) {
        for (int i = 0; i < directories.size(); ++i) {
          if (opts.verbose)
            createArchive(directories[i] + ".rip", {directories[i]}, {'v'});
          if (!opts.verbose)
            createArchive(directories[i] + ".rip", {directories[i]});
        }
      } else if (!sourceFiles.empty()) {
        size_t n1 = directories.size();
        size_t n2 = sourceFiles.size();
        if (n2 > n1) {
          std::cerr << "Too many destination paths specified.\n";
        }
        if (n1 == n2) {
          for (int i = 0; i < n1; ++i) {
            if (opts.verbose)
              createArchive(sourceFiles[i], {directories[i]}, {'v'});

            if (!opts.verbose) {
              createArchive(sourceFiles[i], {directories[i]});
            }
          }

          if (n1 > n2) {
            // loop n2 time (no of archive path specified)
            for (int i = 0; i < n2; ++i) {
              // last iteration
              if (i == n2 - 1) {
                // pack the rest of folder into last archive
                for (int j = i; j < sourceFiles.size(); ++j) {
                  if (opts.verbose)
                    createArchive(sourceFiles[i],
                                  {directories.begin() + j, directories.end()},
                                  {'v'});
                  if (!opts.verbose)
                    createArchive(sourceFiles[i],
                                  {directories.begin() + j, directories.end()});
                }
              }
              // pack in respective destination
              if (opts.verbose)
                createArchive(sourceFiles[i], {directories[i]}, {'v'});
              if (!opts.verbose)
                createArchive(sourceFiles[i], {directories[i]});
            }
          }
        }
      }
    }

    if (!dFiles.empty()) {
      if (sourceFiles.empty()) {
        if (opts.verbose)
          createArchive("RIP" + get_short_timestamp() + ".rip", dFiles, {'v'});
        if (!opts.verbose)
          createArchive("RIP" + get_short_timestamp() + ".rip", dFiles);

      } else if (!sourceFiles.empty()) {
        size_t n1 = dFiles.size();
        size_t n2 = sourceFiles.size();
        if (n2 > n1) {
          std::cerr << "Too many destination paths specified.\n";
        }
        if (n1 == n2) {
          for (int i = 0; i < n1; ++i) {
            if (opts.verbose)
              createArchive(sourceFiles[i], {dFiles[i]}, {'v'});

            if (!opts.verbose) {
              createArchive(sourceFiles[i], {dFiles[i]});
            }
          }

          if (n1 > n2) {
            // loop n2 time (no of archive path specified)
            for (int i = 0; i < n2; ++i) {
              // last iteration
              if (i == n2 - 1) {
                // pack the rest of files into last archive
                for (int j = i; j < sourceFiles.size(); ++j) {
                  if (opts.verbose)
                    createArchive(sourceFiles[i],
                                  {dFiles.begin() + j, dFiles.end()}, {'v'});
                  if (!opts.verbose)
                    createArchive(sourceFiles[i],
                                  {dFiles.begin() + j, dFiles.end()});
                }
              }
              // pack in respective destination
              if (opts.verbose)
                createArchive(sourceFiles[i], {dFiles[i]}, {'v'});
              if (!opts.verbose)
                createArchive(sourceFiles[i], {dFiles[i]});
            }
          }
        }
      }
    }
  }
}

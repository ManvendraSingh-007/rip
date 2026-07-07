#pragma once
#include <string>

class ArgParser {
private:
  std::string source_file;
  std::string destination_dir;
  std::string error_msg;
  bool extract = false;
  bool verbose = false;
  bool force = false;
  bool error = false;

public:
  ArgParser();

  void parse_arg(int argc, char *argv[]);
  void long_flags(std::string &flag);

  std::string get_source_file() const;
  std::string get_destination_dir() const;
  bool is_verbose() const;
  bool is_force() const;
  bool is_extract() const;

  bool has_error() const;
  std::string get_error() const;
  void print_usage() const;
};

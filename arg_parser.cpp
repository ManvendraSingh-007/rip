#include <iostream>

void printUsage(const char *programName) {
  std::cout
      << "Usage: " << programName << " <archive.tar> [options]\n"
      << "Options:\n"
      << "  -h, --help            Show this help text\n"
      << "  -v, --verbose         Enable verbose output logs\n"
      << "  -d, --destination <dir> Specify target extraction directory\n";
}

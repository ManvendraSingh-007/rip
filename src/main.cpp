#include "../include/rip_archive.hpp"
#include <string>
#include <vector>

int main() {
  std::string archivePath = "test_env/rip.rip";
  // std::vector<std::string> inputPaths = {
  //   "../test_env/1.tiff", "../test_env/2.tiff", "../test_env/3.tiff",
  //"../test_env/4.tiff", "../test_env/5.tiff"};

  std::vector<std::string> inputPaths = {"test_env/main.py",
                                         "test_env/game.py"};

  createArchive(archivePath, inputPaths);
  listArchive(archivePath);
  return 0;
}

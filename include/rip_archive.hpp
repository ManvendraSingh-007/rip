#include <string>
#include <vector>
#pragma once

void createArchive(const std::string &archivePath,
                   const std::vector<std::string> &inputPaths);
void listArchive(const std::string &archivePath);

#include <string>
#include <vector>
#pragma once

void createArchive(const std::string &archivePath,
                   const std::vector<std::string> &inputPaths,
                   const std::vector<char> modes = {});
void listArchive(const std::string &archivePath, const std::vector<char> modes = {});
void extractArchive(const std::string &archivePath,
                    const std::vector<char> modes = {},
                    const std::string &outputDir = ".");

void runBenchmarkTest(std::string &archivePath, std::string &outputDir);

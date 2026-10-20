// rip_archive.cpp
// A minimal, from scratch archive format ".rip" using std::c++
//
// File layout
//
//   char     magic[4]      = "RIP1"
//   uint16_t version       = 1
//   uint32_t entryCount
//   --- --- --- --- ---
//   TOC = Table of content
//   --- --- --- --- ---
//   uint16_t nameLen
//   char     name[nameLen]
//   uint64_t dataOffset     // absolute offset from start of file
//   uint64_t dataSize       // uncompressed size in bytes
//   uint32_t crc32          // checksum of the uncompressed data
//   uint8_t  flags          // reserved (bit0 = compressed, unused)
//   --- --- --- --- ---
//   [raw file bytes, back to back, one per entry, in TOC order]

#include "rip_archive.hpp"
#include "col.hpp"
#include "crc32.hpp"
#include "rip.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <print>
#include <ratio>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>
#include <vector>

std::vector<ArchiveEntry>
filesToWork(const std::vector<std::string> &inputPaths) {

  std::vector<ArchiveEntry> filesToPack;

  for (const auto &p : inputPaths) {
    std::filesystem::path path(p);
    std::error_code ec;

    if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
      std::filesystem::path basePath = path.parent_path();

      for (const auto &entry :
           std::filesystem::recursive_directory_iterator(path)) {
        if (std::filesystem::is_regular_file(entry.status())) {
          // Compute relative path from the parent of the input folder
          // Ex: input = "dir", entry = "dir/sub/a.txt" -> "dir/sub/a.txt"
          std::filesystem::path internalPath =
              std::filesystem::relative(entry.path(), basePath);
          filesToPack.push_back({entry.path(), internalPath});
        } else {
          WARN("Skipping : not a regular file {}", p);
        }
      }
      continue;
    }

    if (!std::filesystem::exists(path, ec)) {
      WARN("File does not exist: {}", p);
      WARN("Creating File: {}", p);

      std::ofstream newFile(path);
      newFile.close();

      if (!std::filesystem::exists(path, ec)) {
        ERROR("Failed to create file: {}", p);
        continue;
      }
      filesToPack.push_back({path, path.filename()});
      continue;
    }

    if (std::filesystem::is_regular_file(path, ec)) {
      filesToPack.push_back({path, path.filename()});
      continue;
    }

    WARN("Skipping (not a regular file): {}", p);
  }

  return filesToPack;
}

void createArchive(const std::string &archivePath,
                   const std::vector<std::string> &inputPaths,
                   const std::vector<char> modes) {

  // Gather file Metadata
  bool verbose = false;
  for (const auto &m : modes) {
    if (m == 'v')
      verbose = true;
  }

  const auto filesToPack = filesToWork(inputPaths);
  if (filesToPack.empty()) {
    WARN("No files found to pack");
    return;
  }
  std::vector<Entry> entries;
  entries.reserve(filesToPack.size());

  for (const auto &f : filesToPack) {
    Entry e;
    e.fullPath = f.diskPath;
    e.nameLen = e.fullPath.size();
    e.dataSize = std::filesystem::file_size(f.diskPath);

    entries.push_back(std::move(e));
  }

  if (verbose) {
    INFO("Creating archive: {}", archivePath);
    INFO("Scanning {} input paths...", inputPaths.size());
  }

  // Compute toc + data offsets
  size_t toc = 0;
  for (const auto &e : entries)
    toc += 8 + 8 + 4 + 1 + 1 + e.fullPath.size();

  uint64_t runningOffset = kHeaderSize + toc;
  for (auto &e : entries) {
    e.dataOffset = runningOffset;
    runningOffset += e.dataSize;
  }

  // Writing to archive
  std::ofstream out(archivePath, std::ios::binary);
  if (!out) {
    throw std::runtime_error("[CRITICAL] Cannot open archive for writing: " +
                             archivePath);
  }

  // Writing header - magic, version, entryCount
  out.write(kMagic, 4);
  writeLE<uint16_t>(out, kVersion);
  writeLE<uint32_t>(out, static_cast<uint32_t>(entries.size()));

  // Calculating crc32 checksum
  if (verbose && !entries.empty()) {
    INFO("Calculating CRC32 checksums...");
  }

  constexpr size_t kBufferSize = 1 << 16;
  uint8_t buffer[kBufferSize];
  for (auto &e : entries) {
    if (e.dataSize == 0) {
      e.crc32 = 0;
      continue;
    }

    std::ifstream in(e.fullPath, std::ios::binary);
    if (!in) {
      throw std::runtime_error(
          "[CRITICAL] Failed to open input file for CRC calculation: " +
          e.fullPath);
    }

    e.crc32 = 0;
    while (in) {
      in.read(reinterpret_cast<char *>(buffer), kBufferSize);
      std::streamsize got = in.gcount();
      if (got <= 0)
        break;
      e.crc32 = crc.compute(buffer, got, e.crc32);
    }
  }

  // Writing TOC
  if (verbose) {
    INFO("Writing Table of Contents...");
  }
  for (const auto &e : entries) {
    writeLE<uint64_t>(out, e.dataOffset);
    writeLE<uint64_t>(out, e.dataSize);
    writeLE<uint32_t>(out, e.crc32);
    writeLE<uint8_t>(out, e.flags);
    writeLE<uint8_t>(out, e.nameLen);
    out.write(e.fullPath.data(), e.fullPath.size());
  }

  // Writing file data
  if (verbose && !entries.empty()) {
    INFO("Writing file payloads...");
  }
  for (const auto &e : entries) {
    if (e.dataSize == 0)
      continue;

    if (verbose) {
      INFO("  -> Packing: {}", e.fullPath);
    }

    std::ifstream in(e.fullPath, std::ios::binary);
    if (!in) {
      throw std::runtime_error(
          "[CRITICAL] Input file went missing or became unreadable: " +
          e.fullPath);
    }

    while (in) {
      in.read(reinterpret_cast<char *>(buffer), kBufferSize);
      std::streamsize got = in.gcount();
      if (got <= 0)
        break;
      out.write(reinterpret_cast<const char *>(buffer), got);
    }

    if (!out) {
      throw std::runtime_error(
          "[CRITICAL] Disk write failure while creating archive.");
    }
  }

  SUCCESS("Archive successfully created at: {}", archivePath);
  INFO("Packaged {} files. Total size: {} bytes.", entries.size(),
       runningOffset);
}

std::vector<Entry> readToc(char *mappedArchive) {
  char magic[4];
  std::memcpy(magic, "RIP1", 4);
  if (std::memcmp(magic, mappedArchive, 4) != 0) {
    throw std::runtime_error("[CRITICAL] Invalid archive magic signature");
  }
  mappedArchive += 4;

  uint16_t version = *readFromArchiveMapped<uint16_t>(mappedArchive);
  if (version != kVersion) {
    throw std::runtime_error("[CRITICAL] Unsupported archive version: " +
                             std::to_string(version));
  }

  uint32_t entryCount = *readFromArchiveMapped<uint32_t>(mappedArchive);
  std::vector<Entry> entries(entryCount);
  for (auto &e : entries) {
    e.dataOffset = *readFromArchiveMapped<uint64_t>(mappedArchive);
    e.dataSize = *readFromArchiveMapped<uint64_t>(mappedArchive);
    e.crc32 = *readFromArchiveMapped<uint32_t>(mappedArchive);
    e.flags = *readFromArchiveMapped<uint8_t>(mappedArchive);
    e.nameLen = *readFromArchiveMapped<uint8_t>(mappedArchive);
    char *namePtr = readFromArchiveMapped<char>(mappedArchive, e.nameLen);

    e.fullPath.assign(namePtr, e.nameLen);
  }

  return entries;
}

void listArchive(const std::string &archivePath,
                 const std::vector<char> modes) {
  bool verbose = false;
  for (const auto &m : modes) {
    if (m == 'v')
      verbose = true;
  }

  int fd = open(archivePath.c_str(), O_RDONLY);
  if (fd == -1) {
    throw std::runtime_error("[CRITICAL] Cannot open archive file: " +
                             archivePath);
  }

  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    ERROR("Failed to retrieve archive metadata");
    close(fd);
    return;
  }

  std::size_t fileSize = sb.st_size;
  if (fileSize == 0) {
    return;
  }

  char *mmappedArchive =
      (char *)mmap(NULL, fileSize, PROT_READ, MAP_SHARED, fd, 0);

  close(fd);
  madvise(mmappedArchive, fileSize, MADV_SEQUENTIAL);

  auto entries = readToc(mmappedArchive);
  munmap(mmappedArchive, fileSize);

  INFO("Archive contains {} entr{}", entries.size(),
       (entries.size() == 1 ? "y" : "ies"));

  for (const auto &e : entries) {
    INFO("  {} ({} bytes)", e.fullPath, e.dataSize);
    if (verbose) {
      INFO("    Offset: {}, CRC32: 0x{:08X}", e.dataOffset, e.crc32);
    }
  }
}

void extractArchive(const std::string &archivePath,
                    const std::vector<char> modes,
                    const std::string &outputDir) {
  bool verbose = false;
  for (const auto &m : modes) {
    if (m == 'v')
      verbose = true;
  }

  if (verbose) {
    INFO("Mapping source archive: {}", archivePath);
  }

  int fd = open(archivePath.c_str(), O_RDONLY);
  if (fd == -1) {
    ERROR("Failed to open archive descriptor");
    return;
  }

  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    ERROR("Failed to retrieve archive metadata");
    close(fd);
    return;
  }

  std::size_t fileSize = sb.st_size;
  if (fileSize == 0) {
    return;
  }

  char *mappedArchive =
      (char *)mmap(nullptr, fileSize, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);

  madvise(mappedArchive, fileSize, MADV_SEQUENTIAL);

  auto entries = readToc(mappedArchive);

  if (verbose) {
    INFO("Table of contents parsed successfully. Found {} entries.",
         entries.size());
  }

  if (!std::filesystem::exists(std::filesystem::path(outputDir))) {
    if (verbose) {
      INFO("Creating output directory tree: {}", outputDir);
    }
    std::filesystem::create_directories(outputDir);
  }

  auto startTime = std::chrono::high_resolution_clock::now();
  for (const auto &e : entries) {
    std::filesystem::path outPath =
        std::filesystem::path(outputDir) / e.fullPath;

    if (!std::filesystem::exists(outPath.parent_path())) {
      std::filesystem::create_directories(outPath.parent_path());
    }

    int outFd =
        open(outPath.string().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outFd == -1) {
      ERROR("Failed to write target output descriptor: {}", outPath.string());
      continue;
    }

    write(outFd, mappedArchive + e.dataOffset, e.dataSize);
    close(outFd);
    uint32_t eCrc32 = 0;

    if (verbose) {
      INFO("Extracting node : {} -> {}({} bytes) ", e.fullPath,
           outPath.string(), e.dataSize);
    }

    eCrc32 = crc.compute(
        reinterpret_cast<const unsigned char *>(mappedArchive + e.dataOffset),
        static_cast<size_t>(e.dataSize), eCrc32);

    if (eCrc32 != e.crc32) {
      WARN("Integrity check failed for {} (Expected: 0x{:08X}, Computed: "
           "0x{:08X})",
           e.fullPath, e.crc32, eCrc32);
    } else if (verbose) {
      OK("Integrity verified for {} (CRC32 matches)", e.fullPath);
    }
  }

  munmap(mappedArchive, fileSize);
  auto endTime = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> durationMs = endTime - startTime;

  SUCCESS("Extraction finished executing");
  if (verbose) {
    INFO("Performance profile execution : {:.2f} ms ({:.4f} seconds)",
         durationMs.count(), durationMs.count() / 1000.0);
  }
}

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

#include "../include/rip_archive.hpp"
#include "../include/crc32.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

// Little-endian binary I/O helpers (portable across host endianness)
template <typename T> void writeLE(std::ostream &os, T value) {
  static_assert(std::is_unsigned<T>::value, "writeLE expects unsigned int");

  // Loop sizeof(T) times
  for (size_t i = 0; i < sizeof(T); ++i) {

    // Grab the current byte
    // i=0: shift 0 bits to right (gets original lowest byte)
    // i=1: shift 8 bits to right (brings the next byte into position), and so
    // on cast to uint8_t automatically chops off and drops any higher bits
    //
    // 00000000......01101011
    // (higher)      (lower)
    uint8_t byte = static_cast<uint8_t>(value >> (8 * i));
    os.write(reinterpret_cast<const char *>(&byte), 1);
  }
}

template <typename T> T readLE(std::istream &is) {
  static_assert(std::is_unsigned<T>::value, "readLE expects unsigned int");

  // final number
  T value = 0;

  for (size_t i = 0; i < sizeof(T); ++i) {
    uint8_t byte = 0;

    // Pull exactly 1 raw byte out of the stream
    is.read(reinterpret_cast<char *>(&byte), 1);
    if (!is) {
      throw std::runtime_error("Unexpected EOF while reading archive");
    }

    // 1. Cast the single byte up to the full size of T.
    // 2. Shift it left into its proper slot (0 bits for byte 1, 8 bits for byte
    // 2, etc.).
    // 3. Glue it into the final 'value' using a bitwise OR.
    value |= static_cast<T>(byte) << (8 * i);
  }

  return value;
}

// TOC
struct Entry {
  std::string name;
  std::string fullPath;
  uint64_t dataOffset = 0;
  uint64_t dataSize = 0;
  uint32_t crc32 = 0;
  uint8_t nameLen = 0;
  uint8_t flags = 0;
};

constexpr char kMagic[4] = {'R', 'I', 'P', '1'};
constexpr uint16_t kVersion = 1;
constexpr size_t kHeaderSize = 4 + 2 + 4; // magic + version + entryCount

static const Crc32 crc;
void createArchive(const std::string &archivePath,
                   const std::vector<std::string> &inputPaths) {

  // Gather file Metadata
  std::vector<Entry> entries;
  entries.reserve(inputPaths.size());

  std::cout << "[INFO] Creating archive: " << archivePath << "\n";
  std::cout << "[INFO] Scanning " << inputPaths.size() << " input paths...\n";

  for (const auto &p : inputPaths) {
    std::filesystem::path path(p);
    std::error_code ec;

    if (!std::filesystem::exists(path, ec)) {
      std::cerr << "[WARN] Skipping (does not exist): " << p << "\n";
      continue;
    }

    if (!std::filesystem::is_regular_file(path, ec)) {
      std::cerr << "[WARN] Skipping (not an regular file): " << p << "\n";
      continue;
    }

    uint64_t size = std::filesystem::file_size(path, ec);
    if (ec) {
      std::cerr << "[WARN] Skipping (cannot read size): " << p
                << " Error: " << ec.message() << "\n";
      continue;
    }

    Entry e;
    e.name = path.filename().string();
    e.nameLen = e.name.size();
    e.dataSize = std::filesystem::file_size(path);
    e.fullPath = p;
    entries.push_back(std::move(e));
  }

  // Compute toc + data offsets
  size_t toc = 0;
  for (const auto &e : entries)
    toc += 8 + 8 + 4 + 1 + 1 + e.name.size();

  uint64_t runningOffset = kHeaderSize + toc;
  for (auto &e : entries) {
    e.dataOffset = runningOffset;
    runningOffset += e.dataSize;
  }

  // Writing to archive
  std::ofstream out(archivePath, std::ios::binary);
  if (!out)
    std::runtime_error("[CRITICAL] Cannot open archive for writing " +
                       archivePath + "\n");

  // Writing header - magic, version, entryCount
  out.write(kMagic, 4); // Single byte type; no endian conversion needed
  writeLE<uint16_t>(out, kVersion);
  writeLE<uint32_t>(out, static_cast<uint32_t>(entries.size()));

  // Calculating crc32 checksum
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
  for (const auto &e : entries) {
    writeLE<uint64_t>(out, e.dataOffset);
    writeLE<uint64_t>(out, e.dataSize);
    writeLE<uint32_t>(out, e.crc32);
    writeLE<uint8_t>(out, e.flags);
    writeLE<uint8_t>(out, e.nameLen);
    out.write(e.name.data(), e.name.size());
  }

  // Writing file data
  for (const auto &e : entries) {
    if (e.dataSize == 0)
      continue;

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

  std::cout << "[SUCCESS] Archive successfully created at: " << archivePath
            << "\n";
  std::cout << "[SUMMARY] Packaged " << entries.size()
            << " files. Total size: " << runningOffset << " bytes.\n";
}

std::vector<Entry> readToc(std::ifstream &in) {
  char magic[4];
  in.read(magic, 4);
  if (!in || std::memcmp(magic, kMagic, 4) != 0) {
    throw std::runtime_error(
        "[CRITICAL] Not an valip .rip archive (bad magic)");
  }

  uint16_t version = readLE<uint16_t>(in);
  if (version != kVersion)
    throw std::runtime_error("[CRITICAL] Unsupported archive version : " +
                             std::to_string(version));

  uint32_t entryCount = readLE<uint32_t>(in);
  std::vector<Entry> entries(entryCount);
  for (auto &e : entries) {
    e.dataOffset = readLE<uint64_t>(in);
    e.dataSize = readLE<uint64_t>(in);
    e.crc32 = readLE<uint32_t>(in);
    e.flags = readLE<uint8_t>(in);
    e.nameLen = readLE<uint8_t>(in);
    e.name.resize(e.nameLen);
    in.read(e.name.data(), e.nameLen);
  }

  return entries;
}

void listArchive(const std::string &archivePath) {
  std::ifstream in(archivePath, std::ios::binary);
  if (!in)
    throw std::runtime_error("[CRITICAL] Cannot open archive: " + archivePath);

  auto entries = readToc(in);
  std::cout << entries.size() << " entr" << (entries.size() == 1 ? "y" : "ies")
            << ":\n";
  for (const auto &e : entries) {
    std::cout << "  " << e.name << "  (" << e.dataSize << " bytes, crc32=0x"
              << std::hex << e.crc32 << std::dec << ")\n";
  }
}

void extractArchive(const std::string &archivePath,
                    const std::string &outputDir) {
  std::ifstream in(archivePath, std::ios::binary);
  if (!in)
    throw std::runtime_error("[CRITICAL] Cannot open archive: " + archivePath);

  auto entries = readToc(in);
  std::filesystem::create_directories(outputDir);

  constexpr size_t bufferSize = 1 << 16;
  uint8_t buffer[bufferSize];
  for (const auto &e : entries) {
    in.seekg(static_cast<std::streamsize>(e.dataOffset));
    if (e.dataSize > 0) {
      in.read(reinterpret_cast<char *>(buffer), bufferSize);
    }
  }
}

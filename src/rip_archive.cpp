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

#include "../../include/crypto/crc32.hpp"
#include <cstddef>
#include <cstdint>
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
  uint64_t dataOffset = 0;
  uint64_t dataSize = 0;
  uint32_t crc32 = 0;
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

  for (const auto &p : inputPaths) {
    std::filesystem::path path(p);
    if (!std::filesystem::is_regular_file(path)) {
      std::cerr << "Skipping (not an regular file): " << p << "\n";
      continue;
    }

    Entry e;
    e.name = path.filename().string();
    e.dataSize = std::filesystem::file_size(path);
    entries.push_back(std::move(e));
  }

  // Compute toc + data offsets
  size_t toc = 0;
  for (const auto &e : entries)
    toc += 8 + 8 + 4 + 1 + e.name.size();

  uint64_t runningOffset = kHeaderSize + toc;
  for (auto &e : entries) {
    e.dataOffset = runningOffset;
    runningOffset += e.dataSize;
  }

  // Writing header + TOC
  std::ofstream out(archivePath, std::ios::binary);
  if (!out)
    std::runtime_error("Cannot open archive for writing " + archivePath + "\n");

  // Writing header - magic, version, entryCount
  out.write(kMagic, 4); // Single byte type; no endian conversion needed
  writeLE<uint16_t>(out, kVersion);
  writeLE<uint32_t>(out, static_cast<uint32_t>(entries.size()));

  for (size_t i = 0; i < entries.size(); ++i) {
    if (!std::filesystem::is_regular_file(inputPaths[i]))
      continue;
    std::ifstream in(inputPaths[i], std::ios::binary);
    uint8_t buffer[1 << 16];

    entries[i].crc32 = 0;
    if (entries[i].dataSize > 0) {
      while (in) {
        in.read(reinterpret_cast<char *>(buffer), (1 << 16));
        std::streamsize got = in.gcount();
        if (got <= 0)
          break;
        entries[i].crc32 = crc.compute(buffer, got, entries[i].crc32);
      }
    }
  }

  for (const auto &e : entries) {
    writeLE<uint64_t>(out, e.dataOffset);
    writeLE<uint64_t>(out, e.dataSize);
    writeLE<uint32_t>(out, e.crc32);
    out.write(e.name.data(), e.name.size());
  }

  for (size_t i = 0; i < entries.size(); ++i) {
    std::ifstream in(inputPaths[i], std::ios::binary);
    char buffer[1 << 16];
    while (in) {
      in.read(reinterpret_cast<char *>(buffer), (1 << 16));
      std::streamsize got = in.gcount();
      if (got <= 0)
        break;
      out.write(reinterpret_cast<const char *>(buffer), got);
    }
  }
}

// rip_archive.cpp
// A minimal, from scratch archive format ".rip" using std::c++
//
// File layout
//
//   char     magic[4]      = "RIP1"
//   uint16_t version       = 1
//   uint32_t entryCount
//   --- --- --- --- ---
//   uint16_t nameLen
//   char     name[nameLen]
//   uint64_t dataOffset     // absolute offset from start of file
//   uint64_t dataSize       // uncompressed size in bytes
//   uint32_t crc32          // checksum of the uncompressed data
//   uint8_t  flags          // reserved (bit0 = compressed, unused)
//   --- --- --- --- ---
//   [raw file bytes, back to back, one per entry, in TOC order]

#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>

// ---------------------------------------------------------------------------
// Little-endian binary I/O helpers (portable across host endianness)
// ---------------------------------------------------------------------------
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

struct Entry {
  std::string name;
  uint64_t dataOffset = 0;
  uint64_t dataSize = 0;
  uint32_t crc32 = 0;
  uint8_t flags = 0;
};

constexpr char kMagic[4] = {'R', 'I', 'P', '1'};
constexpr uint16_t kVersion = 1;
constexpr size_t kHeaderSize = 4 + 2 + 4;

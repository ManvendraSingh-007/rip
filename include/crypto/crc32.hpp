// crc32.cpp
//
// ---------------------------------------------------------------------------
// CRC32 (standard polynomial 0xEDB88320), built by hand — no external lib.
// ---------------------------------------------------------------------------

#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

class Crc32 {
public:
  Crc32() {
    for (uint32_t i = 0; i < 256; ++i) {
      uint32_t c = i;
      for (int k = 0; k < 8; ++k)
        c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);

      table_[i] = c;
    }
  }

  uint32_t compute(const uint8_t *data, size_t len) const {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i)
      crc = table_[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
  }

private:
  std::array<uint32_t, 256> table_{};
};

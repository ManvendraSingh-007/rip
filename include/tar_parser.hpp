#pragma once
#include <filesystem>
#include <string>

// The standard POSIX TAR header structure (512 bytes total)
struct TarHeader {
  char name[100];     // File name
  char mode[8];       // File permissions (octal string)
  char uid[8];        // User ID (octal string)
  char gid[8];        // Group ID (octal string)
  char size[12];      // File size in bytes (octal string!)
  char mtime[12];     // Modification time
  char chksum[8];     // Header checksum
  char typeflag;      // File type ('0' = regular file, '5' = directory)
  char linkname[100]; // Name of linked file
  char magic[6];      // Always "ustar" (indicates POSIX format)
  char version[2];    // Ustar version
  char uname[32];     // User name
  char gname[32];     // Group name
  char devmajor[8];   // Device major number
  char devminor[8];   // Device minor number
  char prefix[155];   // Prefix for long file names
  char padding[12];   // Padded to fit 512 bytes perfectly
};

size_t parse_octal(const char *octal_str, size_t max_chars);
void parse_tar(const std::filesystem::path &tar_path);

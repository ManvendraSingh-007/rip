# rip

A fast, tiny command-line file extractor written in modern C++, uses standard c++ libraries and currently optimized for extracting `.tar` archives.

## Features

- **Lightweight & Fast:** Built using zero-copy block buffering (`file.read`) for rapid disk transfers.
- **Cross-Platform Support:** Linux, macOS, and modern Windows terminals.

## Getting Started

### Prerequisites
C++ compiler supporting **C++17** or higher (gcc, clang, or msvc)

### Compilation
Clone the repository and compile the source code using your preferred compiler:

```bash
cmake --build /build
```

### Usage
To extract a standard TAR archive, simply execute `rip` followed by the path to the target file and detination source directory:

```bash
./rip ~/path_to/archive.tar ~/detination/
```
## Roadmap
- [x] Streamlined TAR block parser
- [ ] Support for gzip decompressed streams (`.tar.gz`)
- [ ] Multi-thread un-archiving pipelines

## License
This project is licensed under the MIT License.

# rip

A fast, tiny command-line file extractor written in modern C++, uses standard c++ libraries, it was originally started as an generic file extractor targeting common compressed files like `tar`, but now i am working on creating custom `.rip` archive and an custom `.rip` extractor.

## Features

- **Cross-Platform Support:** Linux, macOS, and modern Windows terminals.

## Getting Started

### Prerequisites
C++ compiler supporting **C++20** or higher (gcc, clang, or msvc)

### Compilation
Clone the repository and compile the source code using your preferred compiler:

```bash
cmake -S . -B build && cmake --build build
```

### Usage
--

```bash
./rip ~/path_to/archive.tar ~/detination/
```
## Roadmap
- [x] Designig custom `.rip` archive
- [ ] Support for compression and extraction
- [ ] Multi-thread un-archiving pipelines

## License
This project is licensed under the MIT License.

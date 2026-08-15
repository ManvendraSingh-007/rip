# rip

A fast, tiny command-line file extractor written in modern C++, uses standard c++ libraries, it was originally started as an generic file extractor targeting common compressed files like `tar`, but now i am working on creating custom `.rip` archive and an custom `.rip` extractor.

## Features

- **Cross-Platform Support:** Linux, macOS, and modern Windows terminals.
- **Flexible cli-arguments:** Support flexible arguments to ease the usage.

## Getting Started

### Prerequisites
C++ compiler supporting **C++20** or higher (gcc, clang, or msvc)

### Compilation
Clone the repository and compile the source code using your preferred compiler:

```bash
cmake -S . -B build && cmake --build build
```

### Usage

```bash
.build/rip [FLAGS...] [FILES...]
```

# Functionalities
### support these arguments right now

```bash
-x | --extract : extract any rip file
```
```bash
-p | --pack    : pack any rip file
```
```bash
-l | --list    : list the content of rip file
```
```bash
-v | --verbose : support detailed verbose
```

## Roadmap
- [x] Designig custom `.rip` archive
- [ ] Support for compression and extraction
- [ ] Multi-thread un-archiving pipelines

## License
This project is licensed under the MIT License.

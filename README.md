# rip

`rip` is a small command-line archiving tool written in C++. It creates and extracts a custom `.rip` archive format for grouping files and folders into one file, and then recovering them later.

I started this project as a simple archive utility and now made it evolve into a custom format with a basic table of contents, file metadata, and CRC32 checks during extraction. It is a lightweight tool for local file packaging rather than a full replacement for formats like `zip` or other.

## What it does

- packs one or more files or directories into a `.rip` archive
- extracts a `.rip` archive to a chosen output location
- lists the files stored inside an archive
- keeps the original relative paths when packing folders
- validates file integrity with CRC32 while extracting
- supports a simple verbose mode for output/debugging

## Current behavior

The program is built around a custom binary archive layout:

- header with archive magic and version
- table of contents with file offsets and sizes
- per-file CRC32 value
- raw file payloads written one after another

This makes it easy to store multiple files in a single archive without depending on external libraries.

## Build

Prerequisites:

- C++ compiler with C++20 support
- CMake

From the project root:

```bash
cmake -S . -B build
cmake --build build
```

This produces the `rip` executable in the build directory.

## Usage

Show help:

```bash
./build/rip --help
```

Pack files into an archive:

```bash
./build/rip -p my_folder
./build/rip -p file1.txt file2.txt
```

This creates files like `my_folder.rip` or a generated archive path based on the input.

Extract an archive:

```bash
./build/rip -x archive.rip
./build/rip -x archive.rip output_folder
```

List archive contents:

```bash
./build/rip -l archive.rip
```

Verbose output:

```bash
./build/rip -lv archive.rip
./build/rip -xv archive.rip
```

Support flexible arguments
```bash
./build/rip --extract a.rip --verbose -l
./build/rip -xvl b.rip ~/dump
./build/rip -x a.rip b.rip c.rip --verbose ~/dump1 --list ~/dump2
``` 


## Example

```bash
./build/rip -p project/assets
./build/rip -l project/assets.rip
./build/rip -x project/assets.rip extracted_assets
```

## Notes

- archive format is custom and meant for this project and wont work for other archives.
- It does not currently implement compression, encryption, or advanced archive features.

## License

This project is licensed under the MIT License.

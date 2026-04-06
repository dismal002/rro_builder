# RRO Builder CLI (C++)

A standalone C++ command-line tool to build and deploy Android Runtime Resource Overlays (RRO), based on the **Privset** application.

## Prerequisites

- **Compiler**: `g++` or `clang++` with C++17 support.
- **Build System**: `make`.
- **Android SDK Tools** (must be in your `PATH`):
  - `aapt2`
  - `zipalign`
  - `apksigner`
  - `adb`

## Setup & Compilation

1. Clone or copy the source files.
2. Compile the binary:
   ```bash
   make
   ```
3. Ensure `debug.pk8` and `debug.x509.pem` are present in the same directory.
4. (Optional) Provide your own `framework-res.apk` if you don't want the tool to pull it from your device.

## Usage

Simply run the compiled binary:
```bash
./rro_builder
```

### Workflow
1.  **Interactive Prompts**: The tool will ask if you want to add Boolean, Integer, or String values.
2.  **Editor Integration**: For each selection, it will open your system's default text editor (e.g., `nano`, `vim`, or `gedit`).
3.  **Automatic Build**: The tool generates the necessary XML files, compiles them using `aapt2`, signs the APK, and aligns it.
4.  **Deployment**: It optionally pushes the APK to a connected rooted device and enables the overlay.

## Project Structure
- `rro_builder.cpp`: Main source code.
- `Makefile`: Build configuration.
- `debug.pk8` & `debug.x509.pem`: Default signing keys.
- `dist/`: Generated APKs will be saved here.

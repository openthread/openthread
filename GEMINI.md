# OpenThread (`openthread`) GEMINI.md

## Project Overview

OpenThread is an open-source implementation of the Thread networking protocol, released by Google. It is designed to be OS and platform-agnostic, with a small memory footprint, making it highly portable. It supports both system-on-chip (SoC) and network co-processor (NCP) designs and is a Thread Certified Component.

The project is primarily written in C and C++, with Python used for scripting and tooling. It uses a variety of build systems, including CMake and GN, and is actively maintained with a strong emphasis on code quality and style, enforced through continuous integration.

## Building and Running

The project uses both CMake and GN as build systems. A collection of scripts in the `script/` directory simplifies the build and test process.

### Prerequisites

- **Compilers:** `gcc` and `clang`
- **Build tools:** `make`, `ninja-build`
- **Other tools:** `python3`, `yapf` (v0.43.0), `clang-format` (v19), `pylint`, `shellcheck`, `iwyu`

A bootstrap script is provided to install the required tools:

```bash
./script/bootstrap
```

### Building with CMake

The project provides CMake presets for easier configuration.

**Configure:**

```bash
cmake --preset simulation
```

**Build:**

```bash
cmake --build --preset simulation
```

### Building with Scripts

The `script/` directory contains several scripts for building the project for different configurations:

- **Simulation Build:**
  ```bash
  script/check-simulation-build
  ```
- **POSIX Build:**
  ```bash
  script/check-posix-build
  ```
- **ARM Build:**
  ```bash
  script/check-arm-build
  ```
- **GN Build:**
  ```bash
  script/check-gn-build
  ```

## Testing

The project uses CTest for testing.

To run the tests after building with the simulation preset:

```bash
ctest --preset simulation
```

## Development Conventions

### Code Style

The project has a strict coding style, which is enforced by the `script/make-pretty` script. Before submitting a pull request, ensure your code is formatted correctly.

**Check code style:**

```bash
script/make-pretty check
```

**Format code:**

```bash
script/make-pretty
```

Key style points:

- **Indentation:** 4 spaces.
- **Naming:**
  - `UpperCamelCase` for types (classes, structs, enums), methods, functions.
  - `lowerCamelCase` for variables.
  - `g` prefix for globals, `s` for statics, `m` for members, `a` for arguments.
- **Comments:** Doxygen is used for API documentation.

For more details, see the [STYLE_GUIDE.md](STYLE_GUIDE.md).

### Commits and Pull Requests

The project follows the "Fork-and-Pull" model. All contributions must be accompanied by a Contributor License Agreement (CLA). Pull requests are tested using GitHub Actions, and all checks must pass before merging.

For more details, see the [CONTRIBUTING.md](CONTRIBUTING.md).

# OpenThread Coding Agent Instructions

## Repository Overview

OpenThread is an open-source implementation of the Thread networking protocol released by Google. It's a Thread Certified Component implementing all features defined in Thread 1.4.0 specification, including IPv6, 6LoWPAN, IEEE 802.15.4 with MAC security, Mesh Link Establishment, and Mesh Routing. The codebase is OS and platform agnostic with a narrow platform abstraction layer.

**Project Type:** Embedded networking protocol stack (C/C++)  
**Languages:** C (C99), C++ (C++11), Python 3, Shell scripts  
**Build Systems:** CMake (primary), GN (secondary)  
**Size:** ~200k LOC excluding third-party code  
**Target Platforms:** SoC and NCP designs (simulation, POSIX, various embedded platforms)

## Critical Build Requirements

### Formatting Tools (REQUIRED)
**ALWAYS run formatting checks before submitting changes.** The CI will reject code that doesn't pass `script/make-pretty check`.

Required versions:
- **clang-format v19** (NOT v18 or v20) - for C/C++ formatting
- **yapf v0.31.0** - for Python formatting  
- **shfmt** - for shell script formatting
- **prettier@2.0.4** - for markdown formatting

Install formatting tools:
```bash
sudo bash script/install-llvm.sh  # Installs clang-format v19
python3 -m pip install yapf==0.31.0
```

### Build Dependencies
```bash
sudo apt-get update
sudo apt-get install -y g++ cmake ninja-build libgtest-dev libgmock-dev
```

## Build Instructions

### Standard Build Process

**For simulation platform (most common for testing):**
```bash
./script/cmake-build simulation
```

**For POSIX platform:**
```bash
./script/cmake-build posix
```

Build artifacts are placed in `build/<platform>/` directory.

**Build time:** Initial build takes ~3-4 minutes. Incremental builds are faster.

### Custom Build Options

Pass CMake options after the platform:
```bash
./script/cmake-build simulation -DOT_FULL_LOGS=ON -DOT_CHANNEL_MANAGER=OFF
```

Build specific targets:
```bash
OT_CMAKE_NINJA_TARGET="ot-cli-ftd" ./script/cmake-build simulation
```

### Important Build Notes

1. **ALWAYS clean build when switching platforms or major configuration changes:**
   ```bash
   rm -rf build/
   ```

2. **The build directory is NOT tracked by git** (in .gitignore). Never commit build artifacts.

3. **Warnings are errors:** The build uses `-DOT_COMPILE_WARNING_AS_ERROR=ON` by default. Fix all warnings.

## Testing

### Unit Tests

Run all unit tests:
```bash
cd build/simulation
ctest --output-on-failure
```

Run a specific test:
```bash
./tests/unit/ot-test-<name>
```

**Test time:** Full unit test suite takes ~1-2 minutes.

### Thread Certification Tests

For Thread 1.4 certification tests:
```bash
./script/test build
./script/test unit
./script/test cert_suite tests/scripts/thread-cert/v1_2_*
```

These tests require additional Python dependencies:
```bash
python3 -m pip install -r tests/scripts/thread-cert/requirements.txt
```

## Code Formatting and Style

### Automatic Formatting

**Format all code before committing:**
```bash
script/make-pretty
```

**Check formatting without modifying files:**
```bash
script/make-pretty check
```

Format specific file types:
```bash
script/make-pretty clang        # C/C++ only
script/make-pretty python       # Python only
script/make-pretty markdown     # Markdown only
script/make-pretty shell        # Shell scripts only
```

### Style Requirements (from STYLE_GUIDE.md)

**C/C++ Conventions:**
- **Standards:** C99 for C, C++11 for C++
- **Indentation:** 4 spaces (NO tabs)
- **Naming:**
  - Classes/types/functions: `UpperCamelCase`
  - Variables/parameters: `lowerCamelCase`
  - Parameters prefix: `a` (e.g., `aLength`)
  - Members prefix: `m` (e.g., `mCount`)
  - Globals prefix: `g` (e.g., `gInstance`)
  - Statics prefix: `s` (e.g., `sInitialized`)
  - Preprocessor symbols: `ALL_UPPERCASE`
- **Braces:** On their own lines
- **Header guards:** Required in all headers, format: `FILENAME_HPP_`
- **Includes order:**
  1. Core config header first (if private file)
  2. Corresponding .h for .cpp files
  3. C++ standard library
  4. C standard library
  5. Third-party libraries
  6. First-party libraries
  7. Private headers
  8. Alphabetically within each group

**Python Conventions:**
- Follow Google Python Style Guide
- Max line length: 119 characters
- Use `yapf` for formatting

**Critical:** OpenThread avoids heap allocation, exceptions, RTTI, virtual functions (minimized), and C++ STL in constrained systems.

## CI/CD Checks

The following GitHub Actions workflows will run on every PR. **Your changes must pass ALL of these:**

### 1. Pretty Check (build.yml)
**Most common failure point.** Checks code formatting.
```bash
script/make-pretty check
```

### 2. Unit Tests (unit.yml)
Runs all unit tests on Ubuntu.
```bash
./script/cmake-build simulation
cd build/simulation && ctest
```

### 3. Simulation Tests (simulation-1.4.yml, simulation-1.1.yml)
Thread certification test suites for different versions.

### 4. Build Checks (build.yml)
- CMake preset validation
- ARM toolchain builds
- POSIX builds
- Size checks
- Various platform-specific builds

### 5. Static Analysis
- **clang-tidy:** Static analysis (may take 10+ minutes)
- **CodeQL:** Security scanning
- **Include-What-You-Use (IWYU):** Header dependency checks

### Common CI Failure Patterns

**Problem:** "script/make-pretty check" fails  
**Solution:** Run `script/make-pretty` locally and commit the changes

**Problem:** Build warnings treated as errors  
**Solution:** Fix the warning. Cannot be suppressed. Common issues:
- Unused variables (use `OT_UNUSED_VARIABLE(var)` macro)
- Missing header guards
- Incorrect include order

**Problem:** Test timeout in CI  
**Solution:** Long-running tests have timeouts. Ensure your changes don't introduce infinite loops or very slow operations.

## Project Layout

### Key Directories

```
openthread/
├── src/                    # Core implementation
│   ├── core/              # Protocol stack core (Thread, 6LoWPAN, MAC, etc.)
│   ├── cli/               # Command-line interface
│   ├── ncp/               # Network Co-Processor implementation
│   ├── posix/             # POSIX platform implementation
│   └── lib/               # Shared libraries
├── include/openthread/    # Public API headers (Doxygen documented)
├── examples/              # Example applications and platform implementations
│   ├── apps/              # Example applications (CLI, NCP, RCP)
│   └── platforms/         # Platform-specific code (simulation, etc.)
├── tests/                 # Test suites
│   ├── unit/              # Unit tests (Google Test based)
│   ├── scripts/           # Test automation scripts
│   │   └── thread-cert/   # Thread certification tests
│   ├── toranj/            # Integration tests
│   └── fuzz/              # Fuzzing tests
├── third_party/           # External dependencies
│   ├── mbedtls/           # Crypto library
│   └── tcplp/             # TCP protocol
├── script/                # Build and utility scripts
├── tools/                 # Development tools
├── doc/                   # Documentation and Doxygen config
└── .github/workflows/     # CI/CD configuration
```

### Configuration Files

- **CMakeLists.txt** (root): Main CMake configuration
- **CMakePresets.json**: CMake presets for common build configurations
- **BUILD.gn**: GN build configuration (secondary build system)
- **.clang-format**: C/C++ formatting rules (clang-format v19)
- **.clang-tidy**: Static analysis configuration
- **.prettierrc**: Markdown formatting rules
- **STYLE_GUIDE.md**: Comprehensive coding standards

### Header File Organization

Public API headers are in `include/openthread/*.h` with Doxygen comments. These are the ONLY headers external applications should include. Internal headers are in `src/core/` and subdirectories.

## Common Workflows

### Adding a New Feature

1. Make code changes following the style guide
2. Add/update tests in `tests/unit/` or `tests/scripts/thread-cert/`
3. Update documentation if adding public APIs (Doxygen comments)
4. Format code: `script/make-pretty`
5. Build: `./script/cmake-build simulation`
6. Test: `cd build/simulation && ctest`
7. Verify checks pass: `script/make-pretty check`

### Fixing a Bug

1. Add a test that reproduces the bug
2. Fix the bug
3. Verify the test passes
4. Format code: `script/make-pretty`
5. Run full test suite

### Modifying Public APIs

**Extra care required:**
- Update Doxygen comments in header files
- Check API version compatibility (`script/check-api-version`)
- Update related documentation
- Ensure backward compatibility when possible

## Build Troubleshooting

**Problem:** "ninja: error: loading 'build.ninja'"  
**Solution:** CMake configuration didn't complete. Re-run `./script/cmake-build <platform>`

**Problem:** mbedTLS configuration errors  
**Solution:** This is auto-generated. Clean build: `rm -rf build/ && ./script/cmake-build simulation`

**Problem:** "undefined reference" errors during linking  
**Solution:** Likely missing a CMake dependency or source file. Check `CMakeLists.txt` in the relevant directory.

**Problem:** clang-format version mismatch  
**Solution:** MUST use v19. Install via `sudo bash script/install-llvm.sh`

## Best Practices

1. **Trust these instructions.** They are comprehensive and verified. Only search for additional information if something is unclear or these instructions are found to be incorrect.

2. **Always format before committing.** Run `script/make-pretty` - it's fast and prevents CI failures.

3. **Build incrementally.** Don't wait until you've made many changes to test the build.

4. **Run unit tests frequently** during development to catch issues early.

5. **Check CI logs carefully.** If a check fails, download artifacts from the GitHub Actions page for detailed logs.

6. **Avoid third-party directory.** Don't modify files in `third_party/` unless absolutely necessary.

7. **Use existing patterns.** Look at similar existing code for guidance on structure and style.

8. **Minimal changes.** Make the smallest possible change to achieve your goal.

9. **Read STYLE_GUIDE.md** for detailed conventions if working on complex C++ changes.

10. **Bootstrap script is optional** for agents. The `script/bootstrap` is for setting up a dev environment from scratch. If you have working build tools (cmake, ninja, gcc/clang), you can skip it.

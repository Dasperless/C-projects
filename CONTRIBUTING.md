# Contributing

## Prerequisites

- CMake 3.20 or newer
- A C11 compiler (GCC 9+ or Clang 10+)
- POSIX threads (`pthread`)
- Optional: `clang-format` for style checks

## Style

- Language standard: C11 (`-std=c11`, `CMAKE_C_EXTENSIONS OFF`).
- Every translation unit must build cleanly with `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`.
- Format rules live in [`.clang-format`](.clang-format) and [`.editorconfig`](.editorconfig) (4-space indentation, 100-column limit, K&R braces). Run `clang-format -i` on modified files before committing.
- Public headers must have `#ifndef ... #define ... #endif` include guards and document ownership / error returns.

## Branch and commit conventions

- Use focused branches per issue: `feat/issue-<n>-short-slug`, `fix/...`, `docs/...`.
- Keep pull requests small and single-purpose. If a feature depends on an earlier unmerged branch, stack it on that branch and state the merge order in the PR description.
- Commit messages follow `<type>(<scope>): <summary> (#<issue>)` (`feat`, `fix`, `refactor`, `test`, `docs`, `ci`).

## Verification checklist (run before opening a PR)

```bash
# Configure and build with AddressSanitizer + UndefinedBehaviorSanitizer
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -DENABLE_UBSAN=ON
cmake --build build

# Run the entire CTest suite
ctest --test-dir build --output-on-failure
```

All tests must pass with zero compiler warnings and zero sanitizer findings.

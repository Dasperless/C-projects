# Contributing to Dasperless/C-projects

Thank you for your interest in contributing to `Dasperless/C-projects`!

## Branching Model
1. Fork and create a dedicated feature branch for each change:
   `git checkout -b feat/your-feature-name`
2. Follow the issue-driven workflow referencing relevant issue IDs.

## Code Style & Formatting
- Code style is formatted with `clang-format` based on the LLVM style guidelines (4-space indent, 100 column limit).
- Run `clang-format -i <file>` prior to submitting.
- Avoid deprecated functions (`bzero`, `inet_addr`). Use modern POSIX standard functions (`memset`, `inet_pton`).

## Build & Testing
- Ensure the project builds cleanly with `-Wall -Wextra -Wpedantic -Werror`.
- Run all unit and E2E tests:
  ```bash
  python3 tests/e2e/runner.py
  ```

## Submitting Pull Requests
- Provide a clear, descriptive title and summary of changes.
- Ensure all CI matrix checks pass (GCC, Clang, sanitizers).

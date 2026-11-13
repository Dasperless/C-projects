# Repository Guidelines for Agents

## Build, Test, and Verification Matrix

Every code change must pass both `Release` (`-O3`, which enables aggressive inlining and `-Werror=format-truncation=`) and `Debug` (`ASan` + `UBSan`) builds, plus `clang-format` and `cppcheck`:

```bash
# 1. Release build (catches -O3 inlining and -Werror=format-truncation=)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_WERROR=ON
cmake --build build
ctest --test-dir build --output-on-failure

# 2. Debug build with AddressSanitizer + UndefinedBehaviorSanitizer
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_WERROR=ON -DENABLE_ASAN=ON -DENABLE_UBSAN=ON
cmake --build build
ctest --test-dir build --output-on-failure

# 3. Formatting check
find chatroom structures \( -name '*.c' -o -name '*.h' \) | xargs clang-format --dry-run --Werror

# 4. Static analysis (if cppcheck is installed)
cppcheck --error-exitcode=1 --enable=warning,performance,portability --inline-suppr --suppress=missingIncludeSystem -I chatroom/includes chatroom/ structures/
```

## Coding & Portability Rules

- **Strict C11 & Warnings as Errors**: All targets compile with `-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`.
- **Format Truncation (`-O3` / Release)**: When formatting strings with `snprintf`, ensure source buffers or `%.*s` precision bounds are provably smaller than the destination buffer so GCC `-O3` inlining does not trigger `-Werror=format-truncation=`.
- **Cross-Platform Socket & Thread Wakeups (Linux + macOS)**:
  - On macOS (BSD sockets), calling `shutdown(listen_fd, SHUT_RDWR)` on a listening socket returns `ENOTCONN` and does **not** wake a thread blocked in `accept(listen_fd, ...)`. When requesting server shutdown from another thread (such as `admin_console_thread`), signal the main thread (`pthread_kill(main_tid, SIGTERM)`) so `accept()` is interrupted with `EINTR` on both Linux and macOS.
  - Never leave detached threads blocked in `fgets(..., stdin)` during process teardown; poll `STDIN_FILENO` with `poll()` and join the console thread cleanly on shutdown so AddressSanitizer/LeakSanitizer exit checks do not deadlock.
  - In multi-process test helpers (`chatroom/tests/test_util.h`), redirect child `STDIN_FILENO` to `/dev/null` unless the test explicitly pipes console commands.
- **Cppcheck Compliance in Tests**:
  - Do not place expressions with side effects (`fwrite`, `fread`, `fgetc`, `sscanf`, `system`) inside `assert(...)` (`[assertWithSideEffect]`); assign the return value to a local variable first and assert on the variable.
  - Always check or assert that `malloc`/`calloc` return values are non-`NULL` before dereferencing (`[nullPointerOutOfMemory]`).

## Git Branch & Pull Request Conventions

- **Branch Naming**: Every new branch must follow the format `<type>/<small-description>` (e.g., `fix/console-shutdown`, `feat/bst-delete`, `docs/readme-quickstart`).
- **Pull Request Titles**: All pull requests must use Conventional Commits format in the title (`<type>(<scope>): <summary>` or `<type>: <summary>`).

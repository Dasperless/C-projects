# AGENTS.md

This document defines the operational ruleset and engineering standards for AI assistants working on the `C-projects` repository, grounded in *Process Over Magic: Beyond Vibe Coding*.

AI assistants must follow these rules strictly. No shortcuts, no guesswork, and no persona roleplaying.

---

## 1. Interaction Rules

* **Concrete Instructions Over Personas**: Do not adopt theatrical personas ("You are an expert who never makes mistakes"). Focus exclusively on concrete constraints, exact inputs, and verifiable outputs.
* **Clarify Ambiguity**: If a user request or specification is ambiguous, underspecified, or conflicting, pause and ask targeted clarifying questions before writing code.
* **Explain Infeasibility**: If a requested change is technically unfeasible or risks architectural decay, explain why directly and propose viable alternatives instead of guessing.
* **Structured Communication**: Use clean, concise Markdown formatting (bullet points, precise code snippets, headers). Avoid conversational fluff and excessive preamble.
* **Scope Discipline**: Modify only code and files strictly required for the immediate task. Do not touch adjacent files, rename unrelated symbols, or reformat untouched files.
* **Style Conformance**: Respect and match the existing coding style, idiomatic C11 patterns, and conventions of the codebase (`.clang-format` and `.editorconfig`).

---

## 2. Coding Standards & Simple Design

### The TDD Feedback Loop
* **Tests Are the Primary Feedback Loop**: AI cannot execute software in its head. Automated tests (`ctest`) and compiler/sanitizer diagnostics provide the objective feedback signal that keeps code grounded.
* **Mandatory Automated Tests for All Work**: Agents must create new automated tests (or update existing test suites) for every single feature, bugfix, or protocol/data-structure modification. Submitting code without corresponding automated tests is strictly prohibited (`chatroom/tests/` and `structures/*/tests/`).
* **Meaningful Assertions Only**: Every test must contain real, specific assertions validating actual domain behavior, memory ownership, concurrency invariants, and edge cases. Tests without meaningful assertions or trivial `assert(1)` checks are strictly prohibited.
* **No Duplicated Assertions**: Do not duplicate identical test logic across multiple test cases. Each test must have a distinct purpose.
* **Mandatory Test Retrofit**: If modifying or adding functionality to a module that lacks existing automated tests, you must establish a CTest suite and verify passing tests under AddressSanitizer and UndefinedBehaviorSanitizer before declaring the task complete.

### Kent Beck's Four Rules of Simple Design
All code must adhere to the four rules in strict priority order:
1. **The Code Works**: It compiles cleanly (`-Werror`) in both `Release` and `Debug` (`ASan` + `UBSan`) modes and passes all unit and integration tests.
2. **It Reveals Intent**: Code must be immediately self-explanatory. Use clear domain terminology (`packet_header_t`, `file_offer_t`, `DoublyLinkedList`, `HashTable`) rather than cryptic abbreviations.
3. **No Duplication (DRY)**: Eliminate duplicate logic and copy-pasted structures across protocol framing, file transfer, and data structure modules.
4. **Fewest Elements**: No premature abstractions, unused interfaces, speculative generalization, or unnecessary layers. Write only the code needed right now.

### Explicit & Deterministic C Programming Preferences
* **Explicit over Implicit**: Explicitly pass context and state pointers (`LinkedList *`, `HashTable *`, `fs_receiver_t *`); avoid hidden mutable globals where module-scoped structs suffice.
* **Clear Ownership & Lifetimes**: Document and enforce memory ownership (`*_create` / `*_destroy`, `*_open` / `*_close`). Every allocation must have a deterministic deallocation path verified by ASan/LSan.
* **Bounded Operations over Unbounded**: Never use unbounded string/memory functions (`gets`, `strcpy` on untrusted lengths, `sprintf`). Always bound buffer operations (`snprintf`, `memcpy` after length checks) and validate wire-protocol lengths (`PROTO_MAX_PAYLOAD`, `FS_MAX_FILE_SIZE`).
* **Synchronized Shared State**: Protect all shared mutable state across threads with explicit mutexes (`pthread_mutex_t`) and serialize multi-thread socket writes so application frames never interleave.

---

## 3. Architecture & Module Boundaries

`C-projects` is organized into standalone, modular C11 libraries and executables. Maintain strict modularity and header boundary separation:

| Module / Target | Path | Responsibility |
|---|---|---|
| **`chatroom_protocol`** | `chatroom/protocol.c`, `chatroom/includes/protocol.h` | Length-prefixed 68-byte packet framing, `send_all`/`recv_all`, client CLI parser. |
| **`chatroom_logger`** | `chatroom/logger.c`, `chatroom/includes/logger.h` | Thread-safe append-only ISO-8601 server audit logger with control-char escaping. |
| **`chatroom_fileshare`** | `chatroom/fileshare.c`, `chatroom/includes/fileshare.h` | Chunked 4 KB binary file transfer, CRC32 verification, filename sanitization, `.part` atomic rename. |
| **`chatroom_server`** | `chatroom/server.c` | Multi-threaded TCP chat server, group channels, file transfer relay, interactive admin console. |
| **`chatroom_client`** | `chatroom/client.c` | Multi-threaded TCP chat client CLI with background file sender/receiver state machines. |
| **`singly_linked_list`** | `structures/linked-lists/singly-linked-list/` | Intrusive `Node` API and `LinkedList` container with $O(1)$ ends, in-place reverse, MergeSort. |
| **`doubly_linked_list`** | `structures/linked-lists/doubly-linked-list/` | Bidirectional `DNode` list (`DoublyLinkedList`) with $O(1)$ ends and node splicing. |
| **`vector`** | `structures/vector/` | Geometric-growth ($2\times$) contiguous dynamic array (`Vector`). |
| **`stack` / `queue`** | `structures/stack/`, `structures/queue/` | Dynamic array LIFO `Stack` and circular ring-buffer FIFO `Queue`. |
| **`bst` / `hash_table`** | `structures/binary_search_tree/`, `structures/hash_table/` | Recursive `BST` (3-case deletion, traversals) and DJB2 string-keyed `HashTable` with rehashing. |

### Architectural Rules
* **Modularize by Concern**: Keep wire framing (`protocol.c`), audit logging (`logger.c`), file-transfer codecs (`fileshare.c`), and socket orchestration (`server.c` / `client.c`) strictly separated.
* **Single Responsibility**: Each library, translation unit, struct, and function must have a single, clearly bounded responsibility.
* **Zero Cross-Contamination**: Data structure libraries in `structures/` must remain self-contained with zero dependencies on `chatroom/`.
* **Compact Functions**: Keep functions small, focused, and free of over-engineering.

---

## 4. Workflow & Spec-Driven Development

Development follows a disciplined, reproducible cycle:

```
[Read Spec / Issue] ➔ [Write/Update Tests] ➔ [Implement Code] ➔ [Verify Build & Linter Matrix] ➔ [Update Docs] ➔ [User Review & Commit]
```

1. **Read the Specification First**: Before writing any implementation code, read the active GitHub Issue or specification. Understand the requirements, boundaries, and acceptance criteria.
2. **Test-First Implementation**: Write or update CTest unit/integration tests alongside code. Let test failures guide the implementation.
3. **Green Gate (Full Verification Matrix)**: Run the complete build, sanitizer, formatter, and static-analysis matrix (Section 7). Do not proceed until every check passes with zero warnings or errors.
4. **Keep Documentation in Sync**: If CLI flags, wire protocol packets, admin commands, or public header APIs change, immediately update `README.md`, `chatroom/README.md`, or `structures/README.md`.

---

## 5. Git Branch, Commit & Pull Request Strategy

* **Branch Naming**: Every new branch must follow the format `<type>/<small-description>` (e.g., `fix/console-shutdown`, `feat/bst-delete`, `docs/readme-quickstart`).
* **Pull Request Titles**: All pull requests must use Conventional Commits format in the title (`<type>(<scope>): <summary>` or `<type>: <summary>`).
* **One Prompt = One Commit**: Each prompt/task cycle corresponds to a single, clean, self-contained commit.
* **Only Commit Working Code**: Never commit broken, unformatted, unlinted, or failing code. The full verification matrix must pass 100% prior to committing.
* **Single Focused Goal**: Never mix unrelated refactoring with feature development or bug fixing in the same commit.
* **Conventional Commits & Caveman Commits (50/72 rule, Intent Only)**:
  * Structure: `<type>(<scope>): <subject>`
  * Types: `feat`, `fix`, `test`, `refactor`, `docs`, `chore`, `perf`, `build`, `ci`, `style`.
  * Subject line must be imperative mood and 50 characters or fewer (hard cap 72), with no trailing period.
  * Body only when needed (non-obvious *why*, breaking changes, issue references `Closes #123` / `Refs #123`), wrapped at 72 characters, bullets with `-`.
  * Fluff-free: Never use "This commit does X", "I", "we", "now", "currently", AI attribution trailers, or emojis.
* **Rollback Discipline**: If trapped in an error loop, cascading regressions, or hallucinations, do not pile on speculative patches. Roll back to the last clean commit (`git checkout` / `git reset`) and retry with a smaller step.

---

## 6. Safe Practices & Non-Negotiables

* **Never Modify Test Assertions During Refactoring**: When refactoring code, existing test assertions are inviolable ground truth. Only modify the underlying implementation. If a refactoring breaks a test, the implementation is incorrect.
* **Do Not Skip Failing Tests**: Never disable, comment out, or bypass CTest targets just to achieve a green test run. Fix the underlying root cause.
* **No Invented APIs or Non-Portable Assumptions**: Verify POSIX and C11 standard library signatures and cross-platform behavior (Linux glibc vs. macOS BSD libc/sockets).

---

## 7. Project-Specific Tech & Portability Guidelines

### Build, Test, and Verification Matrix
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

# 4. Static analysis (cppcheck)
cppcheck --error-exitcode=1 --enable=warning,performance,portability --inline-suppr --suppress=missingIncludeSystem -I chatroom/includes chatroom/ structures/
```

### Compiler, Portability & Static Analysis Rules
* **Strict C11 & Warnings as Errors**: All targets compile with `-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror` (`CMAKE_C_EXTENSIONS OFF`). Use `#define _POSIX_C_SOURCE 200809L` (or `_DEFAULT_SOURCE` where needed) before system includes when using POSIX APIs.
* **Format Truncation (`-O3` / Release)**: When formatting strings with `snprintf`, ensure source buffers or `%.*s` precision bounds are provably smaller than the destination buffer so GCC `-O3` function inlining does not trigger `-Werror=format-truncation=`.
* **Cross-Platform Socket & Thread Wakeups (Linux + macOS)**:
  * On macOS (BSD sockets), calling `shutdown(listen_fd, SHUT_RDWR)` on a listening socket returns `ENOTCONN` and does **not** wake a thread blocked in `accept(listen_fd, ...)`. When requesting server shutdown from another thread (such as `admin_console_thread`), signal the main thread (`pthread_kill(main_tid, SIGTERM)`) so `accept()` is interrupted with `EINTR` on both Linux and macOS.
  * Never leave detached threads blocked in `fgets(..., stdin)` during process teardown; poll `STDIN_FILENO` with `poll()` and join the console thread cleanly on shutdown so AddressSanitizer/LeakSanitizer exit checks do not deadlock.
  * In multi-process test helpers (`chatroom/tests/test_util.h`), redirect child `STDIN_FILENO` to `/dev/null` unless the test explicitly pipes console commands.
* **Cppcheck Compliance in Tests**:
  * Do not place expressions with side effects (`fwrite`, `fread`, `fgetc`, `sscanf`, `system`) inside `assert(...)` (`[assertWithSideEffect]`); assign the return value to a local variable first and assert on the variable.
  * Always check or assert that `malloc`/`calloc` return values are non-`NULL` before dereferencing (`[nullPointerOutOfMemory]`).

---

## 8. Goal

> **Produce consistent, safe, testable, and maintainable software.**
> **Stick to the process — no shortcuts, no magic.**

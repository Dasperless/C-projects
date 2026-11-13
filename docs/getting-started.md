# Getting Started

This guide walks through installing prerequisites, building the project in Release and Sanitizer modes, running the multi-threaded chatroom application, and linking the static libraries into your own C or C++ project.

---

## 1. Prerequisites

| Requirement | Minimum Version | Purpose |
|---|---|---|
| **Operating System** | Linux (glibc) or macOS (BSD) | POSIX sockets (`sys/socket.h`), `poll()`, and `pthread` |
| **CMake** | `3.20+` | Out-of-source build generation and CTest orchestration |
| **C Compiler** | GCC `9+` or Clang `10+` | Strict ISO C11 (`-std=c11`) and sanitizer instrumentation |
| **POSIX Threads** | `libpthread` | Multi-threaded server/client concurrency and mutexes |
| **`clang-format`** *(optional)* | `14+` | Enforcing the repository `.clang-format` style |
| **`cppcheck`** *(optional)* | `2.7+` | Static analysis verification |

### Installing on Ubuntu / Debian

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake clang clang-format cppcheck
```

### Installing on macOS (Homebrew)

```bash
brew install cmake llvm clang-format cppcheck
```

---

## 2. Building the Project

> [!IMPORTANT]
> In-source builds (running `cmake .` in the repository root) are explicitly blocked by `CMakeLists.txt` to keep the source tree clean. Always specify an out-of-source build directory such as `-B build`.

### Option A: Using CMake Directly (Recommended)

```bash
# 1. Configure and compile a Release build (-O3, -Werror)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_WERROR=ON
cmake --build build

# 2. Configure and compile a Debug build with AddressSanitizer + UBSan
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_WERROR=ON -DENABLE_ASAN=ON -DENABLE_UBSAN=ON
cmake --build build
```

### Option B: Using the Convenience `Makefile`

The root [`Makefile`](../Makefile) wraps common CMake invocations:

| Make Target | Equivalent Action |
|---|---|
| `make` or `make build` | Configures and builds `Release` in `./build` |
| `make debug` | Configures and builds `Debug` in `./build` |
| `make asan` | Configures and builds `Debug` with `ENABLE_ASAN=ON` and `ENABLE_UBSAN=ON` |
| `make test` | Builds `Release` and runs the full `ctest --output-on-failure` suite |
| `make clean` | Removes `build/`, `bin/`, and `lib/` |

### CMake Configuration Options

| Option | Default | Description |
|---|---|---|
| `CMAKE_BUILD_TYPE` | *(empty)* | `Release` (`-O3 -DNDEBUG`) or `Debug` (`-g -O0`) |
| `ENABLE_WERROR` | `ON` | Treat all compiler warnings as fatal errors (`-Werror`) |
| `ENABLE_ASAN` | `OFF` | Instrument binaries with AddressSanitizer (`-fsanitize=address`) |
| `ENABLE_UBSAN` | `OFF` | Instrument binaries with UndefinedBehaviorSanitizer (`-fsanitize=undefined`) |
| `ENABLE_TSAN` | `OFF` | Instrument binaries with ThreadSanitizer (`-fsanitize=thread`, mutually exclusive with `ENABLE_ASAN`) |
| `BUILD_TESTING` | `ON` | Build and register all 16 CTest unit and integration test executables |

### Build Output Layout

After building, artifacts are placed in deterministic subdirectories under `build/`:

```text
build/
├── bin/
│   ├── chatroom_server           # TCP chatroom server + admin console
│   ├── chatroom_client           # Interactive CLI chatroom client
│   ├── singly_linked_list_demo   # Singly linked list demonstration binary
│   └── test_*                    # 15 unit & integration test binaries
└── lib/
    ├── libchatroom_protocol.a    # Wire framing & CLI input parser
    ├── libchatroom_logger.a      # Thread-safe ISO-8601 audit logger
    ├── libchatroom_fileshare.a   # Chunked file transfer & CRC-32 codec
    ├── libsingly_linked_list.a   # Node & LinkedList container
    ├── libdoubly_linked_list.a   # DoublyLinkedList (dll_t)
    ├── libvector.a               # Dynamic contiguous array (Vector)
    ├── libstack.a                # Dynamic LIFO Stack
    ├── libqueue.a                # Circular FIFO Queue
    ├── libbst.a                  # Binary Search Tree (BST)
    └── libhash_table.a           # DJB2 Separate-Chaining HashTable
```

---

## 3. Running the Test Suite

Run all 16 CTest suites across both `chatroom/` and `structures/`:

```bash
ctest --test-dir build --output-on-failure
```

Run a specific subset of tests by regular expression:

```bash
# Run only data structure unit tests
ctest --test-dir build --output-on-failure -R "singly|dll|vector|stack|queue|bst|hash_table"

# Run only chatroom unit and multi-process integration tests
ctest --test-dir build --output-on-failure -R "logger|fileshare|protocol|chat|file_transfer|admin|concurrency"
```

---

## 4. Walkthrough: Running a Live Chatroom Session

Open three terminal windows after building the project:

### Step 1: Start the Server (Terminal 1)

```bash
./build/bin/chatroom_server 127.0.0.1 8888 server.log
```

The server binds to `127.0.0.1:8888`, initializes the append-only audit log at `server.log`, and starts the interactive administrative console on `stdin`.

### Step 2: Connect Clients (Terminals 2 & 3)

In Terminal 2, connect as `alice`:

```text
$ ./build/bin/chatroom_client 127.0.0.1 8888
Enter your name: alice
=== Connected to server ===
* Welcome! Type /help.
```

In Terminal 3, connect as `bob`:

```text
$ ./build/bin/chatroom_client 127.0.0.1 8888
Enter your name: bob
=== Connected to server ===
* Welcome! Type /help.
```

### Step 3: Exchange Messages and Transfer Files

In `alice`'s terminal:

```text
Hello everyone!                        # Broadcasts public message
/users                                 # Lists connected users (alice, bob)
/msg bob Secret 1-on-1 message         # Private message to bob only
/group devs                            # Joins channel #devs
#devs Pushing hotfix now               # Sends message to #devs members
/sendfile bob ./README.md              # Offers README.md to bob with CRC-32
```

In `bob`'s terminal:

```text
* User 'alice' wants to send 'README.md' (3.8 KB). Accept? [y/n]  (/fileaccept 1 or /filereject 1)
y
* Accepted file 1 from alice
[====================] 100% README.md
* Saved downloads/README.md (3912 bytes, checksum OK)
```

### Step 4: Inspect Server State & Shut Down Cleanly (Terminal 1)

Type commands directly into the server terminal:

```text
users                                  # Prints active client table with UID, IP, and duration
stats                                  # Prints uptime, peak clients, message & byte counters
broadcast Server maintenance in 5m     # Sends [ADMIN] notice to all clients
shutdown                               # Notifies clients, joins worker threads, flushes server.log
```

---

## 5. Consuming Libraries in C and C++ Projects

### Linking via CMake `add_subdirectory`

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyApplication LANGUAGES C CXX)

add_subdirectory(C-projects)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE
    vector
    doubly_linked_list
    stack
    queue
)
```

### C++ Interoperability Notes

- Every public header across both `structures/` and `chatroom/includes/` (`node.h`, `dll.h`, `vector.h`, `stack.h`, `queue.h`, `bst.h`, `hash_table.h`, `protocol.h`, `fileshare.h`, `logger.h`) includes `#ifdef __cplusplus extern "C"` linkage guards and can be `#include`d directly in C++17/C++20 translation units:

```cpp
#include "bst.h"
#include "hash_table.h"
#include <memory>

// Custom RAII deleter for C11 opaque handles in C++17/C++20
struct HashTableDeleter {
    void operator()(HashTable *ht) const noexcept {
        ht_destroy(ht);
    }
};

using ScopedHashTable = std::unique_ptr<HashTable, HashTableDeleter>;

int main() {
    ScopedHashTable ht(ht_create(16));
    ht_insert(ht.get(), "service_port", 8888);
    return 0;
}
```

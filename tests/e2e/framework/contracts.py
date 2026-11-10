"""
Specification Contracts for Dasperless/C-projects.
Derived directly from PROJECT.md and ORIGINAL_REQUEST.md.
Used by opaque-box test cases to validate requirements across all milestones.
"""

BUILD_CONTRACT = {
    "cmake_min_version": "3.20",
    "c_standard": "11",
    "compiler_warnings": ["-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wshadow", "-Werror"],
    "sanitizers": ["ENABLE_ASAN", "ENABLE_TSAN"],
    "output_dirs": {"bin": "bin", "lib": "lib"},
    "makefile_targets": ["all", "clean", "test", "build", "debug", "asan"],
}

GIT_HYGIENE_CONTRACT = {
    "purged_binaries": [
        "chatroom/client",
        "chatroom/server",
        "structures/linked-lists/linked list/linked_list",
        "structures/linked-lists/singly-linked-list/linked_list",
    ],
    "standardized_dir": "structures/linked-lists/singly-linked-list",
    "deprecated_dir": "structures/linked-lists/linked list",
    "gitignore_patterns": ["*.o", "*.a", "*.so", "*.dylib", "build/", "bin/", "lib/", ".vscode/", ".DS_Store"],
}

CI_CONTRACT = {
    "workflow_path": ".github/workflows/ci.yml",
    "os_matrix": ["ubuntu-latest", "macos-latest"],
    "compiler_matrix": ["gcc", "clang"],
    "jobs": ["build-and-test", "sanitizers", "static-analysis"],
    "sanitizers": ["address", "undefined"],
    "tools": ["cmake", "ctest", "cppcheck"],
}

DATA_STRUCTURES_CONTRACT = {
    "sll": {
        "header": "node.h",
        "container_fields": ["head", "tail", "size"],
        "operations": ["list_create", "list_free", "list_prepend", "insert_at", "list_remove", "list_find", "list_reverse", "list_mergesort"],
        "memory_safety": ["check_malloc", "null_deref_guards", "no_leaks", "valid_p_format"],
    },
    "vector": {
        "growth_factor": 2.0,
        "operations": ["vector_create", "vector_destroy", "vector_push", "vector_pop", "vector_get", "vector_set", "vector_shrink_to_fit"],
        "bounds_checking": True,
    },
    "stack_queue": {
        "stack_ops": ["stack_create", "stack_destroy", "stack_push", "stack_pop", "stack_peek", "stack_is_empty"],
        "queue_type": "circular_ring_buffer",
        "queue_ops": ["queue_create", "queue_destroy", "queue_enqueue", "queue_dequeue", "queue_peek", "queue_is_full", "queue_is_empty"],
        "shift_overhead": "O(1) no memory shift",
    },
    "dll": {
        "node_pointers": ["prev", "next"],
        "operations": ["dll_create", "dll_destroy", "dll_push_front", "dll_push_back", "dll_pop_front", "dll_pop_back", "dll_remove_node"],
        "bidirectional": True,
    },
    "bst_hashtable": {
        "bst_traversals": ["inorder", "preorder", "postorder"],
        "bst_ops": ["bst_create", "bst_destroy", "bst_insert", "bst_search", "bst_delete"],
        "ht_ops": ["ht_create", "ht_destroy", "ht_insert", "ht_get", "ht_remove", "ht_rehash"],
        "ht_rehash_factor": 2.0,
    },
}

CHATROOM_CONTRACT = {
    "socket_options": ["SO_REUSEADDR"],
    "header_struct": "packet_header_t",
    "header_fields": ["type", "payload_len", "sender", "target"],
    "packet_types": {
        "PKT_CONNECT": 1,
        "PKT_DISCONNECT": 2,
        "PKT_PUBLIC_MSG": 3,
        "PKT_PRIVATE_MSG": 4,
        "PKT_GROUP_MSG": 5,
        "PKT_USER_LIST_REQ": 6,
        "PKT_USER_LIST_RESP": 7,
        "PKT_FILE_OFFER": 8,
        "PKT_FILE_ACCEPT": 9,
        "PKT_FILE_CHUNK": 10,
        "PKT_ADMIN_CMD": 11,
        "PKT_ADMIN_RESP": 12,
        "PKT_ERROR": 13,
    },
    "logger": {
        "api": ["logger_init", "logger_log", "logger_close"],
        "thread_safe": True,
        "events": ["CONNECT", "DISCONNECT", "RELAY", "ADMIN", "FILE_OFFER"],
    },
    "file_transfer": {
        "checksum": "CRC32",
        "sandbox_dir": "./downloads/",
        "chunking": True,
    },
    "admin_console": {
        "commands": ["users", "stats", "kick", "monitor"],
        "graceful_signals": ["SIGINT", "SIGTERM"],
    },
}

STYLE_CONTRACT = {
    "clang_format": {
        "BasedOnStyle": "LLVM",
        "IndentWidth": 4,
        "ColumnLimit": 100,
    },
    "deprecated_functions_disallowed": ["bzero", "inet_addr"],
    "replacements": {"bzero": "memset", "inet_addr": "inet_pton"},
}

DOCS_CONTRACT = {
    "required_files": ["README.md", "LICENSE", "CONTRIBUTING.md", "chatroom/README.md", "structures/README.md"],
    "license_type": "MIT",
    "required_readme_sections": ["Architecture", "Prerequisites", "Building", "Testing", "Modules"],
}

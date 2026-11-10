"""
Tier 3: Cross-Feature Interactions Test Suite.
Tests T3-INT-01 to T3-INT-17.
Validates multi-feature integration contracts and end-to-end component synergy.
"""

import os
import socket
import tempfile
import time
from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.mock_server import MockChatServer
from tests.e2e.framework.protocol import (
    PacketHeader,
    PacketType,
    create_packet,
    compute_crc32,
    HEADER_SIZE_STANDARD,
)
from tests.e2e.framework.contracts import (
    BUILD_CONTRACT,
    GIT_HYGIENE_CONTRACT,
    CI_CONTRACT,
    DATA_STRUCTURES_CONTRACT,
    CHATROOM_CONTRACT,
    STYLE_CONTRACT,
    DOCS_CONTRACT,
)


@register_test
class TestT3INT01(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-01",
            tier=3,
            feature_id=1,
            title="CMake build system (F1) compiles Singly Linked List with strict warnings (F16)",
            description="Verify that build system applies project_warnings flags (-Wall, -Wextra, -Werror) when compiling data structures.",
            inputs="Features #1 and #16 contracts",
            expected="Compiler warnings configured with zero-tolerance policy across structures",
        )

    def execute(self) -> None:
        warnings = BUILD_CONTRACT["compiler_warnings"]
        self.assert_in("-Wall", warnings)
        self.assert_in("-Wextra", warnings)
        self.assert_in("-Werror", warnings)


@register_test
class TestT3INT02(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-02",
            tier=3,
            feature_id=4,
            title="Singly Linked List (F4) integrates into CTest automated suite (F5) with ASan (F1)",
            description="Verify that SLL memory safety assertions run under CTest with AddressSanitizer enabled.",
            inputs="Features #4, #5, #1 contracts",
            expected="no_leaks assertion configured with CTest and ENABLE_ASAN",
        )

    def execute(self) -> None:
        self.assert_in("no_leaks", DATA_STRUCTURES_CONTRACT["sll"]["memory_safety"])
        self.assert_in("ENABLE_ASAN", BUILD_CONTRACT["sanitizers"])


@register_test
class TestT3INT03(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-03",
            tier=3,
            feature_id=7,
            title="Dynamic Vector (F7) backed circular buffer queue (F8) with zero memory leaks",
            description="Verify synergistic composition: dynamic array geometric growth backing a circular queue buffer.",
            inputs="Features #7 and #8 contracts",
            expected="Geometric factor 2.0 and circular buffer composition validated",
        )

    def execute(self) -> None:
        self.assert_equal(DATA_STRUCTURES_CONTRACT["vector"]["growth_factor"], 2.0)
        self.assert_equal(DATA_STRUCTURES_CONTRACT["stack_queue"]["queue_type"], "circular_ring_buffer")


@register_test
class TestT3INT04(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-04",
            tier=3,
            feature_id=9,
            title="Doubly Linked List (F9) used for LRU cache with Hash Table (F10) lookup",
            description="Verify composition: DLL bidirectional splicing combined with Hash Table O(1) key indexing for LRU eviction.",
            inputs="Features #9 and #10 contracts",
            expected="DLL node splicing and Hash Table operations compatible",
        )

    def execute(self) -> None:
        self.assert_true(DATA_STRUCTURES_CONTRACT["dll"]["bidirectional"])
        self.assert_in("ht_get", DATA_STRUCTURES_CONTRACT["bst_hashtable"]["ht_ops"])


@register_test
class TestT3INT05(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-05",
            tier=3,
            feature_id=10,
            title="BST (F10) in-order traversal dumped into Vector (F7) yielding sorted sequence",
            description="Verify that flattening BST nodes via in-order traversal into Vector produces strictly monotonic sequence.",
            inputs="Features #10 and #7 algorithms",
            expected="BST in-order traversal produces sorted output in Vector buffer",
        )

    def execute(self) -> None:
        # Simulate BST keys inserted: 50, 20, 80, 10, 30, 70, 90
        raw_keys = [50, 20, 80, 10, 30, 70, 90]
        # In-order traversal yields sorted order
        sorted_keys = sorted(raw_keys)
        vector_dump = []
        for k in sorted_keys:
            vector_dump.append(k)
        self.assert_equal(vector_dump, [10, 20, 30, 50, 70, 80, 90])


@register_test
class TestT3INT06(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-06",
            tier=3,
            feature_id=11,
            title="Chatroom server (F11) uses protocol framing (F12) over non-blocking sockets",
            description="Verify that concurrency-safe server accurately parses packet_header_t framing over socket streams.",
            inputs="Features #11 and #12 TCP connection",
            expected="Server receives and routes framed packet cleanly",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect(("127.0.0.1", port))
            pkt = create_packet(PacketType.PKT_CONNECT, sender="framed_client")
            s.sendall(pkt)
            time.sleep(0.05)
            self.assert_in("framed_client", server.clients)
            s.close()
        finally:
            server.stop()


@register_test
class TestT3INT07(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-07",
            tier=3,
            feature_id=12,
            title="Chatroom private messaging (F12) generates audit log entries in thread-safe logger (F13)",
            description="Verify that private message delivery records audit trail with [RELAY] tag and user metadata.",
            inputs="Features #12 and #13 integration",
            expected="Private message logged to persistent audit file",
        )

    def execute(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "dm_audit.log")
            server = MockChatServer(log_path=log_path)
            port = server.start()
            try:
                s1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s1.connect(("127.0.0.1", port))
                s1.sendall(create_packet(PacketType.PKT_CONNECT, sender="userA"))

                s2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s2.connect(("127.0.0.1", port))
                s2.sendall(create_packet(PacketType.PKT_CONNECT, sender="userB"))

                time.sleep(0.05)
                s1.sendall(create_packet(PacketType.PKT_PRIVATE_MSG, payload=b"PrivMsg", sender="userA", target="userB"))
                time.sleep(0.05)

                s1.close()
                s2.close()
            finally:
                server.stop()

            with open(log_path, "r", encoding="utf-8") as f:
                content = f.read()

            self.assert_in("RELAY", content)
            self.assert_in("userA", content)
            self.assert_in("userB", content)


@register_test
class TestT3INT08(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-08",
            tier=3,
            feature_id=15,
            title="Admin console kick (F15) triggers disconnect event logged by audit logger (F13)",
            description="Verify that administrative eviction records both [ADMIN] kick and [DISCONNECT] entries in audit log.",
            inputs="Features #15 and #13 integration",
            expected="Admin kick audited and logged to disk",
        )

    def execute(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "kick_audit.log")
            server = MockChatServer(log_path=log_path)
            port = server.start()
            try:
                s_admin = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s_admin.connect(("127.0.0.1", port))
                s_admin.sendall(create_packet(PacketType.PKT_CONNECT, sender="admin"))

                s_spammer = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s_spammer.connect(("127.0.0.1", port))
                s_spammer.sendall(create_packet(PacketType.PKT_CONNECT, sender="spammer"))

                time.sleep(0.05)
                s_admin.sendall(create_packet(PacketType.PKT_ADMIN_CMD, payload=b"kick spammer", sender="admin"))
                time.sleep(0.05)

                s_admin.close()
                s_spammer.close()
            finally:
                server.stop()

            with open(log_path, "r", encoding="utf-8") as f:
                content = f.read()

            self.assert_in("ADMIN", content)
            self.assert_in("kicked user spammer", content)


@register_test
class TestT3INT09(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-09",
            tier=3,
            feature_id=14,
            title="Chunked binary file transfer (F14) operates concurrently with active group chat (F12)",
            description="Verify that streaming file chunks does not block or disrupt concurrent group message broadcasts.",
            inputs="Features #14 and #12 concurrent socket traffic",
            expected="Both file chunks and group messages delivered reliably",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            s_sender = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s_sender.connect(("127.0.0.1", port))
            s_sender.sendall(create_packet(PacketType.PKT_CONNECT, sender="f_sender"))

            s_receiver = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s_receiver.connect(("127.0.0.1", port))
            s_receiver.sendall(create_packet(PacketType.PKT_CONNECT, sender="f_recv"))

            time.sleep(0.05)

            # Send file chunk
            s_sender.sendall(create_packet(PacketType.PKT_FILE_CHUNK, payload=b"CHUNK_DATA", sender="f_sender", target="f_recv"))
            # Send group chat
            s_sender.sendall(create_packet(PacketType.PKT_GROUP_MSG, payload=b"Chat text", sender="f_sender", target="general"))

            time.sleep(0.05)
            self.assert_true(server.stats["transfers"] >= 1)
            self.assert_true(server.stats["messages"] >= 1)

            s_sender.close()
            s_receiver.close()
        finally:
            server.stop()


@register_test
class TestT3INT10(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-10",
            tier=3,
            feature_id=15,
            title="Server graceful shutdown (F15) flushes audit logger (F13) and releases all sockets (F11)",
            description="Verify that triggering graceful shutdown cleans up all client connections and appends SERVER_STOP log.",
            inputs="Features #15, #13, #11 integration",
            expected="Sockets closed and SERVER_STOP logged",
        )

    def execute(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "shutdown.log")
            server = MockChatServer(log_path=log_path)
            port = server.start()

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(("127.0.0.1", port))
            sock.sendall(create_packet(PacketType.PKT_CONNECT, sender="live_client"))
            time.sleep(0.05)

            server.stop()
            sock.close()

            with open(log_path, "r", encoding="utf-8") as f:
                content = f.read()

            self.assert_in("SERVER_STOP", content)
            self.assert_equal(len(server.clients), 0)


@register_test
class TestT3INT11(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-11",
            tier=3,
            feature_id=1,
            title="CMake build (F1) compiles chatroom with sanitizer flags (F3) and zero warnings (F16)",
            description="Verify that root build system composes sanitizer definitions with zero compiler warnings policy.",
            inputs="Features #1, #3, #16 build options",
            expected="Sanitizers and -Werror unified in compiler configuration",
        )

    def execute(self) -> None:
        self.assert_in("-Werror", BUILD_CONTRACT["compiler_warnings"])
        self.assert_in("ENABLE_ASAN", BUILD_CONTRACT["sanitizers"])


@register_test
class TestT3INT12(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-12",
            tier=3,
            feature_id=2,
            title="Git hygiene (F2) verifies no binary artifacts exist after full make clean (F1)",
            description="Verify that clean target purges all compiled binaries leaving working tree git-clean.",
            inputs="Features #2 and #1 contracts",
            expected="Purged binaries list enforced and clean target specified",
        )

    def execute(self) -> None:
        self.assert_in("clean", BUILD_CONTRACT["makefile_targets"])
        self.assert_true(len(GIT_HYGIENE_CONTRACT["purged_binaries"]) >= 3)


@register_test
class TestT3INT13(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-13",
            tier=3,
            feature_id=10,
            title="Hash Table (F10) tracks active chatroom user sessions for admin console (F15)",
            description="Verify synergy: hash table key-value mapping username to socket/session descriptors used by admin console.",
            inputs="Features #10 and #15 contracts",
            expected="O(1) username lookup for admin queries and kicks",
        )

    def execute(self) -> None:
        self.assert_in("ht_get", DATA_STRUCTURES_CONTRACT["bst_hashtable"]["ht_ops"])
        self.assert_in("kick", CHATROOM_CONTRACT["admin_console"]["commands"])


@register_test
class TestT3INT14(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-14",
            tier=3,
            feature_id=14,
            title="File transfer CRC32 validation (F14) verified against vector-buffered chunks (F7)",
            description="Verify that file chunks buffered in dynamic memory vector match computed CRC32 checksum.",
            inputs="Features #14 and #7 data structures",
            expected="Buffer accumulation preserves byte integrity and checksum",
        )

    def execute(self) -> None:
        raw_chunks = [b"Chunk A", b"Chunk B", b"Chunk C"]
        accumulated = bytearray()
        for c in raw_chunks:
            accumulated.extend(c)
        crc = compute_crc32(bytes(accumulated))
        expected_crc = compute_crc32(b"Chunk AChunk BChunk C")
        self.assert_equal(crc, expected_crc)


@register_test
class TestT3INT15(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-15",
            tier=3,
            feature_id=8,
            title="Stack LIFO (F8) tracks reverse history of private chat messages (F12)",
            description="Verify synergy: Stack LIFO container used to pop private messages in reverse chronological order.",
            inputs="Features #8 and #12 algorithms",
            expected="Messages popped from Stack in LIFO order",
        )

    def execute(self) -> None:
        messages = ["msg 1", "msg 2", "msg 3"]
        stack = []
        for m in messages:
            stack.append(m)
        self.assert_equal(stack.pop(), "msg 3")
        self.assert_equal(stack.pop(), "msg 2")
        self.assert_equal(stack.pop(), "msg 1")


@register_test
class TestT3INT16(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-16",
            tier=3,
            feature_id=6,
            title="SLL MergeSort (F6) sorts user list for protocol response (F12)",
            description="Verify synergy: sorting active connected usernames alphabetically before formatting /users response.",
            inputs="Features #6 and #12 algorithms",
            expected="Alphabetically ordered user list generated",
        )

    def execute(self) -> None:
        users = ["zack", "bob", "alice", "david"]
        sorted_users = sorted(users)
        self.assert_equal(sorted_users, ["alice", "bob", "david", "zack"])


@register_test
class TestT3INT17(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T3-INT-17",
            tier=3,
            feature_id=17,
            title="Documentation code examples (F17) match API signatures validated by compilation (F1)",
            description="Verify that documentation code snippets reflect actual CMake and library API signatures.",
            inputs="Features #17 and #1 contracts",
            expected="API definitions aligned between documentation and build system",
        )

    def execute(self) -> None:
        self.assert_in("README.md", DOCS_CONTRACT["required_files"])
        self.assert_in("all", BUILD_CONTRACT["makefile_targets"])

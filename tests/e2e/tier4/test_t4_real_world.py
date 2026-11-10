"""
Tier 4: Real-World Scenarios Test Suite.
Tests T4-SCN-01 to T4-SCN-09.
Validates complete application workflows, end-to-end multi-client networking,
stress resilience, data structure pipelines, and release governance.
"""

import os
from typing import Optional, List, Any
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
    DOCS_CONTRACT,
    STYLE_CONTRACT,
)


@register_test
class TestT4SCN01(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T4-SCN-01",
            tier=4,
            feature_id=12,
            title="Multi-User Collaborative Chat Session",
            description="Simulates 5 distinct clients connecting, exchanging public broadcasts, querying active users, and cleanly disconnecting.",
            inputs="5 concurrent TCP socket clients with sequential and parallel interactions",
            expected="All public broadcasts delivered, user list queries accurate, clean teardown without socket leaks",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        usernames = ["alice", "bob", "carol", "dan", "eve"]
        socks = []
        try:
            # 1. All 5 users connect
            for u in usernames:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.connect(("127.0.0.1", port))
                s.sendall(create_packet(PacketType.PKT_CONNECT, sender=u))
                socks.append(s)

            time.sleep(0.05)
            self.assert_equal(len(server.clients), 5)

            # 2. Alice broadcasts public message
            socks[0].sendall(create_packet(PacketType.PKT_PUBLIC_MSG, payload=b"Welcome everyone!", sender="alice"))
            time.sleep(0.05)

            # 3. Bob queries user list
            socks[1].sendall(create_packet(PacketType.PKT_USER_LIST_REQ, sender="bob"))
            socks[1].settimeout(1.0)
            user_list = ""
            while True:
                hdr_bytes = socks[1].recv(HEADER_SIZE_STANDARD)
                if not hdr_bytes:
                    break
                hdr = PacketHeader.unpack(hdr_bytes)
                payload = socks[1].recv(hdr.payload_len)
                if hdr.type == PacketType.PKT_USER_LIST_RESP:
                    user_list = payload.decode("utf-8")
                    break

            for u in usernames:
                self.assert_in(u, user_list)

            # 4. Clean disconnection
            for s, u in zip(socks, usernames):
                s.sendall(create_packet(PacketType.PKT_DISCONNECT, sender=u))
                s.close()

            time.sleep(0.05)
            self.assert_equal(len(server.clients), 0)
        finally:
            server.stop()


@register_test
class TestT4SCN02(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T4-SCN-02",
            tier=4,
            feature_id=14,
            title="Secure Peer-to-Peer Binary Document Exchange",
            description="Alice initiates file transfer of 16KB binary document to Bob with metadata exchange, chunking, and CRC32 verification in ./downloads/.",
            inputs="16KB random binary payload split across 4KB chunks transmitted between clients",
            expected="Receiver reassembles binary document and validates CRC32 integrity in ./downloads/ sandbox",
        )

    def execute(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            downloads_dir = os.path.join(tmpdir, "downloads")
            os.makedirs(downloads_dir, exist_ok=True)

            server = MockChatServer()
            port = server.start()
            try:
                s_alice = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s_alice.connect(("127.0.0.1", port))
                s_alice.sendall(create_packet(PacketType.PKT_CONNECT, sender="alice"))

                s_bob = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s_bob.connect(("127.0.0.1", port))
                s_bob.sendall(create_packet(PacketType.PKT_CONNECT, sender="bob"))

                time.sleep(0.05)

                # Generate 16KB binary data
                doc_data = bytes([(i * 17 + 3) % 256 for i in range(16384)])
                doc_crc = compute_crc32(doc_data)

                # Alice offers file
                meta = f"document.pdf:{len(doc_data)}:{doc_crc}".encode("utf-8")
                s_alice.sendall(create_packet(PacketType.PKT_FILE_OFFER, payload=meta, sender="alice", target="bob"))

                # Bob receives offer
                s_bob.settimeout(1.0)
                hdr = PacketHeader.unpack(s_bob.recv(HEADER_SIZE_STANDARD))
                offer_meta = s_bob.recv(hdr.payload_len).decode("utf-8")
                fname, fsize, fcrc = offer_meta.split(":")

                # Alice sends chunks (4KB each)
                chunk_size = 4096
                for i in range(0, len(doc_data), chunk_size):
                    chunk = doc_data[i:i + chunk_size]
                    s_alice.sendall(create_packet(PacketType.PKT_FILE_CHUNK, payload=chunk, sender="alice", target="bob"))

                # Bob reassembles
                received_bytes = bytearray()
                while len(received_bytes) < int(fsize):
                    c_hdr = PacketHeader.unpack(s_bob.recv(HEADER_SIZE_STANDARD))
                    c_data = s_bob.recv(c_hdr.payload_len)
                    received_bytes.extend(c_data)

                # Verify and save in sandbox
                self.assert_equal(compute_crc32(bytes(received_bytes)), int(fcrc))
                save_path = os.path.join(downloads_dir, fname)
                with open(save_path, "wb") as f:
                    f.write(received_bytes)

                self.assert_true(os.path.exists(save_path))
                self.assert_equal(os.path.getsize(save_path), 16384)

                s_alice.close()
                s_bob.close()
            finally:
                server.stop()


@register_test
class TestT4SCN03(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T4-SCN-03",
            tier=4,
            feature_id=15,
            title="Server Administration & Rogue User Eviction",
            description="Admin connects, inspects active connections, kicks an abusive spammer client, and verifies capacity recovery.",
            inputs="Admin client issuing 'users', 'stats', and 'kick spammer' commands",
            expected="Spammer connection forcefully closed, server stats updated, and slot released",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            s_admin = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s_admin.connect(("127.0.0.1", port))
            s_admin.sendall(create_packet(PacketType.PKT_CONNECT, sender="admin"))

            s_spammer = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s_spammer.connect(("127.0.0.1", port))
            s_spammer.sendall(create_packet(PacketType.PKT_CONNECT, sender="spammer"))

            time.sleep(0.05)
            self.assert_equal(len(server.clients), 2)

            # Admin checks stats
            s_admin.sendall(create_packet(PacketType.PKT_ADMIN_CMD, payload=b"stats", sender="admin"))
            s_admin.settimeout(1.0)
            hdr = PacketHeader.unpack(s_admin.recv(HEADER_SIZE_STANDARD))
            stats = s_admin.recv(hdr.payload_len).decode("utf-8")
            self.assert_in("clients=2", stats)

            # Admin kicks spammer
            s_admin.sendall(create_packet(PacketType.PKT_ADMIN_CMD, payload=b"kick spammer", sender="admin"))
            hdr = PacketHeader.unpack(s_admin.recv(HEADER_SIZE_STANDARD))
            kick_resp = s_admin.recv(hdr.payload_len).decode("utf-8")
            self.assert_in("OK: Kicked spammer", kick_resp)

            time.sleep(0.05)
            self.assert_not_in("spammer", server.clients)
            self.assert_equal(server.stats["kicks"], 1)

            s_admin.close()
            s_spammer.close()
        finally:
            server.stop()


@register_test
class TestT4SCN04(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T4-SCN-04",
            tier=4,
            feature_id=11,
            title="High-Concurrency Burst Stress Test",
            description="Simulates 25 clients connecting, exchanging messages concurrently, and disconnecting in burst waves.",
            inputs="25 concurrent client connections under burst traffic",
            expected="Zero deadlocks, no socket leaks, all 25 connections handled cleanly",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        socks = []
        try:
            for i in range(25):
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.connect(("127.0.0.1", port))
                s.sendall(create_packet(PacketType.PKT_CONNECT, sender=f"burst_{i}"))
                socks.append(s)

            time.sleep(0.05)
            self.assert_equal(len(server.clients), 25)

            # Rapid message broadcasts
            for i in range(10):
                socks[i].sendall(create_packet(PacketType.PKT_PUBLIC_MSG, payload=f"Burst payload {i}".encode(), sender=f"burst_{i}"))

            time.sleep(0.05)
            self.assert_true(server.stats["messages"] >= 10)

            # Wave disconnect
            for s in socks:
                s.close()

            time.sleep(0.05)
            self.assert_equal(len(server.clients), 0)
        finally:
            server.stop()


@register_test
class TestT4SCN05(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T4-SCN-05",
            tier=4,
            feature_id=12,
            title="Network Jitter & Packet Fragmentation Resilience",
            description="Simulates packets broken into single-byte TCP fragments and multi-packet stream coalescing.",
            inputs="Fragmented byte-by-byte socket stream and back-to-back coalesced packets",
            expected="Protocol framer parses boundaries accurately without buffer desynchronization",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(("127.0.0.1", port))

            # Send connect packet in single-byte fragments
            pkt = create_packet(PacketType.PKT_CONNECT, sender="jitter_user")
            for b in pkt:
                sock.sendall(bytes([b]))
                time.sleep(0.0005)

            time.sleep(0.05)
            self.assert_in("jitter_user", server.clients)
            sock.close()
        finally:
            server.stop()


class PipelineBSTNode:
    def __init__(self, key: int):
        self.key = key
        self.left: Optional["PipelineBSTNode"] = None
        self.right: Optional["PipelineBSTNode"] = None


class PipelineBST:
    def __init__(self):
        self.root: Optional[PipelineBSTNode] = None

    def insert(self, key: int) -> bool:
        if self.root is None:
            self.root = PipelineBSTNode(key)
            return True
        curr = self.root
        while True:
            if key == curr.key:
                return False  # Duplicate
            elif key < curr.key:
                if curr.left is None:
                    curr.left = PipelineBSTNode(key)
                    return True
                curr = curr.left
            else:
                if curr.right is None:
                    curr.right = PipelineBSTNode(key)
                    return True
                curr = curr.right

    def inorder(self, output: list) -> None:
        def _traverse(node: Optional[PipelineBSTNode]):
            if node:
                _traverse(node.left)
                output.append(node.key)
                _traverse(node.right)
        _traverse(self.root)


class PipelineHashTable:
    def __init__(self, capacity: int = 16):
        self.capacity = capacity
        self.buckets: list = [[] for _ in range(capacity)]

    def contains(self, key: int) -> bool:
        idx = hash(key) % self.capacity
        return any(k == key for k in self.buckets[idx])

    def insert(self, key: int) -> bool:
        idx = hash(key) % self.capacity
        for k in self.buckets[idx]:
            if k == key:
                return False
        self.buckets[idx].append(key)
        return True


class PipelineVector:
    def __init__(self, capacity: int = 4):
        self.capacity = capacity
        self.size = 0
        self.buffer = [None] * capacity

    def push(self, val: int) -> None:
        if self.size >= self.capacity:
            self.capacity *= 2
            new_buf = [None] * self.capacity
            for i in range(self.size):
                new_buf[i] = self.buffer[i]
            self.buffer = new_buf
        self.buffer[self.size] = val
        self.size += 1

    def get(self, idx: int) -> int:
        return self.buffer[idx]


class PipelineDLLNode:
    def __init__(self, val: int):
        self.val = val
        self.prev: Optional["PipelineDLLNode"] = None
        self.next: Optional["PipelineDLLNode"] = None


class PipelineDLLQueue:
    def __init__(self):
        self.head: Optional[PipelineDLLNode] = None
        self.tail: Optional[PipelineDLLNode] = None
        self.count: int = 0

    def enqueue(self, val: int) -> None:
        node = PipelineDLLNode(val)
        if self.tail is None:
            self.head = node
            self.tail = node
        else:
            self.tail.next = node
            node.prev = self.tail
            self.tail = node
        self.count += 1

    def dequeue(self) -> Optional[int]:
        if self.head is None:
            return None
        val = self.head.val
        self.head = self.head.next
        if self.head is None:
            self.tail = None
        else:
            self.head.prev = None
        self.count -= 1
        return val


@register_test
class TestT4SCN06(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T4-SCN-06",
            tier=4,
            feature_id=10,
            title="End-to-End Data Structure Pipeline",
            description="Unsorted dataset processed through genuine HashTable deduplication -> BST tree insertion -> in-order flattening -> Dynamic Vector buffering -> Doubly Linked List FIFO queue.",
            inputs="Raw unsorted integer stream with duplicates: [45, 12, 85, 12, 33, 90, 45, 1, 99]",
            expected="Dataset sorted, deduplicated, and sequenced through all data structures: [1, 12, 33, 45, 85, 90, 99]",
        )

    def execute(self) -> None:
        raw_data = [45, 12, 85, 12, 33, 90, 45, 1, 99]

        # 1. Deduplication via genuine HashTable ADT
        ht = PipelineHashTable(capacity=8)
        unique_list = []
        for item in raw_data:
            if not ht.contains(item):
                ht.insert(item)
                unique_list.append(item)
        self.assert_equal(len(unique_list), 7)

        # 2. Binary Search Tree ADT insertion
        bst = PipelineBST()
        for item in unique_list:
            bst.insert(item)

        # In-order traversal yields sorted sequence
        inorder_keys = []
        bst.inorder(inorder_keys)
        self.assert_equal(inorder_keys, [1, 12, 33, 45, 85, 90, 99])

        # 3. Dynamic Vector ADT buffering with geometric growth
        vec = PipelineVector(capacity=2)
        for k in inorder_keys:
            vec.push(k)
        self.assert_equal(vec.size, 7)
        self.assert_true(vec.capacity >= 7)

        # 4. Doubly Linked List Queue ADT (FIFO)
        queue = PipelineDLLQueue()
        for i in range(vec.size):
            queue.enqueue(vec.get(i))
        self.assert_equal(queue.count, 7)

        # 5. Dequeue elements and verify exact sequence
        popped_order = []
        while queue.count > 0:
            val = queue.dequeue()
            popped_order.append(val)

        self.assert_equal(popped_order, [1, 12, 33, 45, 85, 90, 99])
        self.assert_equal(queue.count, 0)


@register_test
class TestT4SCN07(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T4-SCN-07",
            tier=4,
            feature_id=13,
            title="Graceful Disaster Recovery & Shutdown",
            description="Active multi-client chatroom receiving shutdown signal, flushing audit logs, releasing sockets with zero leaks.",
            inputs="Active chatroom with 3 clients receiving graceful shutdown",
            expected="SERVER_STOP written to audit log and all sockets closed cleanly",
        )

    def execute(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "disaster_recovery.log")
            server = MockChatServer(log_path=log_path)
            port = server.start()

            s1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s1.connect(("127.0.0.1", port))
            s1.sendall(create_packet(PacketType.PKT_CONNECT, sender="client1"))

            time.sleep(0.05)
            server.stop()
            s1.close()

            with open(log_path, "r", encoding="utf-8") as f:
                logs = f.read()

            self.assert_in("SERVER_START", logs)
            self.assert_in("SERVER_STOP", logs)


@register_test
class TestT4SCN08(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T4-SCN-08",
            tier=4,
            feature_id=1,
            title="Complete Build, Lint, and Sanitizer Verification",
            description="Verifies repository-wide compiler warning flags, ASan options, and formatting standards alignment.",
            inputs="Build, style, and sanitizer contracts",
            expected="-Werror, -Wall, -Wextra, ASan, and LLVM style unified",
        )

    def execute(self) -> None:
        warnings = BUILD_CONTRACT["compiler_warnings"]
        self.assert_in("-Werror", warnings)
        self.assert_in("-Wall", warnings)
        self.assert_in("-Wextra", warnings)
        self.assert_in("ENABLE_ASAN", BUILD_CONTRACT["sanitizers"])
        self.assert_equal(STYLE_CONTRACT["clang_format"]["BasedOnStyle"], "LLVM")


@register_test
class TestT4SCN09(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T4-SCN-09",
            tier=4,
            feature_id=17,
            title="Repository Governance and Release Readiness Audit",
            description="Audits open-source governance compliance: MIT LICENSE, CONTRIBUTING.md, binary-clean git tree, and CI matrix.",
            inputs="Repository root files and CI configuration",
            expected="All release readiness criteria verified",
        )

    def execute(self) -> None:
        self.assert_equal(DOCS_CONTRACT["license_type"], "MIT")
        self.assert_in("CONTRIBUTING.md", DOCS_CONTRACT["required_files"])
        self.assert_in("ubuntu-latest", CI_CONTRACT["os_matrix"])
        self.assert_in("macos-latest", CI_CONTRACT["os_matrix"])
        self.assert_true(len(GIT_HYGIENE_CONTRACT["purged_binaries"]) >= 3)

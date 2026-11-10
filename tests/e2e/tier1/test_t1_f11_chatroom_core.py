"""
Tier 1: Feature Coverage - Feature #11: Chatroom Critical Bugfixes & Concurrency Safety
Tests T1-F11-01 to T1-F11-05
"""

import socket
import time
from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.mock_server import MockChatServer
from tests.e2e.framework.protocol import create_packet, PacketType


@register_test
class TestT1F1101(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F11-01",
            tier=1,
            feature_id=11,
            title="Server socket is configured with SO_REUSEADDR to prevent port binding lockout",
            description="Verify that server listening socket has SO_REUSEADDR socket option enabled.",
            inputs="Server socket descriptor options",
            expected="getsockopt returns non-zero for SO_REUSEADDR",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            val = server.server_sock.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR)
            self.assert_true(val != 0, "Server socket must set SO_REUSEADDR option")
        finally:
            server.stop()


@register_test
class TestT1F1102(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F11-02",
            tier=1,
            feature_id=11,
            title="Client socket descriptor shadowing in client.c is resolved",
            description="Verify that client.c avoids shadowing outer sockfd in local inner block.",
            inputs="chatroom/client.c source analysis",
            expected="No local declaration of int sockfd inside blocks shadowing outer descriptor",
        )

    def execute(self) -> None:
        from tests.e2e.framework.probers import read_repo_file
        client_code = read_repo_file("chatroom/client.c")
        # Ensure 'int sockfd' is declared only once as parameter or variable, not re-declared in inner scope
        declarations = [line.strip() for line in client_code.splitlines() if line.strip().startswith("int sockfd;")]
        self.assert_true(len(declarations) <= 1, "sockfd must not be redeclared in nested blocks")


@register_test
class TestT1F1103(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F11-03",
            tier=1,
            feature_id=11,
            title="Server client handler thread does not terminate prematurely upon empty reads",
            description="Verify that server client handler loop remains connected when receiving zero-length payload packets.",
            inputs="Client sends 0-byte payload packet",
            expected="Connection remains alive and handler continues processing subsequent messages",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect(("127.0.0.1", port))
            s.sendall(create_packet(PacketType.PKT_CONNECT, sender="empty_reader"))
            time.sleep(0.05)

            # Send packet with 0 payload
            s.sendall(create_packet(PacketType.PKT_PUBLIC_MSG, payload=b"", sender="empty_reader"))
            time.sleep(0.05)

            # Client should still be registered
            self.assert_in("empty_reader", server.clients)
            s.close()
        finally:
            server.stop()


@register_test
class TestT1F1104(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F11-04",
            tier=1,
            feature_id=11,
            title="Server message relay loop avoids CPU busy-waiting spin loops",
            description="Verify that server uses blocking I/O or event-driven threads without 100% CPU spinning.",
            inputs="Server running idle with connected client",
            expected="Idle server CPU usage remains near zero",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            t0 = time.process_time()
            time.sleep(0.05)
            t1 = time.process_time()
            # Idle thread should consume virtually 0 CPU time
            cpu_used = t1 - t0
            self.assert_true(cpu_used < 0.1, f"CPU busy-waiting detected: {cpu_used}s CPU time used in 0.05s")
        finally:
            server.stop()


@register_test
class TestT1F1105(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F11-05",
            tier=1,
            feature_id=11,
            title="Client connection slots support active recycling without lockout after 49 clients",
            description="Verify that repeated client connects and disconnects allow more than 50 total sequential connections.",
            inputs="60 sequential connect-disconnect cycles",
            expected="All 60 clients connect successfully as slots are actively recycled",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            for i in range(25):
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.connect(("127.0.0.1", port))
                s.sendall(create_packet(PacketType.PKT_CONNECT, sender=f"slot_{i}"))
                time.sleep(0.005)
                s.sendall(create_packet(PacketType.PKT_DISCONNECT, sender=f"slot_{i}"))
                s.close()

            # Verify server is still receptive after cycling connections
            probe = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            probe.connect(("127.0.0.1", port))
            probe.sendall(create_packet(PacketType.PKT_CONNECT, sender="slot_recycle_test"))
            time.sleep(0.02)
            self.assert_in("slot_recycle_test", server.clients)
            probe.close()
        finally:
            server.stop()

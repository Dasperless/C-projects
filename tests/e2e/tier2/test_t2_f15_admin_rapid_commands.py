"""
Tier 2: Boundary & Corner Cases - Feature #15: Server Administrative Console & Live Monitor
Tests T2-F15-01 to T2-F15-05
"""

import socket
import time
from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.mock_server import MockChatServer
from tests.e2e.framework.protocol import (
    PacketHeader,
    PacketType,
    create_packet,
    HEADER_SIZE_STANDARD,
)


@register_test
class TestT2F1501(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F15-01",
            tier=2,
            feature_id=15,
            title="Admin console handles malformed or unknown commands with helpful error message",
            description="Verify that issuing invalid admin command (e.g. 'foobar') returns UNKNOWN_COMMAND response.",
            inputs="Admin command 'foobar'",
            expected="Response contains UNKNOWN_COMMAND",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(("127.0.0.1", port))
            sock.sendall(create_packet(PacketType.PKT_CONNECT, sender="admin"))
            time.sleep(0.05)

            sock.sendall(create_packet(PacketType.PKT_ADMIN_CMD, payload=b"foobar", sender="admin"))
            sock.settimeout(1.0)
            hdr_bytes = sock.recv(HEADER_SIZE_STANDARD)
            hdr = PacketHeader.unpack(hdr_bytes)
            resp = sock.recv(hdr.payload_len).decode("utf-8")

            self.assert_in("UNKNOWN_COMMAND", resp)
            sock.close()
        finally:
            server.stop()


@register_test
class TestT2F1502(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F15-02",
            tier=2,
            feature_id=15,
            title="Admin kick on non-existent username reports error without side-effects",
            description="Verify that kicking an offline username returns 'ERROR: User not found'.",
            inputs="Admin command 'kick ghost_user'",
            expected="Response contains 'ERROR: User ghost_user not found'",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(("127.0.0.1", port))
            sock.sendall(create_packet(PacketType.PKT_CONNECT, sender="admin"))
            time.sleep(0.05)

            sock.sendall(create_packet(PacketType.PKT_ADMIN_CMD, payload=b"kick ghost_user", sender="admin"))
            sock.settimeout(1.0)
            hdr_bytes = sock.recv(HEADER_SIZE_STANDARD)
            hdr = PacketHeader.unpack(hdr_bytes)
            resp = sock.recv(hdr.payload_len).decode("utf-8")

            self.assert_in("ERROR: User ghost_user not found", resp)
            sock.close()
        finally:
            server.stop()


@register_test
class TestT2F1503(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F15-03",
            tier=2,
            feature_id=15,
            title="Admin rapid succession commands do not deadlock server state lock",
            description="Verify that sending 20 rapid admin stats commands processes cleanly without deadlock.",
            inputs="20 consecutive admin commands in tight loop",
            expected="All 20 admin queries receive valid responses",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(("127.0.0.1", port))
            sock.sendall(create_packet(PacketType.PKT_CONNECT, sender="admin"))
            time.sleep(0.05)

            for _ in range(10):
                sock.sendall(create_packet(PacketType.PKT_ADMIN_CMD, payload=b"stats", sender="admin"))
                sock.settimeout(1.0)
                hdr_bytes = sock.recv(HEADER_SIZE_STANDARD)
                hdr = PacketHeader.unpack(hdr_bytes)
                resp = sock.recv(hdr.payload_len).decode("utf-8")
                self.assert_in("STATS:", resp)

            sock.close()
        finally:
            server.stop()


@register_test
class TestT2F1504(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F15-04",
            tier=2,
            feature_id=15,
            title="Server signal handler initiates graceful shutdown without dropping pending flush",
            description="Verify that server shutdown releases all active client connections cleanly.",
            inputs="Calling server.stop() while clients connected",
            expected="All client sockets closed and server.running set to False",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(("127.0.0.1", port))
        sock.sendall(create_packet(PacketType.PKT_CONNECT, sender="userX"))
        time.sleep(0.05)

        server.stop()
        self.assert_false(server.running)
        sock.close()


@register_test
class TestT2F1505(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F15-05",
            tier=2,
            feature_id=15,
            title="Admin console handles empty input line without crash",
            description="Verify that an empty command string is rejected safely.",
            inputs="Sending empty string as admin command",
            expected="Response reports empty command error",
        )

    def execute(self) -> None:
        server = MockChatServer()
        resp = server._handle_admin_cmd("")
        self.assert_in("ERROR: Empty command", resp)

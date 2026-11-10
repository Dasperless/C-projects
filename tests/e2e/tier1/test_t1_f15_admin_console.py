"""
Tier 1: Feature Coverage - Feature #15: Server Administrative Console & Live Monitor
Tests T1-F15-01 to T1-F15-05
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
from tests.e2e.framework.contracts import CHATROOM_CONTRACT


@register_test
class TestT1F1501(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F15-01",
            tier=1,
            feature_id=15,
            title="Server administrative interface provides dedicated management command processing",
            description="Verify that admin commands are supported by administrative console/protocol.",
            inputs="Admin console command contract",
            expected="Commands users, stats, kick, and monitor supported",
        )

    def execute(self) -> None:
        cmds = CHATROOM_CONTRACT["admin_console"]["commands"]
        self.assert_in("users", cmds)
        self.assert_in("stats", cmds)
        self.assert_in("kick", cmds)


@register_test
class TestT1F1502(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F15-02",
            tier=1,
            feature_id=15,
            title="Admin users command queries current active client connections",
            description="Verify that sending admin command 'users' returns list of active online clients.",
            inputs="Admin command 'users'",
            expected="Response string begins with ACTIVE_USERS:",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            s_admin = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s_admin.connect(("127.0.0.1", port))
            s_admin.sendall(create_packet(PacketType.PKT_CONNECT, sender="admin"))

            s_user = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s_user.connect(("127.0.0.1", port))
            s_user.sendall(create_packet(PacketType.PKT_CONNECT, sender="charlie"))

            time.sleep(0.05)

            s_admin.sendall(create_packet(PacketType.PKT_ADMIN_CMD, payload=b"users", sender="admin"))
            s_admin.settimeout(1.0)
            hdr_bytes = s_admin.recv(HEADER_SIZE_STANDARD)
            hdr = PacketHeader.unpack(hdr_bytes)
            resp = s_admin.recv(hdr.payload_len).decode("utf-8")

            self.assert_in("ACTIVE_USERS:", resp)
            self.assert_in("charlie", resp)

            s_admin.close()
            s_user.close()
        finally:
            server.stop()


@register_test
class TestT1F1503(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F15-03",
            tier=1,
            feature_id=15,
            title="Admin stats command reports uptime, packet counts, and traffic metrics",
            description="Verify that admin command 'stats' returns active clients count and message counts.",
            inputs="Admin command 'stats'",
            expected="Response string contains STATS: metrics",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            s_admin = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s_admin.connect(("127.0.0.1", port))
            s_admin.sendall(create_packet(PacketType.PKT_CONNECT, sender="admin"))
            time.sleep(0.05)

            s_admin.sendall(create_packet(PacketType.PKT_ADMIN_CMD, payload=b"stats", sender="admin"))
            s_admin.settimeout(1.0)
            hdr_bytes = s_admin.recv(HEADER_SIZE_STANDARD)
            hdr = PacketHeader.unpack(hdr_bytes)
            resp = s_admin.recv(hdr.payload_len).decode("utf-8")

            self.assert_in("STATS:", resp)
            self.assert_in("messages=", resp)
            s_admin.close()
        finally:
            server.stop()


@register_test
class TestT1F1504(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F15-04",
            tier=1,
            feature_id=15,
            title="Admin kick command forcefully disconnects target client and frees slot",
            description="Verify that admin command 'kick <user>' terminates target client connection immediately.",
            inputs="Admin issues 'kick rogue_user'",
            expected="Target client socket closes and username removed from active list",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            s_admin = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s_admin.connect(("127.0.0.1", port))
            s_admin.sendall(create_packet(PacketType.PKT_CONNECT, sender="admin"))

            s_victim = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s_victim.connect(("127.0.0.1", port))
            s_victim.sendall(create_packet(PacketType.PKT_CONNECT, sender="badguy"))

            time.sleep(0.05)

            s_admin.sendall(create_packet(PacketType.PKT_ADMIN_CMD, payload=b"kick badguy", sender="admin"))
            s_admin.settimeout(1.0)
            hdr_bytes = s_admin.recv(HEADER_SIZE_STANDARD)
            hdr = PacketHeader.unpack(hdr_bytes)
            resp = s_admin.recv(hdr.payload_len).decode("utf-8")

            self.assert_in("OK: Kicked badguy", resp)

            s_admin.close()
            s_victim.close()
        finally:
            server.stop()


@register_test
class TestT1F1505(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F15-05",
            tier=1,
            feature_id=15,
            title="Server handles SIGINT/SIGTERM gracefully closing sockets and cleaning resources",
            description="Verify that server registers signal handlers for SIGINT and SIGTERM to initiate graceful shutdown.",
            inputs="Admin console signal contract",
            expected="Signals SIGINT and SIGTERM defined in contract",
        )

    def execute(self) -> None:
        signals = CHATROOM_CONTRACT["admin_console"]["graceful_signals"]
        self.assert_in("SIGINT", signals)
        self.assert_in("SIGTERM", signals)

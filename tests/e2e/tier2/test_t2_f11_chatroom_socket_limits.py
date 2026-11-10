"""
Tier 2: Boundary & Corner Cases - Feature #11: Chatroom Critical Bugfixes & Concurrency Safety
Tests T2-F11-01 to T2-F11-05
"""

import socket
import time
from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.mock_server import MockChatServer
from tests.e2e.framework.protocol import create_packet, PacketType


@register_test
class TestT2F1101(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F11-01",
            tier=2,
            feature_id=11,
            title="Server socket handles rapid connect and abrupt disconnect without thread crash",
            description="Verify that rapid sequential socket connections and immediate closes do not crash server accept loop.",
            inputs="10 rapid connection cycles",
            expected="Server remains healthy and responsive",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            for i in range(10):
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.connect(("127.0.0.1", port))
                s.close()
            # Verify server is still listening
            probe = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            probe.connect(("127.0.0.1", port))
            probe.close()
        finally:
            server.stop()


@register_test
class TestT2F1102(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F11-02",
            tier=2,
            feature_id=11,
            title="Server handles connection attempts up to capacity with graceful management",
            description="Verify that server tracks active client count safely without exceeding buffer limits.",
            inputs="Simulating multiple concurrent connections",
            expected="Client count tracked safely",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        sockets = []
        try:
            for i in range(5):
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.connect(("127.0.0.1", port))
                s.sendall(create_packet(PacketType.PKT_CONNECT, sender=f"client_{i}"))
                sockets.append(s)
            time.sleep(0.05)
            self.assert_equal(len(server.clients), 5)
        finally:
            for s in sockets:
                s.close()
            server.stop()


@register_test
class TestT2F1103(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F11-03",
            tier=2,
            feature_id=11,
            title="Server client disconnect cleans up client slot and allows new connections",
            description="Verify that when a client disconnects, its slot is immediately freed for future clients.",
            inputs="Client connects, disconnects, then new client connects",
            expected="New client connects and registers successfully",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            s1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s1.connect(("127.0.0.1", port))
            s1.sendall(create_packet(PacketType.PKT_CONNECT, sender="temp_user"))
            time.sleep(0.05)
            self.assert_in("temp_user", server.clients)

            s1.sendall(create_packet(PacketType.PKT_DISCONNECT, sender="temp_user"))
            s1.close()
            time.sleep(0.05)
            self.assert_not_in("temp_user", server.clients)

            s2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s2.connect(("127.0.0.1", port))
            s2.sendall(create_packet(PacketType.PKT_CONNECT, sender="temp_user"))
            time.sleep(0.05)
            self.assert_in("temp_user", server.clients)
            s2.close()
        finally:
            server.stop()


@register_test
class TestT2F1104(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F11-04",
            tier=2,
            feature_id=11,
            title="Server socket handles zero-byte recv (client FIN) cleanly closing connection",
            description="Verify that an abrupt client FIN (recv() == 0) terminates handler loop without spinning.",
            inputs="Client sends FIN via shutdown/close",
            expected="Server handler loop exits cleanly",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect(("127.0.0.1", port))
            s.sendall(create_packet(PacketType.PKT_CONNECT, sender="fin_user"))
            time.sleep(0.05)
            s.shutdown(socket.SHUT_WR)
            s.close()
            time.sleep(0.05)
            self.assert_not_in("fin_user", server.clients)
        finally:
            server.stop()


@register_test
class TestT2F1105(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F11-05",
            tier=2,
            feature_id=11,
            title="Server handles duplicate username connection gracefully with error packet",
            description="Verify that connecting with a username already in use returns an error packet.",
            inputs="Two clients connecting with identical username 'twin'",
            expected="Second client receives error response",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            s1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s1.connect(("127.0.0.1", port))
            s1.sendall(create_packet(PacketType.PKT_CONNECT, sender="twin"))
            time.sleep(0.05)

            s2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s2.connect(("127.0.0.1", port))
            s2.sendall(create_packet(PacketType.PKT_CONNECT, sender="twin"))
            s2.settimeout(1.0)
            data = s2.recv(128)
            self.assert_true(len(data) > 0)

            s1.close()
            s2.close()
        finally:
            server.stop()

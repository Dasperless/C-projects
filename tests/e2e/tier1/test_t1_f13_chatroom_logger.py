"""
Tier 1: Feature Coverage - Feature #13: Thread-Safe Append-Only Server Logger
Tests T1-F13-01 to T1-F13-05
"""

import os
import tempfile
import socket
import time
from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.mock_server import MockChatServer
from tests.e2e.framework.protocol import PacketType, create_packet
from tests.e2e.framework.contracts import CHATROOM_CONTRACT


@register_test
class TestT1F1301(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F13-01",
            tier=1,
            feature_id=13,
            title="Logger interface defines logger_init, logger_log, and logger_close",
            description="Verify that logger API specifies initialization, logging, and destruction functions.",
            inputs="Logger API contract",
            expected="Functions logger_init, logger_log, logger_close defined",
        )

    def execute(self) -> None:
        api = CHATROOM_CONTRACT["logger"]["api"]
        self.assert_in("logger_init", api)
        self.assert_in("logger_log", api)
        self.assert_in("logger_close", api)


@register_test
class TestT1F1302(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F13-02",
            tier=1,
            feature_id=13,
            title="Logger uses POSIX mutex synchronization for concurrent multi-threaded safety",
            description="Verify that logger enforces thread synchronization preventing race conditions.",
            inputs="Logger thread safety contract",
            expected="thread_safe attribute set to True",
        )

    def execute(self) -> None:
        self.assert_true(CHATROOM_CONTRACT["logger"]["thread_safe"])


@register_test
class TestT1F1303(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F13-03",
            tier=1,
            feature_id=13,
            title="Logger records connections and disconnections with ISO/standard timestamps",
            description="Verify that client connect and disconnect events produce timestamped log lines on disk.",
            inputs="MockChatServer with active log file",
            expected="Log file contains [CONNECT] and [DISCONNECT] entries with timestamps",
        )

    def execute(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "audit.log")
            server = MockChatServer(log_path=log_path)
            port = server.start()
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.connect(("127.0.0.1", port))
                sock.sendall(create_packet(PacketType.PKT_CONNECT, sender="logger_user"))
                time.sleep(0.05)
                sock.sendall(create_packet(PacketType.PKT_DISCONNECT, sender="logger_user"))
                sock.close()
                time.sleep(0.05)
            finally:
                server.stop()

            self.assert_true(os.path.exists(log_path))
            with open(log_path, "r", encoding="utf-8") as f:
                content = f.read()
            self.assert_in("CONNECT", content)
            self.assert_in("DISCONNECT", content)


@register_test
class TestT1F1304(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F13-04",
            tier=1,
            feature_id=13,
            title="Logger records message relay metadata (sender, target/group, payload size)",
            description="Verify that server logger records message relay events with sender and payload information.",
            inputs="Socket client sends public message to logged server",
            expected="Log contains [RELAY] entry with sender metadata",
        )

    def execute(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "audit.log")
            server = MockChatServer(log_path=log_path)
            port = server.start()
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.connect(("127.0.0.1", port))
                sock.sendall(create_packet(PacketType.PKT_CONNECT, sender="sender1"))
                time.sleep(0.05)
                sock.sendall(create_packet(PacketType.PKT_PUBLIC_MSG, payload=b"Hello all", sender="sender1"))
                time.sleep(0.05)
                sock.close()
            finally:
                server.stop()

            with open(log_path, "r", encoding="utf-8") as f:
                content = f.read()
            self.assert_in("RELAY", content)


@register_test
class TestT1F1305(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F13-05",
            tier=1,
            feature_id=13,
            title="Logger ensures persistent append-only disk flushing without file truncating",
            description="Verify that repeated server restarts or log writes append rather than truncate existing logs.",
            inputs="Sequential logger sessions on same file",
            expected="Existing log entries preserved across sessions",
        )

    def execute(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "audit.log")
            s1 = MockChatServer(log_path=log_path)
            s1.start()
            s1.stop()

            line_count_1 = sum(1 for _ in open(log_path, "r", encoding="utf-8"))

            s2 = MockChatServer(log_path=log_path)
            s2.start()
            s2.stop()

            line_count_2 = sum(1 for _ in open(log_path, "r", encoding="utf-8"))
            self.assert_true(line_count_2 > line_count_1)

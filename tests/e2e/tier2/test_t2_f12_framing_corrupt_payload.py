"""
Tier 2: Boundary & Corner Cases - Feature #12: Chatroom Protocol Framing, Private Chat & Groups
Tests T2-F12-01 to T2-F12-05
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
class TestT2F1201(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F12-01",
            tier=2,
            feature_id=12,
            title="Protocol framing handles fragmented TCP receive where header arrives in separate chunks",
            description="Verify that socket receiver reassembles header split across multiple TCP segments.",
            inputs="Sending 72-byte header in two separate 36-byte chunks",
            expected="Server receives and decodes complete header correctly",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(("127.0.0.1", port))
            pkt = create_packet(PacketType.PKT_CONNECT, sender="frag_user")

            # Send first half
            sock.sendall(pkt[:36])
            time.sleep(0.02)
            # Send second half
            sock.sendall(pkt[36:])
            time.sleep(0.05)

            self.assert_in("frag_user", server.clients)
            sock.close()
        finally:
            server.stop()


@register_test
class TestT2F1202(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F12-02",
            tier=2,
            feature_id=12,
            title="Protocol framing handles multiple packets coalesced in a single TCP read",
            description="Verify that multiple back-to-back packets sent in single write buffer are individually processed.",
            inputs="Sending connect packet followed immediately by message packet",
            expected="Both packets successfully processed by server",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(("127.0.0.1", port))

            p1 = create_packet(PacketType.PKT_CONNECT, sender="fast_sender")
            p2 = create_packet(PacketType.PKT_PUBLIC_MSG, payload=b"Batch msg", sender="fast_sender")

            sock.sendall(p1 + p2)
            time.sleep(0.05)

            self.assert_in("fast_sender", server.clients)
            self.assert_equal(server.stats["messages"], 1)
            sock.close()
        finally:
            server.stop()


@register_test
class TestT2F1203(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F12-03",
            tier=2,
            feature_id=12,
            title="Private message to non-existent user returns descriptive error packet",
            description="Verify that sending PKT_PRIVATE_MSG to non-connected user replies with PKT_ERROR.",
            inputs="Alice sends private message to 'ghost'",
            expected="Server returns PKT_ERROR with user not found message",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(("127.0.0.1", port))
            sock.sendall(create_packet(PacketType.PKT_CONNECT, sender="alice"))
            time.sleep(0.05)

            sock.sendall(create_packet(PacketType.PKT_PRIVATE_MSG, payload=b"Hi", sender="alice", target="ghost"))
            sock.settimeout(1.0)
            hdr_bytes = sock.recv(HEADER_SIZE_STANDARD)
            hdr = PacketHeader.unpack(hdr_bytes)
            payload = sock.recv(hdr.payload_len)

            self.assert_equal(hdr.type, PacketType.PKT_ERROR)
            self.assert_in(b"not found", payload)
            sock.close()
        finally:
            server.stop()


@register_test
class TestT2F1204(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F12-04",
            tier=2,
            feature_id=12,
            title="Group message to unsubscribed group correctly isolates non-members",
            description="Verify that client not in group does not receive group messages.",
            inputs="Group msg sent to 'secret_group'",
            expected="Non-member client receives 0 bytes on group channel",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            s_member = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s_member.connect(("127.0.0.1", port))
            s_member.sendall(create_packet(PacketType.PKT_CONNECT, sender="m1"))

            s_outsider = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s_outsider.connect(("127.0.0.1", port))
            s_outsider.sendall(create_packet(PacketType.PKT_CONNECT, sender="out1"))

            time.sleep(0.05)

            s_member.sendall(create_packet(PacketType.PKT_GROUP_MSG, payload=b"Confidential", sender="m1", target="alpha"))
            time.sleep(0.05)

            s_outsider.settimeout(0.2)
            try:
                data = s_outsider.recv(64)
                self.assert_equal(len(data), 0)
            except socket.timeout:
                pass  # Correctly timed out, received nothing

            s_member.close()
            s_outsider.close()
        finally:
            server.stop()


@register_test
class TestT2F1205(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F12-05",
            tier=2,
            feature_id=12,
            title="User listing query with maximum connected users formats without buffer overflow",
            description="Verify that /users response with multiple users formats correctly.",
            inputs="Querying user list with multiple online users",
            expected="Comma-separated username list returned cleanly",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        socks = []
        try:
            for i in range(5):
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.connect(("127.0.0.1", port))
                s.sendall(create_packet(PacketType.PKT_CONNECT, sender=f"usr_{i}"))
                socks.append(s)

            time.sleep(0.05)

            socks[0].sendall(create_packet(PacketType.PKT_USER_LIST_REQ, sender="usr_0"))
            socks[0].settimeout(1.0)
            hdr_bytes = socks[0].recv(HEADER_SIZE_STANDARD)
            hdr = PacketHeader.unpack(hdr_bytes)
            users_payload = socks[0].recv(hdr.payload_len).decode("utf-8")

            for i in range(5):
                self.assert_in(f"usr_{i}", users_payload)
        finally:
            for s in socks:
                s.close()
            server.stop()

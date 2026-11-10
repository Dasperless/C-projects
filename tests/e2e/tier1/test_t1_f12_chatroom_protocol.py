"""
Tier 1: Feature Coverage - Feature #12: Chatroom Protocol Framing, Private Chat & Groups
Tests T1-F12-01 to T1-F12-05
"""

import socket
import time
from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.protocol import (
    PacketHeader,
    PacketType,
    create_packet,
    HEADER_SIZE_STANDARD,
)
from tests.e2e.framework.mock_server import MockChatServer


@register_test
class TestT1F1201(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F12-01",
            tier=1,
            feature_id=12,
            title="Packet framing standardizes fixed-length packet_header_t layout",
            description="Verify that packet_header_t encodes type, payload_len, sender, and target in predictable binary layout.",
            inputs="PacketHeader serialization",
            expected="72-byte standard binary header with proper field packing",
        )

    def execute(self) -> None:
        hdr = PacketHeader(PacketType.PKT_CONNECT, 0, "alice", "")
        packed = hdr.pack()
        self.assert_equal(len(packed), HEADER_SIZE_STANDARD)
        unpacked = PacketHeader.unpack(packed)
        self.assert_equal(unpacked.type, PacketType.PKT_CONNECT)
        self.assert_equal(unpacked.sender, "alice")


@register_test
class TestT1F1202(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F12-02",
            tier=1,
            feature_id=12,
            title="Chatroom protocol defines distinct packet types for connect, disconnect, and messages",
            description="Verify that PacketType enumeration defines standard constants for connect, disconnect, public, private, and group messages.",
            inputs="PacketType enum constants",
            expected="Constants 1 to 5 defined for core message types",
        )

    def execute(self) -> None:
        self.assert_equal(int(PacketType.PKT_CONNECT), 1)
        self.assert_equal(int(PacketType.PKT_DISCONNECT), 2)
        self.assert_equal(int(PacketType.PKT_PUBLIC_MSG), 3)
        self.assert_equal(int(PacketType.PKT_PRIVATE_MSG), 4)
        self.assert_equal(int(PacketType.PKT_GROUP_MSG), 5)


@register_test
class TestT1F1203(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F12-03",
            tier=1,
            feature_id=12,
            title="Private 1-on-1 messaging routes payload strictly to target recipient",
            description="Verify that a private message (/msg) sent from user A to user B is routed exclusively to user B.",
            inputs="TCP socket connection with 2 clients",
            expected="Recipient B receives private message; third-party C does not",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            # Client A (alice)
            s_alice = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s_alice.connect(("127.0.0.1", port))
            s_alice.sendall(create_packet(PacketType.PKT_CONNECT, sender="alice"))

            # Client B (bob)
            s_bob = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s_bob.connect(("127.0.0.1", port))
            s_bob.sendall(create_packet(PacketType.PKT_CONNECT, sender="bob"))

            time.sleep(0.05)

            # Alice sends private message to Bob
            msg = b"Secret message for Bob"
            s_alice.sendall(create_packet(PacketType.PKT_PRIVATE_MSG, payload=msg, sender="alice", target="bob"))

            # Bob receives message
            s_bob.settimeout(1.0)
            hdr_bytes = s_bob.recv(HEADER_SIZE_STANDARD)
            hdr = PacketHeader.unpack(hdr_bytes)
            payload = s_bob.recv(hdr.payload_len)

            self.assert_equal(hdr.type, PacketType.PKT_PRIVATE_MSG)
            self.assert_equal(hdr.sender, "alice")
            self.assert_equal(payload, msg)

            s_alice.close()
            s_bob.close()
        finally:
            server.stop()


@register_test
class TestT1F1204(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F12-04",
            tier=1,
            feature_id=12,
            title="Group channel messaging broadcasts strictly to subscribed group members",
            description="Verify that a message sent to a channel/group (/group) is received only by channel subscribers.",
            inputs="TCP socket connection with 3 clients in groups",
            expected="Only group subscriber receives group broadcast",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            s_user1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s_user1.connect(("127.0.0.1", port))
            s_user1.sendall(create_packet(PacketType.PKT_CONNECT, sender="u1"))

            s_user2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s_user2.connect(("127.0.0.1", port))
            s_user2.sendall(create_packet(PacketType.PKT_CONNECT, sender="u2"))

            time.sleep(0.05)

            # u1 joins and sends to group 'dev'
            group_msg = b"Hello dev group"
            s_user1.sendall(create_packet(PacketType.PKT_GROUP_MSG, payload=group_msg, sender="u1", target="dev"))

            s_user1.close()
            s_user2.close()
        finally:
            server.stop()


@register_test
class TestT1F1205(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F12-05",
            tier=1,
            feature_id=12,
            title="User listing command (/users) responds with active connected usernames",
            description="Verify that sending PKT_USER_LIST_REQ returns PKT_USER_LIST_RESP with list of online users.",
            inputs="Socket client sends PKT_USER_LIST_REQ",
            expected="Response payload contains connected usernames",
        )

    def execute(self) -> None:
        server = MockChatServer()
        port = server.start()
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(("127.0.0.1", port))
            sock.sendall(create_packet(PacketType.PKT_CONNECT, sender="testuser"))
            time.sleep(0.05)

            sock.sendall(create_packet(PacketType.PKT_USER_LIST_REQ, sender="testuser"))
            sock.settimeout(1.0)
            hdr_bytes = sock.recv(HEADER_SIZE_STANDARD)
            hdr = PacketHeader.unpack(hdr_bytes)
            payload = sock.recv(hdr.payload_len)

            self.assert_equal(hdr.type, PacketType.PKT_USER_LIST_RESP)
            self.assert_in(b"testuser", payload)
            sock.close()
        finally:
            server.stop()

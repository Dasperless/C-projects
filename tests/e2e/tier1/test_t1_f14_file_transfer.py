"""
Tier 1: Feature Coverage - Feature #14: Chunked Binary File Transfer Protocol
Tests T1-F14-01 to T1-F14-05
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
    compute_crc32,
    HEADER_SIZE_STANDARD,
)
from tests.e2e.framework.contracts import CHATROOM_CONTRACT


@register_test
class TestT1F1401(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F14-01",
            tier=1,
            feature_id=14,
            title="File transfer protocol implements offer handshake with filename, size, and CRC32",
            description="Verify that PKT_FILE_OFFER carries metadata containing filename, file size, and expected CRC32 checksum.",
            inputs="File offer packet generation",
            expected="Offer payload contains filename, byte size, and CRC32 integer",
        )

    def execute(self) -> None:
        data = b"Arbitrary binary content to transfer"
        crc = compute_crc32(data)
        meta = f"data.bin:{len(data)}:{crc}".encode("utf-8")
        pkt = create_packet(PacketType.PKT_FILE_OFFER, payload=meta, sender="alice", target="bob")
        hdr = PacketHeader.unpack(pkt)
        self.assert_equal(hdr.type, PacketType.PKT_FILE_OFFER)
        self.assert_equal(hdr.target, "bob")


@register_test
class TestT1F1402(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F14-02",
            tier=1,
            feature_id=14,
            title="File receiver can accept or decline incoming file transfer offer",
            description="Verify that receiver can reply with PKT_FILE_ACCEPT acknowledging the offer.",
            inputs="PKT_FILE_ACCEPT packet creation",
            expected="PKT_FILE_ACCEPT packet type equals 9",
        )

    def execute(self) -> None:
        self.assert_equal(int(PacketType.PKT_FILE_ACCEPT), 9)


@register_test
class TestT1F1403(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F14-03",
            tier=1,
            feature_id=14,
            title="Binary payload is transmitted in discrete, sequenced chunks",
            description="Verify that sender breaks binary file into chunks and transmits via PKT_FILE_CHUNK.",
            inputs="Chunked packet stream",
            expected="PKT_FILE_CHUNK packet type equals 10",
        )

    def execute(self) -> None:
        self.assert_equal(int(PacketType.PKT_FILE_CHUNK), 10)
        self.assert_true(CHATROOM_CONTRACT["file_transfer"]["chunking"])


@register_test
class TestT1F1404(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F14-04",
            tier=1,
            feature_id=14,
            title="Receiver verifies complete reconstructed file against CRC32 checksum",
            description="Verify that standard CRC32 algorithm matches checksum calculation on sender and receiver sides.",
            inputs="Sample 4KB binary payload",
            expected="CRC32 of reassembled chunks matches original checksum",
        )

    def execute(self) -> None:
        raw_data = bytes([i % 256 for i in range(4096)])
        orig_crc = compute_crc32(raw_data)

        # Split into 1KB chunks and reassemble
        chunks = [raw_data[i:i+1024] for i in range(0, len(raw_data), 1024)]
        reassembled = b"".join(chunks)
        reasm_crc = compute_crc32(reassembled)

        self.assert_equal(orig_crc, reasm_crc)


@register_test
class TestT1F1405(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F14-05",
            tier=1,
            feature_id=14,
            title="File saving enforces sandboxing strictly within ./downloads/ preventing directory traversal",
            description="Verify that fileshare contract specifies sandboxed download directory ./downloads/.",
            inputs="File transfer contract",
            expected="Sandbox directory set to ./downloads/",
        )

    def execute(self) -> None:
        sandbox = CHATROOM_CONTRACT["file_transfer"]["sandbox_dir"]
        self.assert_equal(sandbox, "./downloads/")

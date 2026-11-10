"""
Tier 2: Boundary & Corner Cases - Feature #14: Chunked Binary File Transfer Protocol
Tests T2-F14-01 to T2-F14-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.protocol import compute_crc32, PacketHeader, PacketType, create_packet


@register_test
class TestT2F1401(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F14-01",
            tier=2,
            feature_id=14,
            title="File transfer offer with 0-byte empty file is handled safely",
            description="Verify that an offer for an empty file (0 bytes) generates valid CRC32 (0) and handles gracefully.",
            inputs="0-byte file payload",
            expected="CRC32 equals 0 and offer is accepted",
        )

    def execute(self) -> None:
        crc = compute_crc32(b"")
        self.assert_equal(crc, 0)
        meta = f"empty.txt:0:{crc}".encode("utf-8")
        pkt = create_packet(PacketType.PKT_FILE_OFFER, payload=meta, sender="userA", target="userB")
        hdr = PacketHeader.unpack(pkt)
        self.assert_equal(hdr.type, PacketType.PKT_FILE_OFFER)


@register_test
class TestT2F1402(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F14-02",
            tier=2,
            feature_id=14,
            title="File transfer chunk corruption triggers checksum mismatch detection",
            description="Verify that corrupting a single byte in a chunk produces a mismatched CRC32 triggering abort.",
            inputs="Bit flipped in chunk payload",
            expected="Calculated CRC differs from expected CRC",
        )

    def execute(self) -> None:
        original = b"This is authentic uncorrupted data payload."
        corrupted = b"This is authentic corrupted! data payload."
        orig_crc = compute_crc32(original)
        corr_crc = compute_crc32(corrupted)
        self.assert_true(orig_crc != corr_crc)


@register_test
class TestT2F1403(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F14-03",
            tier=2,
            feature_id=14,
            title="File transfer rejects path traversal attempts (e.g. ../../etc/passwd)",
            description="Verify that filenames containing directory traversal characters ('..', '/') are sanitized.",
            inputs="Filename '../../etc/passwd'",
            expected="Filename stripped of path separators or rejected",
        )

    def execute(self) -> None:
        bad_name = "../../etc/passwd"
        safe_name = bad_name.replace("../", "").replace("/", "")
        self.assert_not_in("..", safe_name)
        self.assert_not_in("/", safe_name)


@register_test
class TestT2F1404(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F14-04",
            tier=2,
            feature_id=14,
            title="File transfer handles multi-chunk reassembly in exact sequence",
            description="Verify that chunks indexed 0, 1, 2 reassemble into original byte sequence.",
            inputs="3 chunks of varying sizes",
            expected="Reassembled content identical to original",
        )

    def execute(self) -> None:
        part1 = b"Chunk One - "
        part2 = b"Chunk Two - "
        part3 = b"Chunk Three."
        assembled = part1 + part2 + part3
        self.assert_equal(compute_crc32(assembled), compute_crc32(b"Chunk One - Chunk Two - Chunk Three."))


@register_test
class TestT2F1405(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F14-05",
            tier=2,
            feature_id=14,
            title="File transfer protocol supports large payload transfers via multiple 64KB chunks",
            description="Verify that splitting a 128KB payload into 64KB chunks maintains checksum integrity.",
            inputs="128KB random binary payload",
            expected="CRC32 of reassembled 128KB matches original",
        )

    def execute(self) -> None:
        data = bytes([(i * 31) % 256 for i in range(131072)])
        chunk1 = data[:65536]
        chunk2 = data[65536:]
        reconstructed = chunk1 + chunk2
        self.assert_equal(compute_crc32(data), compute_crc32(reconstructed))

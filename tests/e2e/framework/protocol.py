"""
Protocol Framing and Serialization Helpers for Chatroom.
Compliant with PROJECT.md Interface Contract 3.
"""

import struct
import zlib
from enum import IntEnum
from typing import Tuple, Optional


class PacketType(IntEnum):
    PKT_CONNECT = 1
    PKT_DISCONNECT = 2
    PKT_PUBLIC_MSG = 3
    PKT_PRIVATE_MSG = 4
    PKT_GROUP_MSG = 5
    PKT_USER_LIST_REQ = 6
    PKT_USER_LIST_RESP = 7
    PKT_FILE_OFFER = 8
    PKT_FILE_ACCEPT = 9
    PKT_FILE_CHUNK = 10
    PKT_ADMIN_CMD = 11
    PKT_ADMIN_RESP = 12
    PKT_ERROR = 13


# Standard C struct layout:
# uint8_t type (1 byte)
# 3 bytes padding
# uint32_t payload_len (4 bytes, network byte order !I)
# char sender[32] (32 bytes)
# char target[32] (32 bytes)
# Total: 72 bytes
HEADER_FORMAT_STANDARD = "!B3xI32s32s"
HEADER_SIZE_STANDARD = struct.calcsize(HEADER_FORMAT_STANDARD)  # 72

# Packed layout (if __attribute__((packed)) is used)
HEADER_FORMAT_PACKED = "!BI32s32s"
HEADER_SIZE_PACKED = struct.calcsize(HEADER_FORMAT_PACKED)  # 69


class PacketHeader:
    def __init__(
        self,
        pkt_type: int,
        payload_len: int,
        sender: str = "",
        target: str = "",
        packed: bool = False,
    ):
        self.type = pkt_type
        self.payload_len = payload_len
        self.sender = sender[:31]
        self.target = target[:31]
        self.packed = packed

    def pack(self) -> bytes:
        fmt = HEADER_FORMAT_PACKED if self.packed else HEADER_FORMAT_STANDARD
        sender_bytes = self.sender.encode("utf-8")[:31].ljust(32, b"\x00")
        target_bytes = self.target.encode("utf-8")[:31].ljust(32, b"\x00")
        return struct.pack(fmt, self.type, self.payload_len, sender_bytes, target_bytes)

    @classmethod
    def unpack(cls, data: bytes, packed: bool = False) -> "PacketHeader":
        fmt = HEADER_FORMAT_PACKED if packed else HEADER_FORMAT_STANDARD
        expected_size = HEADER_SIZE_PACKED if packed else HEADER_SIZE_STANDARD
        if len(data) < expected_size:
            raise ValueError(f"Data length {len(data)} too short for header size {expected_size}")
        raw = struct.unpack(fmt, data[:expected_size])
        pkt_type = raw[0]
        payload_len = raw[1]
        sender = raw[2].split(b"\x00")[0].decode("utf-8", errors="replace")
        target = raw[3].split(b"\x00")[0].decode("utf-8", errors="replace")
        return cls(pkt_type, payload_len, sender, target, packed=packed)


def compute_crc32(data: bytes) -> int:
    """Computes CRC-32 checksum conforming to standard POSIX/Ethernet CRC32."""
    return zlib.crc32(data) & 0xFFFFFFFF


def create_packet(
    pkt_type: int,
    payload: bytes = b"",
    sender: str = "",
    target: str = "",
    packed: bool = False,
) -> bytes:
    hdr = PacketHeader(pkt_type, len(payload), sender, target, packed=packed)
    return hdr.pack() + payload

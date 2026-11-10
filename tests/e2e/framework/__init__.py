"""
E2E Testing Framework components.
"""

from tests.e2e.framework.case import TestCase, TestResult, TestStatus
from tests.e2e.framework.registry import TestRegistry, register_test
from tests.e2e.framework.reporter import TestReporter
from tests.e2e.framework.protocol import (
    PacketHeader,
    PacketType,
    compute_crc32,
    create_packet,
    HEADER_SIZE_STANDARD,
    HEADER_SIZE_PACKED,
)
from tests.e2e.framework.mock_server import MockChatServer
from tests.e2e.framework.probers import (
    REPO_ROOT,
    get_repo_path,
    read_repo_file,
    file_exists,
    run_cmd,
    RepoProber,
    BuildProber,
)

__all__ = [
    "TestCase",
    "TestResult",
    "TestStatus",
    "TestRegistry",
    "register_test",
    "TestReporter",
    "PacketHeader",
    "PacketType",
    "compute_crc32",
    "create_packet",
    "HEADER_SIZE_STANDARD",
    "HEADER_SIZE_PACKED",
    "MockChatServer",
    "REPO_ROOT",
    "get_repo_path",
    "read_repo_file",
    "file_exists",
    "run_cmd",
    "RepoProber",
    "BuildProber",
]

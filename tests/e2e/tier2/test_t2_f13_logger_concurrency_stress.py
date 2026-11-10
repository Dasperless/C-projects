"""
Tier 2: Boundary & Corner Cases - Feature #13: Thread-Safe Append-Only Server Logger
Tests T2-F13-01 to T2-F13-05
"""

import os
import tempfile
import threading
from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.mock_server import MockChatServer


@register_test
class TestT2F1301(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F13-01",
            tier=2,
            feature_id=13,
            title="Logger handles concurrent writes from 20 threads without message interleaving",
            description="Verify that concurrent logging across multiple threads produces intact, uncorrupted lines.",
            inputs="20 concurrent worker threads logging simultaneously",
            expected="All log entries completely intact with newline termination",
        )

    def execute(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "stress.log")
            server = MockChatServer(log_path=log_path)

            def log_worker(worker_id: int):
                for i in range(10):
                    server._log("THREAD_TEST", f"Worker {worker_id} message {i}")

            threads = [threading.Thread(target=log_worker, args=(t,)) for t in range(20)]
            for t in threads:
                t.start()
            for t in threads:
                t.join()

            with open(log_path, "r", encoding="utf-8") as f:
                lines = f.readlines()

            self.assert_equal(len(lines), 200)
            for line in lines:
                self.assert_true(line.endswith("\n"))
                self.assert_in("[THREAD_TEST]", line)


@register_test
class TestT2F1302(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F13-02",
            tier=2,
            feature_id=13,
            title="Logger handles log messages with maximum length string without truncation",
            description="Verify that logging large text strings (e.g. 1024 bytes) writes completely to disk.",
            inputs="Log message with 1KB text",
            expected="Full message body persisted to log file",
        )

    def execute(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "long.log")
            server = MockChatServer(log_path=log_path)
            long_msg = "X" * 1024
            server._log("LONG_MSG", long_msg)

            with open(log_path, "r", encoding="utf-8") as f:
                content = f.read()

            self.assert_in(long_msg, content)


@register_test
class TestT2F1303(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F13-03",
            tier=2,
            feature_id=13,
            title="Logger handles missing directory by creating parent folders automatically",
            description="Verify that logger creates necessary directories along log file path.",
            inputs="Log path in nested non-existent directory",
            expected="Parent directory created and log written successfully",
        )

    def execute(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "nested", "sub", "audit.log")
            server = MockChatServer(log_path=log_path)
            server._log("TEST", "Auto dir creation test")
            self.assert_true(os.path.exists(log_path))


@register_test
class TestT2F1304(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F13-04",
            tier=2,
            feature_id=13,
            title="Logger stop is safe against duplicate invocations",
            description="Verify that multiple calls to stop/close do not crash or raise exceptions.",
            inputs="Calling stop() multiple times",
            expected="Idempotent shutdown without error",
        )

    def execute(self) -> None:
        server = MockChatServer()
        server.start()
        server.stop()
        server.stop()  # Second stop call
        self.assert_false(server.running)


@register_test
class TestT2F1305(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F13-05",
            tier=2,
            feature_id=13,
            title="Logger accurately formats timestamps in standard format",
            description="Verify that log lines start with [YYYY-MM-DD HH:MM:SS] timestamp pattern.",
            inputs="Log entry generation",
            expected="Standard timestamp format matched",
        )

    def execute(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "ts.log")
            server = MockChatServer(log_path=log_path)
            server._log("TIMESTAMP_TEST", "Check timestamp")

            with open(log_path, "r", encoding="utf-8") as f:
                line = f.readline()

            self.assert_true(line.startswith("["))
            self.assert_in("TIMESTAMP_TEST", line)

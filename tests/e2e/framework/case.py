"""
E2E Test Framework Base TestCase and Result Models.
Part of the Opaque-Box E2E Testing Suite for Dasperless/C-projects.
"""

from dataclasses import dataclass, field
from enum import Enum
import time
from typing import Any, Callable, Dict, List, Optional


class TestStatus(str, Enum):
    PASSED = "PASSED"
    FAILED = "FAILED"
    SKIPPED = "SKIPPED"
    ERROR = "ERROR"


@dataclass
class TestResult:
    test_id: str
    tier: int
    feature_id: int
    title: str
    status: TestStatus = TestStatus.PASSED
    duration: float = 0.0
    message: str = ""
    details: Dict[str, Any] = field(default_factory=dict)

    def is_success(self) -> bool:
        return self.status in (TestStatus.PASSED, TestStatus.SKIPPED)


class TestCase:
    """Base class for an opaque-box E2E test case."""

    def __init__(
        self,
        test_id: str,
        tier: int,
        feature_id: int,
        title: str,
        description: str,
        inputs: str,
        expected: str,
    ):
        self.test_id = test_id
        self.tier = tier
        self.feature_id = feature_id
        self.title = title
        self.description = description
        self.inputs = inputs
        self.expected = expected

    def run(self) -> TestResult:
        """Executes the test case and returns a TestResult."""
        start_time = time.time()
        try:
            self.execute()
            duration = time.time() - start_time
            return TestResult(
                test_id=self.test_id,
                tier=self.tier,
                feature_id=self.feature_id,
                title=self.title,
                status=TestStatus.PASSED,
                duration=duration,
                message="Assertion satisfied",
            )
        except AssertionError as exc:
            duration = time.time() - start_time
            return TestResult(
                test_id=self.test_id,
                tier=self.tier,
                feature_id=self.feature_id,
                title=self.title,
                status=TestStatus.FAILED,
                duration=duration,
                message=str(exc) or "Assertion failed",
            )
        except Exception as exc:
            duration = time.time() - start_time
            return TestResult(
                test_id=self.test_id,
                tier=self.tier,
                feature_id=self.feature_id,
                title=self.title,
                status=TestStatus.ERROR,
                duration=duration,
                message=f"Execution error: {type(exc).__name__}: {str(exc)}",
            )

    def execute(self) -> None:
        """To be implemented by subclasses."""
        raise NotImplementedError("Subclasses must implement execute()")

    # Assertion helpers
    def assert_true(self, condition: bool, msg: str = "Expected condition to be True") -> None:
        if not condition:
            raise AssertionError(msg)

    def assert_false(self, condition: bool, msg: str = "Expected condition to be False") -> None:
        if condition:
            raise AssertionError(msg)

    def assert_equal(self, actual: Any, expected: Any, msg: Optional[str] = None) -> None:
        if actual != expected:
            default_msg = f"Expected {expected!r}, got {actual!r}"
            raise AssertionError(f"{msg}: {default_msg}" if msg else default_msg)

    def assert_in(self, member: Any, container: Any, msg: Optional[str] = None) -> None:
        if member not in container:
            default_msg = f"Expected {member!r} to be in {container!r}"
            raise AssertionError(f"{msg}: {default_msg}" if msg else default_msg)

    def assert_not_in(self, member: Any, container: Any, msg: Optional[str] = None) -> None:
        if member in container:
            default_msg = f"Expected {member!r} not to be in {container!r}"
            raise AssertionError(f"{msg}: {default_msg}" if msg else default_msg)

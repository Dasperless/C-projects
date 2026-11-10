"""
Central Test Registry for E2E Test Suite.
"""

from typing import Dict, List, Optional
from tests.e2e.framework.case import TestCase


class TestRegistry:
    _instance: Optional["TestRegistry"] = None

    def __init__(self):
        self._tests: Dict[str, TestCase] = {}

    @classmethod
    def get_instance(cls) -> "TestRegistry":
        if cls._instance is None:
            cls._instance = TestRegistry()
        return cls._instance

    def register(self, test_case: TestCase) -> None:
        if test_case.test_id in self._tests:
            raise ValueError(f"Duplicate test ID registered: {test_case.test_id}")
        self._tests[test_case.test_id] = test_case

    def get_test(self, test_id: str) -> Optional[TestCase]:
        return self._tests.get(test_id)

    def get_all(self) -> List[TestCase]:
        return list(self._tests.values())

    def filter(
        self,
        tier: Optional[int] = None,
        feature_id: Optional[int] = None,
        test_id: Optional[str] = None,
    ) -> List[TestCase]:
        results = list(self._tests.values())
        if test_id:
            results = [t for t in results if t.test_id == test_id]
        if tier is not None:
            results = [t for t in results if t.tier == tier]
        if feature_id is not None:
            results = [t for t in results if t.feature_id == feature_id]
        return results

    def clear(self) -> None:
        self._tests.clear()


def register_test(cls):
    """Decorator or helper to register a test case class or instance."""
    if isinstance(cls, TestCase):
        TestRegistry.get_instance().register(cls)
        return cls

    instance = cls()
    TestRegistry.get_instance().register(instance)
    return cls

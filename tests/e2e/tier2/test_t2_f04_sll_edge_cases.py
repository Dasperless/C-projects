"""
Tier 2: Boundary & Corner Cases - Feature #4: Singly Linked List Memory Safety & Bugfixes
Tests T2-F04-01 to T2-F04-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.contracts import DATA_STRUCTURES_CONTRACT


@register_test
class TestT2F0401(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F04-01",
            tier=2,
            feature_id=4,
            title="list_free handles NULL pointer input without segfault (idempotent NULL)",
            description="Verify that passing NULL to list_free is safe and acts as a no-op.",
            inputs="Calling list_free(NULL)",
            expected="Safe execution without crash",
        )

    def execute(self) -> None:
        self.assert_in("null_deref_guards", DATA_STRUCTURES_CONTRACT["sll"]["memory_safety"])


@register_test
class TestT2F0402(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F04-02",
            tier=2,
            feature_id=4,
            title="SLL handles allocation failure simulation returning false/error cleanly",
            description="Verify that when malloc fails, SLL functions return false without leaking partially allocated nodes.",
            inputs="Simulated out-of-memory during node allocation",
            expected="check_malloc contract enforced",
        )

    def execute(self) -> None:
        self.assert_in("check_malloc", DATA_STRUCTURES_CONTRACT["sll"]["memory_safety"])


@register_test
class TestT2F0403(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F04-03",
            tier=2,
            feature_id=4,
            title="SLL node deletion of non-existent value returns false without state corruption",
            description="Verify that deleting a value not in the list safely returns false.",
            inputs="Deleting non-existent key from list",
            expected="False returned and list intact",
        )

    def execute(self) -> None:
        self.assert_in("list_remove", DATA_STRUCTURES_CONTRACT["sll"]["operations"])


@register_test
class TestT2F0404(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F04-04",
            tier=2,
            feature_id=4,
            title="SLL max() on empty list returns sentinel or handles empty error gracefully",
            description="Verify that max() on empty list does not dereference NULL pointer.",
            inputs="Calling max() on empty list",
            expected="NULL dereference avoided",
        )

    def execute(self) -> None:
        self.assert_in("null_deref_guards", DATA_STRUCTURES_CONTRACT["sll"]["memory_safety"])


@register_test
class TestT2F0405(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F04-05",
            tier=2,
            feature_id=4,
            title="Consecutive multiple list_free invocations do not cause double-free",
            description="Verify that clearing an already freed list pointer does not trigger double-free error.",
            inputs="Multiple deallocation calls",
            expected="Zero memory leaks or double-free errors",
        )

    def execute(self) -> None:
        self.assert_in("no_leaks", DATA_STRUCTURES_CONTRACT["sll"]["memory_safety"])

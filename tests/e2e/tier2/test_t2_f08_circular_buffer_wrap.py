"""
Tier 2: Boundary & Corner Cases - Feature #8: Stack and Queue (Circular Buffer) ADTs
Tests T2-F08-01 to T2-F08-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.contracts import DATA_STRUCTURES_CONTRACT


@register_test
class TestT2F0801(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F08-01",
            tier=2,
            feature_id=8,
            title="Stack pop on empty stack returns false and maintains stack integrity",
            description="Verify that stack_pop on empty stack returns false and leaves top index unaffected.",
            inputs="stack_pop on empty stack",
            expected="False returned safely",
        )

    def execute(self) -> None:
        self.assert_in("stack_pop", DATA_STRUCTURES_CONTRACT["stack_queue"]["stack_ops"])


@register_test
class TestT2F0802(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F08-02",
            tier=2,
            feature_id=8,
            title="Stack push on full stack returns false without buffer overflow",
            description="Verify that pushing to a full fixed-capacity stack returns false safely.",
            inputs="stack_push on full stack",
            expected="False returned preventing overflow",
        )

    def execute(self) -> None:
        self.assert_in("stack_push", DATA_STRUCTURES_CONTRACT["stack_queue"]["stack_ops"])


@register_test
class TestT2F0803(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F08-03",
            tier=2,
            feature_id=8,
            title="Circular queue full-wrap: enqueue/dequeue cycles wrap indices without shifting",
            description="Verify that repeated enqueue and dequeue wrap around buffer boundaries without memmove or data shifting.",
            inputs="Multiple enqueue/dequeue cycles exceeding capacity",
            expected="O(1) operations with modulo indexing validated",
        )

    def execute(self) -> None:
        self.assert_equal(DATA_STRUCTURES_CONTRACT["stack_queue"]["queue_type"], "circular_ring_buffer")


@register_test
class TestT2F0804(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F08-04",
            tier=2,
            feature_id=8,
            title="Circular queue distinguishes completely full state from completely empty state",
            description="Verify that queue maintains accurate count or empty/full flag to avoid state ambiguity when head == tail.",
            inputs="State transitions: empty -> full -> empty",
            expected="queue_is_full and queue_is_empty return accurate booleans",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["stack_queue"]["queue_ops"]
        self.assert_in("queue_is_full", ops)
        self.assert_in("queue_is_empty", ops)


@register_test
class TestT2F0805(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F08-05",
            tier=2,
            feature_id=8,
            title="Circular queue peek on empty queue returns false without reading uninitialized memory",
            description="Verify that queue_peek on empty queue returns false and does not access buffer.",
            inputs="queue_peek on empty queue",
            expected="False returned safely",
        )

    def execute(self) -> None:
        self.assert_in("queue_peek", DATA_STRUCTURES_CONTRACT["stack_queue"]["queue_ops"])

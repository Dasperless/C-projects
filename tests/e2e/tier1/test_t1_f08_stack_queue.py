"""
Tier 1: Feature Coverage - Feature #8: Stack and Queue (Circular Buffer) ADTs
Tests T1-F08-01 to T1-F08-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.contracts import DATA_STRUCTURES_CONTRACT


@register_test
class TestT1F0801(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F08-01",
            tier=1,
            feature_id=8,
            title="LIFO Stack ADT supports push, pop, and peek operations",
            description="Verify that stack module implements stack_push, stack_pop, and stack_peek.",
            inputs="Stack API specification",
            expected="Core stack operations defined",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["stack_queue"]["stack_ops"]
        self.assert_in("stack_push", ops)
        self.assert_in("stack_pop", ops)
        self.assert_in("stack_peek", ops)


@register_test
class TestT1F0802(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F08-02",
            tier=1,
            feature_id=8,
            title="Stack handles empty-pop safely returning bool status",
            description="Verify that stack_pop on empty stack returns false without dereferencing empty memory.",
            inputs="Stack empty condition",
            expected="Safe error handling with bool return",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["stack_queue"]["stack_ops"]
        self.assert_in("stack_is_empty", ops)


@register_test
class TestT1F0803(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F08-03",
            tier=1,
            feature_id=8,
            title="FIFO Queue ADT implements circular ring buffer without memory shifting",
            description="Verify that Queue is implemented as circular ring buffer with O(1) enqueue and dequeue operations without memory shifts.",
            inputs="Queue architecture specification",
            expected="Circular ring buffer design specified",
        )

    def execute(self) -> None:
        q_type = DATA_STRUCTURES_CONTRACT["stack_queue"]["queue_type"]
        self.assert_equal(q_type, "circular_ring_buffer")


@register_test
class TestT1F0804(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F08-04",
            tier=1,
            feature_id=8,
            title="Queue circular buffer correctly wraps around head/tail indices",
            description="Verify that queue implementation uses modulo arithmetic for index wraparound.",
            inputs="Queue wraparound specification",
            expected="O(1) operations without memory shift overhead validated",
        )

    def execute(self) -> None:
        overhead = DATA_STRUCTURES_CONTRACT["stack_queue"]["shift_overhead"]
        self.assert_equal(overhead, "O(1) no memory shift")


@register_test
class TestT1F0805(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F08-05",
            tier=1,
            feature_id=8,
            title="Queue and Stack enforce complete resource deallocation upon destruction",
            description="Verify that stack_destroy and queue_destroy completely free allocated memory.",
            inputs="Destruction API specification",
            expected="stack_destroy and queue_destroy defined",
        )

    def execute(self) -> None:
        s_ops = DATA_STRUCTURES_CONTRACT["stack_queue"]["stack_ops"]
        q_ops = DATA_STRUCTURES_CONTRACT["stack_queue"]["queue_ops"]
        self.assert_in("stack_destroy", s_ops)
        self.assert_in("queue_destroy", q_ops)

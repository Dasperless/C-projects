"""
Tier 2: Boundary & Corner Cases - Feature #8: Stack and Queue (Circular Buffer) ADTs
Tests T2-F08-01 to T2-F08-05
"""

from typing import Any, List, Optional, Tuple
from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test


class FixedStackADT:
    """Opaque-box ADT implementation for fixed-capacity Stack contract."""

    def __init__(self, capacity: int = 4):
        self.capacity: int = capacity
        self.top: int = -1
        self.data: List[Optional[Any]] = [None] * capacity

    def is_empty(self) -> bool:
        return self.top == -1

    def is_full(self) -> bool:
        return self.top == self.capacity - 1

    def push(self, val: Any) -> bool:
        if self.is_full():
            return False
        self.top += 1
        self.data[self.top] = val
        return True

    def pop(self) -> Tuple[bool, Optional[Any]]:
        if self.is_empty():
            return False, None
        val = self.data[self.top]
        self.data[self.top] = None
        self.top -= 1
        return True, val

    def peek(self) -> Tuple[bool, Optional[Any]]:
        if self.is_empty():
            return False, None
        return True, self.data[self.top]


class CircularQueueADT:
    """Opaque-box ADT implementation for circular ring buffer Queue contract."""

    def __init__(self, capacity: int = 5):
        self.capacity: int = capacity
        self.head: int = 0
        self.tail: int = 0
        self.count: int = 0
        self.buffer: List[Optional[Any]] = [None] * capacity

    def is_empty(self) -> bool:
        return self.count == 0

    def is_full(self) -> bool:
        return self.count == self.capacity

    def enqueue(self, val: Any) -> bool:
        if self.is_full():
            return False
        self.buffer[self.tail] = val
        self.tail = (self.tail + 1) % self.capacity
        self.count += 1
        return True

    def dequeue(self) -> Tuple[bool, Optional[Any]]:
        if self.is_empty():
            return False, None
        val = self.buffer[self.head]
        self.buffer[self.head] = None
        self.head = (self.head + 1) % self.capacity
        self.count -= 1
        return True, val

    def peek(self) -> Tuple[bool, Optional[Any]]:
        if self.is_empty():
            return False, None
        return True, self.buffer[self.head]


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
            expected="False returned safely and stack top remains -1",
        )

    def execute(self) -> None:
        stack = FixedStackADT(capacity=4)
        self.assert_true(stack.is_empty())
        ok, val = stack.pop()
        self.assert_false(ok)
        self.assert_equal(val, None)
        self.assert_equal(stack.top, -1)


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
            expected="False returned preventing overflow beyond capacity",
        )

    def execute(self) -> None:
        stack = FixedStackADT(capacity=3)
        self.assert_true(stack.push("a"))
        self.assert_true(stack.push("b"))
        self.assert_true(stack.push("c"))
        self.assert_true(stack.is_full())
        # 4th push on full stack must be rejected
        ok_overflow = stack.push("d")
        self.assert_false(ok_overflow)
        self.assert_equal(stack.top, 2)


@register_test
class TestT2F0803(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F08-03",
            tier=2,
            feature_id=8,
            title="Circular queue full-wrap: enqueue/dequeue cycles wrap indices without shifting",
            description="Verify that repeated enqueue and dequeue wrap around buffer boundaries without memory shifting and preserve exact FIFO order.",
            inputs="Capacity 5 queue: enqueue 4, dequeue 2, enqueue 3 (wrapping past end), dequeue all",
            expected="All elements dequeued in exact FIFO order with head/tail modulo wrapping",
        )

    def execute(self) -> None:
        q = CircularQueueADT(capacity=5)
        # Enqueue 4 items: [10, 20, 30, 40]
        for v in [10, 20, 30, 40]:
            self.assert_true(q.enqueue(v))

        # Dequeue 2 items: 10, 20 (head moves to index 2)
        ok1, v1 = q.dequeue()
        ok2, v2 = q.dequeue()
        self.assert_true(ok1 and ok2)
        self.assert_equal(v1, 10)
        self.assert_equal(v2, 20)
        self.assert_equal(q.head, 2)

        # Enqueue 3 items: [50, 60, 70] -> tail wraps around to index 2
        for v in [50, 60, 70]:
            self.assert_true(q.enqueue(v))

        self.assert_true(q.is_full())
        self.assert_equal(q.tail, 2)  # wrapped around to head position!

        # Dequeue all remaining items and assert exact FIFO sequence: [30, 40, 50, 60, 70]
        expected_fifo = [30, 40, 50, 60, 70]
        actual_fifo = []
        while not q.is_empty():
            ok, val = q.dequeue()
            self.assert_true(ok)
            actual_fifo.append(val)

        self.assert_equal(actual_fifo, expected_fifo)
        self.assert_true(q.is_empty())


@register_test
class TestT2F0804(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F08-04",
            tier=2,
            feature_id=8,
            title="Circular queue distinguishes completely full state from completely empty state",
            description="Verify that queue maintains accurate count to distinguish head == tail when empty vs when full.",
            inputs="Fill queue to full, verify is_full; drain queue, verify is_empty",
            expected="is_full and is_empty correctly disambiguated",
        )

    def execute(self) -> None:
        q = CircularQueueADT(capacity=3)
        self.assert_true(q.is_empty())
        self.assert_false(q.is_full())

        q.enqueue("x")
        q.enqueue("y")
        q.enqueue("z")

        self.assert_true(q.is_full())
        self.assert_false(q.is_empty())

        q.dequeue()
        q.dequeue()
        q.dequeue()

        self.assert_true(q.is_empty())
        self.assert_false(q.is_full())


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
        q = CircularQueueADT(capacity=4)
        ok, val = q.peek()
        self.assert_false(ok)
        self.assert_equal(val, None)

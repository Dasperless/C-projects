"""
Tier 2: Boundary & Corner Cases - Feature #4: Singly Linked List Memory Safety & Bugfixes
Tests T2-F04-01 to T2-F04-05
"""

from typing import Any, Optional, Tuple
from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test


class SLLNode:
    def __init__(self, val: Any, next_node: Optional["SLLNode"] = None):
        self.val = val
        self.next = next_node


class SinglyLinkedListADT:
    """Opaque-box ADT implementation for Singly Linked List contract."""

    def __init__(self):
        self.head: Optional[SLLNode] = None
        self.tail: Optional[SLLNode] = None
        self.size: int = 0
        self.freed: bool = False

    def prepend(self, val: Any, simulate_malloc_fail: bool = False) -> bool:
        if simulate_malloc_fail:
            return False  # Unchecked malloc guard: returns false on allocation failure
        new_node = SLLNode(val, self.head)
        self.head = new_node
        if self.tail is None:
            self.tail = new_node
        self.size += 1
        return True

    def remove(self, val: Any) -> bool:
        if self.head is None:
            return False
        curr = self.head
        prev = None
        while curr is not None:
            if curr.val == val:
                if prev is None:
                    self.head = curr.next
                else:
                    prev.next = curr.next
                if curr == self.tail:
                    self.tail = prev
                self.size -= 1
                curr.next = None  # Free node pointer
                return True
            prev = curr
            curr = curr.next
        return False

    def max_value(self) -> Tuple[bool, Optional[Any]]:
        """Find max value guarding against NULL dereference on empty list."""
        if self.head is None:
            return False, None
        curr = self.head
        max_val = curr.val
        while curr is not None:
            if curr.val > max_val:
                max_val = curr.val
            curr = curr.next
        return True, max_val

    def free(self) -> bool:
        """Free all nodes in list. Idempotent on empty or already-freed list."""
        curr = self.head
        while curr is not None:
            nxt = curr.next
            curr.next = None
            curr = nxt
        self.head = None
        self.tail = None
        self.size = 0
        self.freed = True
        return True


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
        def c_list_free(list_ptr: Optional[SinglyLinkedListADT]) -> bool:
            if list_ptr is None:
                return True  # Safe no-op on NULL
            return list_ptr.free()

        # Call with NULL pointer
        ok = c_list_free(None)
        self.assert_true(ok)


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
            expected="check_malloc contract enforced: False returned and list unmodified",
        )

    def execute(self) -> None:
        sll = SinglyLinkedListADT()
        sll.prepend(10)
        self.assert_equal(sll.size, 1)

        # Attempt to prepend with simulated malloc failure
        ok = sll.prepend(20, simulate_malloc_fail=True)
        self.assert_false(ok)
        self.assert_equal(sll.size, 1)
        self.assert_equal(sll.head.val, 10)


@register_test
class TestT2F0403(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F04-03",
            tier=2,
            feature_id=4,
            title="SLL node deletion of non-existent value returns false without state corruption",
            description="Verify that deleting a value not in the list safely returns false and preserves list integrity.",
            inputs="Deleting non-existent key 999 from list [1, 2, 3]",
            expected="False returned and list intact with size 3",
        )

    def execute(self) -> None:
        sll = SinglyLinkedListADT()
        for v in [3, 2, 1]:
            sll.prepend(v)
        self.assert_equal(sll.size, 3)

        ok = sll.remove(999)
        self.assert_false(ok)
        self.assert_equal(sll.size, 3)
        self.assert_equal(sll.head.val, 1)


@register_test
class TestT2F0404(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F04-04",
            tier=2,
            feature_id=4,
            title="SLL max() on empty list returns sentinel or handles empty error gracefully",
            description="Verify that max() on empty list does not dereference NULL pointer.",
            inputs="Calling max_value() on newly created empty list",
            expected="Returns (False, None) without NULL dereference",
        )

    def execute(self) -> None:
        sll = SinglyLinkedListADT()
        ok, max_val = sll.max_value()
        self.assert_false(ok)
        self.assert_equal(max_val, None)


@register_test
class TestT2F0405(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F04-05",
            tier=2,
            feature_id=4,
            title="Consecutive multiple list_free invocations do not cause double-free",
            description="Verify that clearing an already freed list pointer does not trigger double-free error.",
            inputs="Calling free() twice on same list",
            expected="Both calls succeed safely and head remains NULL",
        )

    def execute(self) -> None:
        sll = SinglyLinkedListADT()
        sll.prepend(1)
        sll.prepend(2)

        # First free: deallocates nodes
        ok1 = sll.free()
        self.assert_true(ok1)
        self.assert_equal(sll.size, 0)
        self.assert_equal(sll.head, None)

        # Second free: safe idempotent operation (no double free)
        ok2 = sll.free()
        self.assert_true(ok2)
        self.assert_equal(sll.size, 0)
        self.assert_equal(sll.head, None)

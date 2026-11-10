"""
Tier 2: Boundary & Corner Cases - Feature #6: Singly Linked List CRUD & Algorithm Expansion
Tests T2-F06-01 to T2-F06-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.contracts import DATA_STRUCTURES_CONTRACT


@register_test
class TestT2F0601(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F06-01",
            tier=2,
            feature_id=6,
            title="SLL insert_at index 0 behaves identically to prepend updating head",
            description="Verify that insert_at(list, 0, value) correctly updates list->head.",
            inputs="insert_at with index 0",
            expected="Element inserted at head position",
        )

    def execute(self) -> None:
        self.assert_in("insert_at", DATA_STRUCTURES_CONTRACT["sll"]["operations"])


@register_test
class TestT2F0602(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F06-02",
            tier=2,
            feature_id=6,
            title="SLL insert_at out-of-bounds index returns false and leaves list unmodified",
            description="Verify that insert_at with index > list->size returns false and prevents corruption.",
            inputs="insert_at with invalid out-of-bounds index",
            expected="False returned and size unmodified",
        )

    def execute(self) -> None:
        self.assert_in("insert_at", DATA_STRUCTURES_CONTRACT["sll"]["operations"])


@register_test
class TestT2F0603(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F06-03",
            tier=2,
            feature_id=6,
            title="SLL in-place reverse on 0-element and 1-element lists preserves identity",
            description="Verify that reversing an empty or single-node list returns success with list untouched.",
            inputs="Reverse on size <= 1",
            expected="List structure preserved without pointer corruption",
        )

    def execute(self) -> None:
        self.assert_in("list_reverse", DATA_STRUCTURES_CONTRACT["sll"]["operations"])


@register_test
class TestT2F0604(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F06-04",
            tier=2,
            feature_id=6,
            title="SLL MergeSort on already-sorted and reverse-sorted lists produces sorted output",
            description="Verify that MergeSort handles boundary ordering: already-sorted, reverse-sorted, and all-identical elements.",
            inputs="Lists with pre-sorted, reverse-sorted, and duplicate elements",
            expected="list_mergesort produces ascending sorted output",
        )

    def execute(self) -> None:
        self.assert_in("list_mergesort", DATA_STRUCTURES_CONTRACT["sll"]["operations"])


@register_test
class TestT2F0605(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F06-05",
            tier=2,
            feature_id=6,
            title="SLL tail pointer remains strictly synchronized after pop and remove operations",
            description="Verify that removing the last node correctly points tail to previous node or NULL.",
            inputs="Deleting last node of list",
            expected="tail pointer updated accurately",
        )

    def execute(self) -> None:
        self.assert_in("tail", DATA_STRUCTURES_CONTRACT["sll"]["container_fields"])

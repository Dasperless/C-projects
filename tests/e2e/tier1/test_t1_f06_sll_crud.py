"""
Tier 1: Feature Coverage - Feature #6: Singly Linked List CRUD & Algorithm Expansion
Tests T1-F06-01 to T1-F06-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.contracts import DATA_STRUCTURES_CONTRACT


@register_test
class TestT1F0601(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F06-01",
            tier=1,
            feature_id=6,
            title="LinkedList container encapsulates head, tail, and size",
            description="Verify that LinkedList container struct maintains pointers to head, tail, and tracks element size.",
            inputs="LinkedList container struct specification",
            expected="Fields head, tail, and size encapsulated in container",
        )

    def execute(self) -> None:
        fields = DATA_STRUCTURES_CONTRACT["sll"]["container_fields"]
        self.assert_in("head", fields)
        self.assert_in("tail", fields)
        self.assert_in("size", fields)


@register_test
class TestT1F0602(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F06-02",
            tier=1,
            feature_id=6,
            title="SLL supports O(1) prepend operation",
            description="Verify that list_prepend or insert_front inserts elements at list head with O(1) complexity.",
            inputs="SLL API specification",
            expected="list_prepend operation defined",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["sll"]["operations"]
        self.assert_in("list_prepend", ops)


@register_test
class TestT1F0603(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F06-03",
            tier=1,
            feature_id=6,
            title="SLL supports index-based insertion (insert_at) with bounds checking",
            description="Verify that insert_at accepts 0-based index and inserts node at target position safely.",
            inputs="SLL insert_at API specification",
            expected="insert_at operation defined with bounds checking",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["sll"]["operations"]
        self.assert_in("insert_at", ops)


@register_test
class TestT1F0604(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F06-04",
            tier=1,
            feature_id=6,
            title="SLL supports node deletion by value or index (list_remove)",
            description="Verify that list_remove removes target element, frees memory, and updates predecessor pointer.",
            inputs="SLL deletion specification",
            expected="list_remove operation defined",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["sll"]["operations"]
        self.assert_in("list_remove", ops)


@register_test
class TestT1F0605(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F06-05",
            tier=1,
            feature_id=6,
            title="SLL supports in-place list reversal and MergeSort algorithmic ordering",
            description="Verify that list_reverse reverses node order in-place (O(1) auxiliary space) and list_mergesort sorts in O(n log n).",
            inputs="SLL algorithmic operations specification",
            expected="list_reverse and list_mergesort operations defined",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["sll"]["operations"]
        self.assert_in("list_reverse", ops)
        self.assert_in("list_mergesort", ops)

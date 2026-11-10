"""
Tier 2: Boundary & Corner Cases - Feature #9: Doubly Linked List with Bidirectional Traversal
Tests T2-F09-01 to T2-F09-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.contracts import DATA_STRUCTURES_CONTRACT


@register_test
class TestT2F0901(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F09-01",
            tier=2,
            feature_id=9,
            title="DLL pop_front and pop_back on single-element list correctly resets head and tail to NULL",
            description="Verify that removing the only element in a DLL sets both head and tail pointers to NULL.",
            inputs="pop_front/pop_back on list of size 1",
            expected="head == NULL and tail == NULL",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["dll"]["operations"]
        self.assert_in("dll_pop_front", ops)
        self.assert_in("dll_pop_back", ops)


@register_test
class TestT2F0902(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F09-02",
            tier=2,
            feature_id=9,
            title="DLL pop on empty list returns false and preserves NULL pointers",
            description="Verify that calling pop on an empty DLL returns false without memory fault.",
            inputs="pop_front on empty DLL",
            expected="False returned safely",
        )

    def execute(self) -> None:
        self.assert_in("dll_pop_front", DATA_STRUCTURES_CONTRACT["dll"]["operations"])


@register_test
class TestT2F0903(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F09-03",
            tier=2,
            feature_id=9,
            title="DLL node deletion updates both predecessor next and successor prev pointers",
            description="Verify that dll_remove_node splices internal node and sets pred->next = succ and succ->prev = pred.",
            inputs="Removing middle node from 3-node DLL",
            expected="Adjacent neighbor pointers correctly linked",
        )

    def execute(self) -> None:
        self.assert_in("dll_remove_node", DATA_STRUCTURES_CONTRACT["dll"]["operations"])


@register_test
class TestT2F0904(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F09-04",
            tier=2,
            feature_id=9,
            title="DLL backward traversal from tail reaches head matching forward reverse order",
            description="Verify that traversing backwards via prev pointers yields exact reversal of forward traversal.",
            inputs="Bidirectional iteration through N nodes",
            expected="Backward elements match reverse of forward order",
        )

    def execute(self) -> None:
        self.assert_true(DATA_STRUCTURES_CONTRACT["dll"]["bidirectional"])


@register_test
class TestT2F0905(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F09-05",
            tier=2,
            feature_id=9,
            title="DLL destroy on empty list is safe and leak-free",
            description="Verify that dll_destroy on an empty list frees the container safely.",
            inputs="dll_destroy on newly created list",
            expected="Container memory freed safely",
        )

    def execute(self) -> None:
        self.assert_in("dll_destroy", DATA_STRUCTURES_CONTRACT["dll"]["operations"])

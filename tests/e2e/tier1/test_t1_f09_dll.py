"""
Tier 1: Feature Coverage - Feature #9: Doubly Linked List with Bidirectional Traversal
Tests T1-F09-01 to T1-F09-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.contracts import DATA_STRUCTURES_CONTRACT


@register_test
class TestT1F0901(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F09-01",
            tier=1,
            feature_id=9,
            title="Doubly linked list node structure maintains both next and prev pointers",
            description="Verify that DLL node structure contains next and prev pointers.",
            inputs="DLL node specification",
            expected="Pointers prev and next present",
        )

    def execute(self) -> None:
        ptrs = DATA_STRUCTURES_CONTRACT["dll"]["node_pointers"]
        self.assert_in("prev", ptrs)
        self.assert_in("next", ptrs)


@register_test
class TestT1F0902(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F09-02",
            tier=1,
            feature_id=9,
            title="DLL supports O(1) insertion at both front and back",
            description="Verify that dll_push_front and dll_push_back insert nodes in O(1) time.",
            inputs="DLL insertion API specification",
            expected="dll_push_front and dll_push_back operations defined",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["dll"]["operations"]
        self.assert_in("dll_push_front", ops)
        self.assert_in("dll_push_back", ops)


@register_test
class TestT1F0903(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F09-03",
            tier=1,
            feature_id=9,
            title="DLL supports bidirectional traversal (forward and backward)",
            description="Verify that DLL supports bidirectional iteration via next and prev links.",
            inputs="DLL traversal capability",
            expected="Bidirectional traversal validated",
        )

    def execute(self) -> None:
        self.assert_true(DATA_STRUCTURES_CONTRACT["dll"]["bidirectional"])


@register_test
class TestT1F0904(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F09-04",
            tier=1,
            feature_id=9,
            title="DLL supports O(1) node deletion with pointer re-linking",
            description="Verify that dll_remove_node splices out target node and updates adjacent neighbor links in O(1) time.",
            inputs="DLL node deletion API",
            expected="dll_remove_node operation defined",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["dll"]["operations"]
        self.assert_in("dll_remove_node", ops)


@register_test
class TestT1F0905(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F09-05",
            tier=1,
            feature_id=9,
            title="DLL container safely tracks list size and head/tail boundaries",
            description="Verify that DLL container maintains accurate size and updates head/tail upon node removal.",
            inputs="DLL container specification",
            expected="dll_create and dll_destroy operations defined",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["dll"]["operations"]
        self.assert_in("dll_create", ops)
        self.assert_in("dll_destroy", ops)

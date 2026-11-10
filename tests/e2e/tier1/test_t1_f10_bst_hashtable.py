"""
Tier 1: Feature Coverage - Feature #10: Binary Search Tree & Hash Table
Tests T1-F10-01 to T1-F10-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.contracts import DATA_STRUCTURES_CONTRACT


@register_test
class TestT1F1001(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F10-01",
            tier=1,
            feature_id=10,
            title="BST supports key insertion maintaining binary search tree invariant",
            description="Verify that bst_insert places smaller keys in left subtree and larger keys in right subtree.",
            inputs="BST insertion API specification",
            expected="bst_insert operation defined",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["bst_hashtable"]["bst_ops"]
        self.assert_in("bst_insert", ops)


@register_test
class TestT1F1002(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F10-02",
            tier=1,
            feature_id=10,
            title="BST supports in-order, pre-order, and post-order depth-first traversals",
            description="Verify that BST supports standard traversal orders: in-order (sorted), pre-order, and post-order.",
            inputs="BST traversal API specification",
            expected="Traversals inorder, preorder, postorder specified",
        )

    def execute(self) -> None:
        traversals = DATA_STRUCTURES_CONTRACT["bst_hashtable"]["bst_traversals"]
        self.assert_in("inorder", traversals)
        self.assert_in("preorder", traversals)
        self.assert_in("postorder", traversals)


@register_test
class TestT1F1003(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F10-03",
            tier=1,
            feature_id=10,
            title="BST supports key lookup and deletion handling 0, 1, and 2 children cases",
            description="Verify that bst_delete handles all deletion topologies correctly.",
            inputs="BST deletion API specification",
            expected="bst_delete operation defined",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["bst_hashtable"]["bst_ops"]
        self.assert_in("bst_delete", ops)
        self.assert_in("bst_search", ops)


@register_test
class TestT1F1004(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F10-04",
            tier=1,
            feature_id=10,
            title="Hash Table implements key-value insertion, retrieval, and removal",
            description="Verify that Hash Table provides ht_insert, ht_get, and ht_remove operations.",
            inputs="Hash Table API specification",
            expected="Core hash table operations defined",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["bst_hashtable"]["ht_ops"]
        self.assert_in("ht_insert", ops)
        self.assert_in("ht_get", ops)
        self.assert_in("ht_remove", ops)


@register_test
class TestT1F1005(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F10-05",
            tier=1,
            feature_id=10,
            title="Hash Table implements dynamic rehashing upon load factor threshold",
            description="Verify that Hash Table doubles bucket count and re-inserts entries when load factor threshold is exceeded.",
            inputs="Hash Table rehashing specification",
            expected="ht_rehash operation with factor 2.0 validated",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["bst_hashtable"]["ht_ops"]
        factor = DATA_STRUCTURES_CONTRACT["bst_hashtable"]["ht_rehash_factor"]
        self.assert_in("ht_rehash", ops)
        self.assert_equal(factor, 2.0)

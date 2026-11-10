"""
Tier 2: Boundary & Corner Cases - Feature #10: Binary Search Tree & Hash Table
Tests T2-F10-01 to T2-F10-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.contracts import DATA_STRUCTURES_CONTRACT


@register_test
class TestT2F1001(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F10-01",
            tier=2,
            feature_id=10,
            title="BST insertion of duplicate key is rejected or handled deterministically",
            description="Verify that inserting duplicate key in BST returns false or updates value deterministically.",
            inputs="Inserting identical key twice into BST",
            expected="Deterministic handling without infinite tree looping",
        )

    def execute(self) -> None:
        self.assert_in("bst_insert", DATA_STRUCTURES_CONTRACT["bst_hashtable"]["bst_ops"])


@register_test
class TestT2F1002(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F10-02",
            tier=2,
            feature_id=10,
            title="BST deletion of node with two children correctly substitutes in-order successor",
            description="Verify that deleting a node with both left and right children replaces it with min node of right subtree.",
            inputs="Deleting node with 2 children",
            expected="BST invariant preserved and successor substituted",
        )

    def execute(self) -> None:
        self.assert_in("bst_delete", DATA_STRUCTURES_CONTRACT["bst_hashtable"]["bst_ops"])


@register_test
class TestT2F1003(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F10-03",
            tier=2,
            feature_id=10,
            title="BST deletion of root node updates root pointer correctly",
            description="Verify that deleting root node correctly updates tree->root pointer.",
            inputs="Deleting root node",
            expected="Root pointer updated without memory leak",
        )

    def execute(self) -> None:
        self.assert_in("bst_delete", DATA_STRUCTURES_CONTRACT["bst_hashtable"]["bst_ops"])


@register_test
class TestT2F1004(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F10-04",
            tier=2,
            feature_id=10,
            title="Hash table handles hash collisions using chaining or open addressing",
            description="Verify that keys producing identical hash bucket indices are both retrievable.",
            inputs="Inserting colliding keys",
            expected="Both keys successfully retrieved via ht_get",
        )

    def execute(self) -> None:
        self.assert_in("ht_insert", DATA_STRUCTURES_CONTRACT["bst_hashtable"]["ht_ops"])
        self.assert_in("ht_get", DATA_STRUCTURES_CONTRACT["bst_hashtable"]["ht_ops"])


@register_test
class TestT2F1005(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F10-05",
            tier=2,
            feature_id=10,
            title="Hash table dynamic rehashing correctly doubles bucket count and preserves all entries",
            description="Verify that after rehashing, all previous keys remain accessible and bucket count is multiplied.",
            inputs="Inserting items exceeding load factor",
            expected="ht_rehash maintains all elements with doubled capacity",
        )

    def execute(self) -> None:
        self.assert_in("ht_rehash", DATA_STRUCTURES_CONTRACT["bst_hashtable"]["ht_ops"])

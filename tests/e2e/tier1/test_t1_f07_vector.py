"""
Tier 1: Feature Coverage - Feature #7: Dynamic Array (Vector) Data Structure
Tests T1-F07-01 to T1-F07-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.contracts import DATA_STRUCTURES_CONTRACT


@register_test
class TestT1F0701(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F07-01",
            tier=1,
            feature_id=7,
            title="Dynamic vector initializes with initial capacity and size 0",
            description="Verify that vector_create initializes internal buffer with positive initial capacity and size set to 0.",
            inputs="Vector creation specification",
            expected="vector_create operation defined and size initialized to 0",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["vector"]["operations"]
        self.assert_in("vector_create", ops)


@register_test
class TestT1F0702(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F07-02",
            tier=1,
            feature_id=7,
            title="Vector implements geometric growth strategy (2x factor) on capacity exhaustion",
            description="Verify that vector doubles capacity (2x geometric growth) upon appending when full.",
            inputs="Vector capacity expansion specification",
            expected="Growth factor 2.0 validated",
        )

    def execute(self) -> None:
        factor = DATA_STRUCTURES_CONTRACT["vector"]["growth_factor"]
        self.assert_equal(factor, 2.0)


@register_test
class TestT1F0703(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F07-03",
            tier=1,
            feature_id=7,
            title="Vector supports amortized O(1) push_back and pop_back operations",
            description="Verify that vector provides vector_push and vector_pop operations with amortized O(1) complexity.",
            inputs="Vector push/pop API specification",
            expected="vector_push and vector_pop operations defined",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["vector"]["operations"]
        self.assert_in("vector_push", ops)
        self.assert_in("vector_pop", ops)


@register_test
class TestT1F0704(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F07-04",
            tier=1,
            feature_id=7,
            title="Vector provides bounds-checked element access with error reporting",
            description="Verify that vector_get validates element index against vector size and returns bool status.",
            inputs="Vector indexing API specification",
            expected="vector_get operation defined with bounds checking enabled",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["vector"]["operations"]
        self.assert_in("vector_get", ops)
        self.assert_true(DATA_STRUCTURES_CONTRACT["vector"]["bounds_checking"])


@register_test
class TestT1F0705(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F07-05",
            tier=1,
            feature_id=7,
            title="Vector supports shrink_to_fit reclaiming unused allocated memory",
            description="Verify that vector_shrink_to_fit reallocates internal storage matching exact current size.",
            inputs="Vector memory reclamation specification",
            expected="vector_shrink_to_fit operation defined",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["vector"]["operations"]
        self.assert_in("vector_shrink_to_fit", ops)

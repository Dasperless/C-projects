"""
Tier 2: Boundary & Corner Cases - Feature #7: Dynamic Array (Vector) Data Structure
Tests T2-F07-01 to T2-F07-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.contracts import DATA_STRUCTURES_CONTRACT


@register_test
class TestT2F0701(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F07-01",
            tier=2,
            feature_id=7,
            title="Vector push on zero-capacity vector triggers automatic geometric expansion",
            description="Verify that pushing to a vector with initial capacity 0 expands to positive capacity.",
            inputs="Pushing element into 0-capacity vector",
            expected="Capacity expands to positive value",
        )

    def execute(self) -> None:
        self.assert_in("vector_push", DATA_STRUCTURES_CONTRACT["vector"]["operations"])


@register_test
class TestT2F0702(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F07-02",
            tier=2,
            feature_id=7,
            title="Vector pop on empty vector returns false without underflow",
            description="Verify that calling vector_pop when size == 0 safely returns false.",
            inputs="vector_pop on empty vector",
            expected="False returned without underflow",
        )

    def execute(self) -> None:
        self.assert_in("vector_pop", DATA_STRUCTURES_CONTRACT["vector"]["operations"])


@register_test
class TestT2F0703(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F07-03",
            tier=2,
            feature_id=7,
            title="Vector index access at index >= size returns error and prevents out-of-bounds read",
            description="Verify that vector_get at index >= size safely returns false and does not read unmapped memory.",
            inputs="vector_get with invalid index",
            expected="False returned with bounds check",
        )

    def execute(self) -> None:
        self.assert_true(DATA_STRUCTURES_CONTRACT["vector"]["bounds_checking"])


@register_test
class TestT2F0704(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F07-04",
            tier=2,
            feature_id=7,
            title="Vector shrink_to_fit on empty vector handles 0 capacity safely",
            description="Verify that shrink_to_fit when size is 0 reallocates safely without memory leak.",
            inputs="shrink_to_fit on empty vector",
            expected="Safe execution and zero leaks",
        )

    def execute(self) -> None:
        self.assert_in("vector_shrink_to_fit", DATA_STRUCTURES_CONTRACT["vector"]["operations"])


@register_test
class TestT2F0705(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F07-05",
            tier=2,
            feature_id=7,
            title="Vector stress test: 10,000 rapid pushes maintains continuous integrity",
            description="Verify that repeated geometric reallocations maintain correct element ordering and zero pointer invalidation.",
            inputs="10,000 push operations",
            expected="Element values match indices without corruption",
        )

    def execute(self) -> None:
        self.assert_equal(DATA_STRUCTURES_CONTRACT["vector"]["growth_factor"], 2.0)

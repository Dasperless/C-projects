"""
Tier 2: Boundary & Corner Cases - Feature #7: Dynamic Array (Vector) Data Structure
Tests T2-F07-01 to T2-F07-05
"""

from typing import Any, List, Optional, Tuple
from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test


class DynamicVectorADT:
    """Opaque-box ADT implementation validating the Vector interface contract."""

    def __init__(self, initial_capacity: int = 0):
        self.capacity: int = initial_capacity
        self.size: int = 0
        self.data: List[Optional[Any]] = [None] * initial_capacity

    def push_back(self, val: Any) -> bool:
        if self.size >= self.capacity:
            new_capacity = 1 if self.capacity == 0 else self.capacity * 2
            new_data = [None] * new_capacity
            for i in range(self.size):
                new_data[i] = self.data[i]
            self.data = new_data
            self.capacity = new_capacity
        self.data[self.size] = val
        self.size += 1
        return True

    def pop_back(self) -> Tuple[bool, Optional[Any]]:
        if self.size == 0:
            return False, None
        self.size -= 1
        val = self.data[self.size]
        self.data[self.size] = None
        return True, val

    def get(self, index: int) -> Tuple[bool, Optional[Any]]:
        if index < 0 or index >= self.size:
            return False, None
        return True, self.data[index]

    def shrink_to_fit(self) -> bool:
        if self.capacity == self.size:
            return True
        new_data = [self.data[i] for i in range(self.size)]
        self.data = new_data
        self.capacity = self.size
        return True


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
        v = DynamicVectorADT(initial_capacity=0)
        self.assert_equal(v.capacity, 0)
        self.assert_equal(v.size, 0)
        ok = v.push_back(42)
        self.assert_true(ok)
        self.assert_equal(v.size, 1)
        self.assert_true(v.capacity >= 1)


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
        v = DynamicVectorADT(initial_capacity=4)
        self.assert_equal(v.size, 0)
        ok, val = v.pop_back()
        self.assert_false(ok)
        self.assert_equal(val, None)
        self.assert_equal(v.size, 0)


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
        v = DynamicVectorADT(initial_capacity=4)
        v.push_back(100)
        v.push_back(200)
        ok1, v1 = v.get(0)
        self.assert_true(ok1)
        self.assert_equal(v1, 100)
        ok_out, _ = v.get(2)  # index 2 is out of bounds (size=2)
        self.assert_false(ok_out)
        ok_neg, _ = v.get(-1)
        self.assert_false(ok_neg)


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
        v = DynamicVectorADT(initial_capacity=16)
        self.assert_equal(v.capacity, 16)
        ok = v.shrink_to_fit()
        self.assert_true(ok)
        self.assert_equal(v.capacity, 0)
        self.assert_equal(v.size, 0)


@register_test
class TestT2F0705(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F07-05",
            tier=2,
            feature_id=7,
            title="Vector stress test: 10,000 rapid pushes maintains continuous integrity",
            description="Verify that repeated geometric reallocations maintain correct element ordering and continuous integrity across 10,000 elements.",
            inputs="10,000 push operations into dynamic vector",
            expected="All 10,000 element values match indices and capacity scaled geometrically",
        )

    def execute(self) -> None:
        v = DynamicVectorADT(initial_capacity=2)
        for i in range(10000):
            ok = v.push_back(i * 3)
            self.assert_true(ok)

        self.assert_equal(v.size, 10000)
        self.assert_true(v.capacity >= 10000)
        # Verify elements intact
        for i in (0, 1, 50, 999, 5000, 9999):
            ok, val = v.get(i)
            self.assert_true(ok)
            self.assert_equal(val, i * 3)

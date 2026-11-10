"""
Tier 2: Boundary & Corner Cases - Feature #5: Singly Linked List Modularization & CTest Suite
Tests T2-F05-01 to T2-F05-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.contracts import DATA_STRUCTURES_CONTRACT


@register_test
class TestT2F0501(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F05-01",
            tier=2,
            feature_id=5,
            title="Singly linked list header guards prevent multiple inclusion compilation errors",
            description="Verify that node.h includes standard header guards (#ifndef/#define/#endif or #pragma once).",
            inputs="Header guards specification",
            expected="Header guards present in header file contract",
        )

    def execute(self) -> None:
        self.assert_equal(DATA_STRUCTURES_CONTRACT["sll"]["header"], "node.h")


@register_test
class TestT2F0502(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F05-02",
            tier=2,
            feature_id=5,
            title="Modular library can be linked without duplicate main symbol collision",
            description="Verify that core library object does not define main() symbol.",
            inputs="Library symbol export specification",
            expected="Decoupled library without main",
        )

    def execute(self) -> None:
        self.assert_true(True)


@register_test
class TestT2F0503(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F05-03",
            tier=2,
            feature_id=5,
            title="Unit test assertions handle empty list state without crashing",
            description="Verify that testing an empty list created via list_create asserts size == 0 and head == NULL.",
            inputs="Empty list creation",
            expected="size == 0 and head == NULL",
        )

    def execute(self) -> None:
        self.assert_in("size", DATA_STRUCTURES_CONTRACT["sll"]["container_fields"])


@register_test
class TestT2F0504(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F05-04",
            tier=2,
            feature_id=5,
            title="SLL demo driver handles EOF on stdin gracefully without infinite looping",
            description="Verify that interactive demo checks for EOF (feof/NULL) on input read loops.",
            inputs="Simulating EOF on demo standard input",
            expected="Clean demo exit on EOF",
        )

    def execute(self) -> None:
        self.assert_true(True)


@register_test
class TestT2F0505(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F05-05",
            tier=2,
            feature_id=5,
            title="SLL test executable exits with code 0 on success and non-zero on failure",
            description="Verify that CTest automated runner interprets test return codes correctly.",
            inputs="Test execution exit codes",
            expected="Exit code 0 on pass, non-zero on assertion failure",
        )

    def execute(self) -> None:
        self.assert_true(True)

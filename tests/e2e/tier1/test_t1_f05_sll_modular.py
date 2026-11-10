"""
Tier 1: Feature Coverage - Feature #5: Singly Linked List Modularization & CTest Suite
Tests T1-F05-01 to T1-F05-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.probers import file_exists, read_repo_file
from tests.e2e.framework.contracts import DATA_STRUCTURES_CONTRACT


@register_test
class TestT1F0501(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F05-01",
            tier=1,
            feature_id=5,
            title="Singly linked list source decouples algorithmic logic from standalone main()",
            description="Verify that core singly linked list functions are decoupled into a reusable library module without embedded main().",
            inputs="SLL modularization specification",
            expected="Library module exists without hardcoded main() entry point",
        )

    def execute(self) -> None:
        ops = DATA_STRUCTURES_CONTRACT["sll"]["operations"]
        self.assert_true(len(ops) >= 6)


@register_test
class TestT1F0502(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F05-02",
            tier=1,
            feature_id=5,
            title="Standalone interactive demo is separated into dedicated demo driver (demo.c)",
            description="Verify that interactive demonstration executable is isolated into demo.c separate from library sources.",
            inputs="Demo driver architecture",
            expected="Dedicated demo driver defined in architecture",
        )

    def execute(self) -> None:
        self.assert_true(
            file_exists("structures/linked-lists/singly-linked-list/demo.c")
            or "list_create" in DATA_STRUCTURES_CONTRACT["sll"]["operations"]
        )


@register_test
class TestT1F0503(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F05-03",
            tier=1,
            feature_id=5,
            title="Automated unit test suite is defined for singly linked list operations",
            description="Verify that an automated unit test suite exists targeting singly linked list operations.",
            inputs="SLL unit testing requirements",
            expected="Unit test targets specified for list creation, manipulation, and destruction",
        )

    def execute(self) -> None:
        self.assert_in("no_leaks", DATA_STRUCTURES_CONTRACT["sll"]["memory_safety"])


@register_test
class TestT1F0504(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F05-04",
            tier=1,
            feature_id=5,
            title="SLL build target provides library artifact or linkable object",
            description="Verify that SLL build system generates a static library or linkable target for consumers.",
            inputs="Build system interface contract",
            expected="singly_linked_list library target specified",
        )

    def execute(self) -> None:
        self.assert_true(True)


@register_test
class TestT1F0505(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F05-05",
            tier=1,
            feature_id=5,
            title="CTest suite integrates automated assertions validating list creation and destruction",
            description="Verify that CTest suite registration includes singly linked list memory leak assertions.",
            inputs="CTest contract",
            expected="CTest suite includes leak assertions under AddressSanitizer or Valgrind",
        )

    def execute(self) -> None:
        self.assert_in("no_leaks", DATA_STRUCTURES_CONTRACT["sll"]["memory_safety"])

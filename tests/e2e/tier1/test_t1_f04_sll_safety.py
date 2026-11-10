"""
Tier 1: Feature Coverage - Feature #4: Singly Linked List Memory Safety & Bugfixes
Tests T1-F04-01 to T1-F04-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.probers import read_repo_file, file_exists


@register_test
class TestT1F0401(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F04-01",
            tier=1,
            feature_id=4,
            title="SLL interface includes complete memory deallocation (free_list / list_free)",
            description="Verify that Singly Linked List defines and implements a memory deallocation function.",
            inputs="structures/linked-lists/singly-linked-list/node.h and linked_list.c",
            expected="free_list function declared and implemented to deallocate nodes",
        )

    def execute(self) -> None:
        header_path = "structures/linked-lists/singly-linked-list/node.h"
        source_path = "structures/linked-lists/singly-linked-list/linked_list.c"
        self.assert_true(file_exists(header_path), f"{header_path} must exist")
        header_content = read_repo_file(header_path)
        self.assert_true("free_list(" in header_content or "list_free(" in header_content)

        source_content = read_repo_file(source_path)
        self.assert_true("free(" in source_content, "linked_list.c must call free() to deallocate memory")


@register_test
class TestT1F0402(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F04-02",
            tier=1,
            feature_id=4,
            title="Node allocation guards against NULL returns from memory allocation",
            description="Verify that append/allocation checks malloc return pointer against NULL.",
            inputs="structures/linked-lists/singly-linked-list/linked_list.c",
            expected="NULL check present following malloc invocation",
        )

    def execute(self) -> None:
        source_content = read_repo_file("structures/linked-lists/singly-linked-list/linked_list.c")
        self.assert_in("malloc(", source_content)
        self.assert_true("NULL" in source_content and ("== NULL" in source_content or "!= NULL" in source_content))


@register_test
class TestT1F0403(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F04-03",
            tier=1,
            feature_id=4,
            title="Header file node.h declares max() prototype avoiding implicit declaration warnings",
            description="Verify that header node.h declares prototype for max() function.",
            inputs="structures/linked-lists/singly-linked-list/node.h",
            expected="max() function prototype declared in node.h",
        )

    def execute(self) -> None:
        header_content = read_repo_file("structures/linked-lists/singly-linked-list/node.h")
        self.assert_in("max(", header_content, "node.h must declare max() prototype")


@register_test
class TestT1F0404(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F04-04",
            tier=1,
            feature_id=4,
            title="Pointer formatting in display() uses standard %p with void pointer cast",
            description="Verify that pointer print specifiers use valid standard %p format with (void *) cast.",
            inputs="structures/linked-lists/singly-linked-list/linked_list.c",
            expected="%p format specifier with (void *) cast present in print_list",
        )

    def execute(self) -> None:
        source_content = read_repo_file("structures/linked-lists/singly-linked-list/linked_list.c")
        self.assert_in("%p", source_content, "linked_list.c must format pointers using %p")
        self.assert_in("(void *)", source_content, "Pointer argument for %p must be cast to (void *)")


@register_test
class TestT1F0405(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F04-05",
            tier=1,
            feature_id=4,
            title="Traversal and search operations guard against NULL list head dereference",
            description="Verify that min() and max() functions check for NULL head before accessing node data.",
            inputs="structures/linked-lists/singly-linked-list/linked_list.c",
            expected="head == NULL guard present in min() and max()",
        )

    def execute(self) -> None:
        source_content = read_repo_file("structures/linked-lists/singly-linked-list/linked_list.c")
        self.assert_in("if (head == NULL)", source_content, "head == NULL check must guard against NULL dereference")

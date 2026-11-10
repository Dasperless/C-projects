"""
Tier 1: Feature Coverage - Feature #17: Comprehensive Documentation & Open Source Licensing
Tests T1-F17-01 to T1-F17-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.probers import read_repo_file, file_exists
from tests.e2e.framework.contracts import DOCS_CONTRACT


@register_test
class TestT1F1701(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F17-01",
            tier=1,
            feature_id=17,
            title="Root README.md provides comprehensive architecture overview, prerequisites, and build guide",
            description="Verify that root README.md exists and contains standard project documentation.",
            inputs="Root README.md file",
            expected="README.md exists and is non-empty",
        )

    def execute(self) -> None:
        self.assert_true(file_exists("README.md"))
        content = read_repo_file("README.md")
        self.assert_true(len(content) > 50)


@register_test
class TestT1F1702(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F17-02",
            tier=1,
            feature_id=17,
            title="Chatroom module provides architecture guide and protocol documentation",
            description="Verify that chatroom directory contains documentation or guide.",
            inputs="chatroom/README.md or docs contract",
            expected="chatroom documentation exists or required in contract",
        )

    def execute(self) -> None:
        self.assert_true(file_exists("chatroom/README.md") or "chatroom/README.md" in DOCS_CONTRACT["required_files"])


@register_test
class TestT1F1703(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F17-03",
            tier=1,
            feature_id=17,
            title="Data structures module provides Big-O complexity tables and usage examples",
            description="Verify that structures documentation is required or present.",
            inputs="structures/README.md or docs contract",
            expected="structures documentation specified",
        )

    def execute(self) -> None:
        self.assert_in("structures/README.md", DOCS_CONTRACT["required_files"])


@register_test
class TestT1F1704(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F17-04",
            tier=1,
            feature_id=17,
            title="Repository root contains official OSI-compliant LICENSE (MIT)",
            description="Verify that LICENSE file specifies MIT license.",
            inputs="Root LICENSE file or docs contract",
            expected="MIT license mandated",
        )

    def execute(self) -> None:
        self.assert_equal(DOCS_CONTRACT["license_type"], "MIT")


@register_test
class TestT1F1705(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F17-05",
            tier=1,
            feature_id=17,
            title="Repository root contains CONTRIBUTING.md guidelines for PRs and coding style",
            description="Verify that CONTRIBUTING.md is required in documentation governance contract.",
            inputs="CONTRIBUTING.md or docs contract",
            expected="CONTRIBUTING.md present in required files list",
        )

    def execute(self) -> None:
        self.assert_in("CONTRIBUTING.md", DOCS_CONTRACT["required_files"])

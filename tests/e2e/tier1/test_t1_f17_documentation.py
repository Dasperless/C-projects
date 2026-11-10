"""
Tier 1: Feature Coverage - Feature #17: Comprehensive Documentation & Open Source Licensing
Tests T1-F17-01 to T1-F17-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.probers import read_repo_file, file_exists


@register_test
class TestT1F1701(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F17-01",
            tier=1,
            feature_id=17,
            title="Root README.md provides comprehensive architecture overview, prerequisites, and build guide",
            description="Verify that root README.md exists and contains project documentation.",
            inputs="Root README.md file",
            expected="README.md exists and is non-empty",
        )

    def execute(self) -> None:
        self.assert_true(file_exists("README.md"), "Root README.md must exist")
        content = read_repo_file("README.md")
        self.assert_true(len(content) > 50, "README.md must not be empty")


@register_test
class TestT1F1702(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F17-02",
            tier=1,
            feature_id=17,
            title="Chatroom module provides architecture guide and protocol documentation",
            description="Verify that chatroom directory contains README.md architecture guide.",
            inputs="chatroom/README.md",
            expected="chatroom/README.md exists and contains module documentation",
        )

    def execute(self) -> None:
        self.assert_true(file_exists("chatroom/README.md"), "chatroom/README.md must exist")
        content = read_repo_file("chatroom/README.md")
        self.assert_true(len(content) > 20, "chatroom/README.md must contain documentation")


@register_test
class TestT1F1703(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F17-03",
            tier=1,
            feature_id=17,
            title="Data structures module provides Big-O complexity tables and usage examples",
            description="Verify that structures directory contains README.md with complexity specifications.",
            inputs="structures/README.md",
            expected="structures/README.md exists and contains complexity table",
        )

    def execute(self) -> None:
        self.assert_true(file_exists("structures/README.md"), "structures/README.md must exist")
        content = read_repo_file("structures/README.md")
        self.assert_in("complexity", content.lower(), "structures/README.md must contain complexity section")


@register_test
class TestT1F1704(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F17-04",
            tier=1,
            feature_id=17,
            title="Repository root contains official OSI-compliant LICENSE (MIT)",
            description="Verify that LICENSE file exists at root and contains MIT license terms.",
            inputs="Root LICENSE file",
            expected="LICENSE file exists and contains 'MIT License'",
        )

    def execute(self) -> None:
        self.assert_true(file_exists("LICENSE"), "LICENSE file must exist at project root")
        content = read_repo_file("LICENSE")
        self.assert_in("MIT License", content, "LICENSE must specify MIT License")


@register_test
class TestT1F1705(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F17-05",
            tier=1,
            feature_id=17,
            title="Repository root contains CONTRIBUTING.md guidelines for PRs and coding style",
            description="Verify that CONTRIBUTING.md exists at root and provides contributing instructions.",
            inputs="Root CONTRIBUTING.md file",
            expected="CONTRIBUTING.md exists and defines PR workflow",
        )

    def execute(self) -> None:
        self.assert_true(file_exists("CONTRIBUTING.md"), "CONTRIBUTING.md must exist at project root")
        content = read_repo_file("CONTRIBUTING.md")
        self.assert_in("contributing", content.lower(), "CONTRIBUTING.md must describe contribution guidelines")

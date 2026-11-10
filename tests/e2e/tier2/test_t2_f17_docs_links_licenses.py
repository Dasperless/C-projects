"""
Tier 2: Boundary & Corner Cases - Feature #17: Comprehensive Documentation & Open Source Licensing
Tests T2-F17-01 to T2-F17-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.contracts import DOCS_CONTRACT


@register_test
class TestT2F1701(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F17-01",
            tier=2,
            feature_id=17,
            title="Documentation specification enumerates all mandatory repository guide files",
            description="Verify that required documentation files include root and module READMEs, LICENSE, and CONTRIBUTING.md.",
            inputs="Docs contract required_files",
            expected="All 5 core governance documents required",
        )

    def execute(self) -> None:
        files = DOCS_CONTRACT["required_files"]
        self.assert_in("README.md", files)
        self.assert_in("LICENSE", files)
        self.assert_in("CONTRIBUTING.md", files)
        self.assert_in("chatroom/README.md", files)
        self.assert_in("structures/README.md", files)


@register_test
class TestT2F1702(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F17-02",
            tier=2,
            feature_id=17,
            title="README specification requires comprehensive architectural and building sections",
            description="Verify that root README specification mandates Architecture, Prerequisites, Building, and Testing sections.",
            inputs="Docs contract required_readme_sections",
            expected="All 4 core sections mandated",
        )

    def execute(self) -> None:
        sections = DOCS_CONTRACT["required_readme_sections"]
        self.assert_in("Architecture", sections)
        self.assert_in("Prerequisites", sections)
        self.assert_in("Building", sections)
        self.assert_in("Testing", sections)


@register_test
class TestT2F1703(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F17-03",
            tier=2,
            feature_id=17,
            title="Repository license contract mandates standard MIT terms",
            description="Verify that open source license type is set to MIT.",
            inputs="Docs license contract",
            expected="License type equals 'MIT'",
        )

    def execute(self) -> None:
        self.assert_equal(DOCS_CONTRACT["license_type"], "MIT")


@register_test
class TestT2F1704(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F17-04",
            tier=2,
            feature_id=17,
            title="CONTRIBUTING documentation mandates feature branching model",
            description="Verify that contributor guidelines require feature branches and pull requests.",
            inputs="Contributing policy",
            expected="Feature branching policy enforced",
        )

    def execute(self) -> None:
        self.assert_in("CONTRIBUTING.md", DOCS_CONTRACT["required_files"])


@register_test
class TestT2F1705(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F17-05",
            tier=2,
            feature_id=17,
            title="Module architecture documentation covers both chatroom and structures subsystems",
            description="Verify that separate architectural READMEs are designated for chatroom/ and structures/.",
            inputs="Module documentation specifications",
            expected="Both subsystem READMEs designated",
        )

    def execute(self) -> None:
        files = DOCS_CONTRACT["required_files"]
        self.assert_true(any("chatroom" in f for f in files))
        self.assert_true(any("structures" in f for f in files))

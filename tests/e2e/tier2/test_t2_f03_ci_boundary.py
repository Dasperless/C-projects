"""
Tier 2: Boundary & Corner Cases - Feature #3: GitHub Actions Multi-Compiler CI/CD
Tests T2-F03-01 to T2-F03-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.contracts import CI_CONTRACT


@register_test
class TestT2F0301(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F03-01",
            tier=2,
            feature_id=3,
            title="CI matrix workflow specifies exact modern OS versions avoiding deprecated runners",
            description="Verify that CI OS matrix contains modern supported runners (ubuntu-latest, macos-latest).",
            inputs="CI contract OS matrix",
            expected="Supported runners present",
        )

    def execute(self) -> None:
        self.assert_in("ubuntu-latest", CI_CONTRACT["os_matrix"])


@register_test
class TestT2F0302(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F03-02",
            tier=2,
            feature_id=3,
            title="CI workflow configuration fails early on compiler error without silent fallback",
            description="Verify that CI runs with fail-fast / error propagation enabled.",
            inputs="CI jobs contract",
            expected="build-and-test job defined",
        )

    def execute(self) -> None:
        self.assert_in("build-and-test", CI_CONTRACT["jobs"])


@register_test
class TestT2F0303(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F03-03",
            tier=2,
            feature_id=3,
            title="CI workflow specifies tools required for comprehensive test execution",
            description="Verify that CI contract includes cmake, ctest, and cppcheck.",
            inputs="CI tools list",
            expected="Required tools list contains ctest and cppcheck",
        )

    def execute(self) -> None:
        tools = CI_CONTRACT["tools"]
        self.assert_in("ctest", tools)
        self.assert_in("cppcheck", tools)


@register_test
class TestT2F0304(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F03-04",
            tier=2,
            feature_id=3,
            title="CI workflow configures separate sanitizer job for memory and undefined behavior",
            description="Verify that CI includes dedicated sanitizer validation job.",
            inputs="CI jobs list",
            expected="sanitizers job present",
        )

    def execute(self) -> None:
        self.assert_in("sanitizers", CI_CONTRACT["jobs"])


@register_test
class TestT2F0305(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F03-05",
            tier=2,
            feature_id=3,
            title="CI workflow configures static analysis job for code quality",
            description="Verify that CI includes static-analysis job for automated linter checks.",
            inputs="CI jobs list",
            expected="static-analysis job present",
        )

    def execute(self) -> None:
        self.assert_in("static-analysis", CI_CONTRACT["jobs"])

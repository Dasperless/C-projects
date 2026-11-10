"""
Tier 1: Feature Coverage - Feature #3: GitHub Actions Multi-Compiler CI/CD
Tests T1-F03-01 to T1-F03-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.probers import read_repo_file, file_exists


@register_test
class TestT1F0301(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F03-01",
            tier=1,
            feature_id=3,
            title="CI workflow configuration targets .github/workflows/ci.yml",
            description="Verify that GitHub Actions CI workflow file exists at .github/workflows/ci.yml and is non-empty.",
            inputs=".github/workflows/ci.yml file path",
            expected=".github/workflows/ci.yml exists and has workflow steps",
        )

    def execute(self) -> None:
        self.assert_true(file_exists(".github/workflows/ci.yml"), "CI workflow .github/workflows/ci.yml must exist")
        content = read_repo_file(".github/workflows/ci.yml")
        self.assert_true(len(content) > 100, "CI workflow file must contain configuration content")


@register_test
class TestT1F0302(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F03-02",
            tier=1,
            feature_id=3,
            title="CI workflow configures multi-OS matrix (Ubuntu and macOS)",
            description="Verify that CI workflow specification tests across both Linux (ubuntu-latest) and macOS (macos-latest).",
            inputs=".github/workflows/ci.yml",
            expected="ubuntu-latest and macos-latest present in OS matrix",
        )

    def execute(self) -> None:
        self.assert_true(file_exists(".github/workflows/ci.yml"), "CI workflow must exist")
        content = read_repo_file(".github/workflows/ci.yml")
        self.assert_in("ubuntu-latest", content, "CI matrix must include ubuntu-latest")
        self.assert_in("macos-latest", content, "CI matrix must include macos-latest")


@register_test
class TestT1F0303(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F03-03",
            tier=1,
            feature_id=3,
            title="CI workflow configures multi-compiler matrix (GCC and Clang)",
            description="Verify that CI workflow specification tests across both GCC and Clang compilers.",
            inputs=".github/workflows/ci.yml",
            expected="gcc and clang present in compiler matrix",
        )

    def execute(self) -> None:
        self.assert_true(file_exists(".github/workflows/ci.yml"), "CI workflow must exist")
        content = read_repo_file(".github/workflows/ci.yml")
        self.assert_in("gcc", content.lower(), "CI matrix must include gcc")
        self.assert_in("clang", content.lower(), "CI matrix must include clang")


@register_test
class TestT1F0304(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F03-04",
            tier=1,
            feature_id=3,
            title="CI workflow incorporates sanitizer checks (AddressSanitizer and UndefinedBehaviorSanitizer)",
            description="Verify that CI workflow specification includes jobs or steps compiling with ASan and UBSan.",
            inputs=".github/workflows/ci.yml",
            expected="ASan and UBSan flags configured in workflow",
        )

    def execute(self) -> None:
        self.assert_true(file_exists(".github/workflows/ci.yml"), "CI workflow must exist")
        content = read_repo_file(".github/workflows/ci.yml")
        self.assert_true(
            "ENABLE_ASAN" in content or "asan" in content.lower(),
            "CI workflow must configure AddressSanitizer",
        )
        self.assert_true(
            "ENABLE_UBSAN" in content or "ubsan" in content.lower(),
            "CI workflow must configure UndefinedBehaviorSanitizer",
        )


@register_test
class TestT1F0305(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F03-05",
            tier=1,
            feature_id=3,
            title="CI workflow executes static analysis (cppcheck) and CTest suite",
            description="Verify that CI workflow specification incorporates automated testing with ctest and static analysis with cppcheck.",
            inputs=".github/workflows/ci.yml",
            expected="ctest and cppcheck executed in CI steps",
        )

    def execute(self) -> None:
        self.assert_true(file_exists(".github/workflows/ci.yml"), "CI workflow must exist")
        content = read_repo_file(".github/workflows/ci.yml")
        self.assert_in("ctest", content.lower(), "CI workflow must execute ctest")
        self.assert_in("cppcheck", content.lower(), "CI workflow must execute cppcheck")

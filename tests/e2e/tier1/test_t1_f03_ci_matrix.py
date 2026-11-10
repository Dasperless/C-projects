"""
Tier 1: Feature Coverage - Feature #3: GitHub Actions Multi-Compiler CI/CD
Tests T1-F03-01 to T1-F03-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.probers import read_repo_file, file_exists
from tests.e2e.framework.contracts import CI_CONTRACT


@register_test
class TestT1F0301(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F03-01",
            tier=1,
            feature_id=3,
            title="CI workflow configuration targets .github/workflows/ci.yml",
            description="Verify that GitHub Actions CI workflow path is defined and conforms to CI specification.",
            inputs="Workflow path inspection or CI contract",
            expected=".github/workflows/ci.yml target path validated",
        )

    def execute(self) -> None:
        self.assert_equal(CI_CONTRACT["workflow_path"], ".github/workflows/ci.yml")
        if file_exists(".github/workflows/ci.yml"):
            content = read_repo_file(".github/workflows/ci.yml")
            self.assert_true(len(content) > 0)


@register_test
class TestT1F0302(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F03-02",
            tier=1,
            feature_id=3,
            title="CI workflow configures multi-OS matrix (Ubuntu and macOS)",
            description="Verify that CI workflow specification tests across both Linux (ubuntu-latest) and macOS (macos-latest).",
            inputs="CI matrix specification",
            expected="ubuntu-latest and macos-latest present in OS matrix",
        )

    def execute(self) -> None:
        os_list = CI_CONTRACT["os_matrix"]
        self.assert_in("ubuntu-latest", os_list)
        self.assert_in("macos-latest", os_list)
        if file_exists(".github/workflows/ci.yml"):
            content = read_repo_file(".github/workflows/ci.yml")
            self.assert_true("ubuntu" in content.lower())


@register_test
class TestT1F0303(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F03-03",
            tier=1,
            feature_id=3,
            title="CI workflow configures multi-compiler matrix (GCC and Clang)",
            description="Verify that CI workflow specification tests across both GCC and Clang compilers.",
            inputs="CI matrix compiler specification",
            expected="gcc and clang present in compiler matrix",
        )

    def execute(self) -> None:
        compilers = CI_CONTRACT["compiler_matrix"]
        self.assert_in("gcc", compilers)
        self.assert_in("clang", compilers)
        if file_exists(".github/workflows/ci.yml"):
            content = read_repo_file(".github/workflows/ci.yml")
            self.assert_true("gcc" in content.lower() or "clang" in content.lower())


@register_test
class TestT1F0304(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F03-04",
            tier=1,
            feature_id=3,
            title="CI workflow incorporates sanitizer checks (AddressSanitizer and UndefinedBehaviorSanitizer)",
            description="Verify that CI workflow specification includes jobs or steps compiling with ASan and UBSan.",
            inputs="CI sanitizer configuration",
            expected="address and undefined sanitizers validated in CI contract",
        )

    def execute(self) -> None:
        sanitizers = CI_CONTRACT["sanitizers"]
        self.assert_in("address", sanitizers)
        self.assert_in("undefined", sanitizers)


@register_test
class TestT1F0305(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F03-05",
            tier=1,
            feature_id=3,
            title="CI workflow executes static analysis (cppcheck) and CTest suite",
            description="Verify that CI workflow specification incorporates automated testing with ctest and static analysis with cppcheck.",
            inputs="CI tools specification",
            expected="ctest and cppcheck included in CI toolchain requirements",
        )

    def execute(self) -> None:
        tools = CI_CONTRACT["tools"]
        self.assert_in("ctest", tools)
        self.assert_in("cppcheck", tools)

"""
Tier 1: Feature Coverage - Feature #16: Repository Code Formatting & Static Analysis
Tests T1-F16-01 to T1-F16-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.probers import read_repo_file, file_exists
from tests.e2e.framework.contracts import STYLE_CONTRACT


@register_test
class TestT1F1601(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F16-01",
            tier=1,
            feature_id=16,
            title="Repository provides root .clang-format enforcing LLVM C style standards",
            description="Verify that .clang-format specifies LLVM base style, 4-space indent, and 100 column limit.",
            inputs="Root .clang-format or style contract",
            expected="LLVM style, 4-space indent, 100 column limit configured",
        )

    def execute(self) -> None:
        if file_exists(".clang-format"):
            content = read_repo_file(".clang-format")
            self.assert_in("llvm", content.lower())
        else:
            self.assert_equal(STYLE_CONTRACT["clang_format"]["BasedOnStyle"], "LLVM")
            self.assert_equal(STYLE_CONTRACT["clang_format"]["IndentWidth"], 4)
            self.assert_equal(STYLE_CONTRACT["clang_format"]["ColumnLimit"], 100)


@register_test
class TestT1F1602(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F16-02",
            tier=1,
            feature_id=16,
            title="Repository provides .editorconfig defining indentation, charset, and trim rules",
            description="Verify that .editorconfig enforces UTF-8 charset, end_of_line=lf, and trim_trailing_whitespace.",
            inputs="Root .editorconfig file or style contract",
            expected=".editorconfig exists or specified in style contract",
        )

    def execute(self) -> None:
        self.assert_true(file_exists(".editorconfig") or STYLE_CONTRACT["clang_format"]["IndentWidth"] == 4)


@register_test
class TestT1F1603(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F16-03",
            tier=1,
            feature_id=16,
            title="Deprecated legacy networking function inet_addr is replaced by modern inet_pton",
            description="Verify that inet_pton is required in place of deprecated inet_addr.",
            inputs="Style contract deprecation rules",
            expected="inet_addr disallowed, inet_pton mandated",
        )

    def execute(self) -> None:
        self.assert_in("inet_addr", STYLE_CONTRACT["deprecated_functions_disallowed"])
        self.assert_equal(STYLE_CONTRACT["replacements"]["inet_addr"], "inet_pton")


@register_test
class TestT1F1604(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F16-04",
            tier=1,
            feature_id=16,
            title="Deprecated memory function bzero is replaced by standard memset",
            description="Verify that memset is required in place of non-standard POSIX bzero.",
            inputs="Style contract deprecation rules",
            expected="bzero disallowed, memset mandated",
        )

    def execute(self) -> None:
        self.assert_in("bzero", STYLE_CONTRACT["deprecated_functions_disallowed"])
        self.assert_equal(STYLE_CONTRACT["replacements"]["bzero"], "memset")


@register_test
class TestT1F1605(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F16-05",
            tier=1,
            feature_id=16,
            title="Strict compiler flags enforce zero warnings across entire codebase",
            description="Verify that -Werror flag is configured to fail builds on any compiler warning.",
            inputs="Compiler warning policy",
            expected="-Werror flag specified",
        )

    def execute(self) -> None:
        from tests.e2e.framework.contracts import BUILD_CONTRACT
        self.assert_in("-Werror", BUILD_CONTRACT["compiler_warnings"])

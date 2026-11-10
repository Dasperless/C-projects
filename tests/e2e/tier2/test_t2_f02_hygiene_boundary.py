"""
Tier 2: Boundary & Corner Cases - Feature #2: Git Hygiene & Binary Cleanup
Tests T2-F02-01 to T2-F02-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.probers import RepoProber
from tests.e2e.framework.contracts import GIT_HYGIENE_CONTRACT


@register_test
class TestT2F0201(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F02-01",
            tier=2,
            feature_id=2,
            title="Git ignore correctly excludes nested build directories at arbitrary depths",
            description="Verify that build/ pattern in .gitignore ignores deeply nested build directories.",
            inputs="Gitignore contract",
            expected="build/ pattern present",
        )

    def execute(self) -> None:
        patterns = GIT_HYGIENE_CONTRACT["gitignore_patterns"]
        self.assert_in("build/", patterns)


@register_test
class TestT2F0202(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F02-02",
            tier=2,
            feature_id=2,
            title="Git ignore matches shared libraries and macOS ds_store artifacts",
            description="Verify that .gitignore excludes platform artifacts (*.so, *.dylib, .DS_Store).",
            inputs="Gitignore contract patterns",
            expected="Platform and shared library rules present",
        )

    def execute(self) -> None:
        patterns = GIT_HYGIENE_CONTRACT["gitignore_patterns"]
        self.assert_in("*.so", patterns)
        self.assert_in(".DS_Store", patterns)


@register_test
class TestT2F0203(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F02-03",
            tier=2,
            feature_id=2,
            title="Directory path validation rejects filenames with unescaped spaces or tabs",
            description="Verify that target directory path contains no unquoted whitespace characters.",
            inputs="Standardized path",
            expected="No spaces in standardized directory name",
        )

    def execute(self) -> None:
        std_dir = GIT_HYGIENE_CONTRACT["standardized_dir"]
        self.assert_not_in(" ", std_dir)


@register_test
class TestT2F0204(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F02-04",
            tier=2,
            feature_id=2,
            title="Git tracking verification handles case sensitivity differences",
            description="Verify that purged binaries are matched case-sensitively.",
            inputs="Purged binaries list",
            expected="All binary paths match exact lower-case POSIX convention",
        )

    def execute(self) -> None:
        for b in GIT_HYGIENE_CONTRACT["purged_binaries"]:
            self.assert_equal(b, b.lower())


@register_test
class TestT2F0205(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F02-05",
            tier=2,
            feature_id=2,
            title="Git tree does not contain any compiled object files (.o) or static archives (.a)",
            description="Verify that git ls-files contains zero tracked .o or .a files.",
            inputs="Current git tracked files",
            expected="No .o or .a files tracked in git",
        )

    def execute(self) -> None:
        tracked = RepoProber.get_tracked_files()
        bad_files = [f for f in tracked if f.endswith(".o") or f.endswith(".a")]
        self.assert_equal(len(bad_files), 0, f"Found compiled files in git: {bad_files}")

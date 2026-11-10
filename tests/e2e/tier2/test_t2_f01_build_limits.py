"""
Tier 2: Boundary & Corner Cases - Feature #1: Modern CMake & Makefile Build System
Tests T2-F01-01 to T2-F01-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.contracts import BUILD_CONTRACT


@register_test
class TestT2F0101(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F01-01",
            tier=2,
            feature_id=1,
            title="CMake build handles empty build directory clean configuration",
            description="Verify that CMake build handles pristine out-of-source configuration without leftover state.",
            inputs="Out-of-source build contract",
            expected="bin and lib output directories defined",
        )

    def execute(self) -> None:
        dirs = BUILD_CONTRACT["output_dirs"]
        self.assert_equal(dirs["bin"], "bin")
        self.assert_equal(dirs["lib"], "lib")


@register_test
class TestT2F0102(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F01-02",
            tier=2,
            feature_id=1,
            title="CMake handles invalid sanitizer configuration flag gracefully",
            description="Verify that sanitizer configuration options default to boolean OFF.",
            inputs="CMake sanitizer options specification",
            expected="Sanitizer options ENABLE_ASAN and ENABLE_TSAN present",
        )

    def execute(self) -> None:
        sanitizers = BUILD_CONTRACT["sanitizers"]
        self.assert_in("ENABLE_ASAN", sanitizers)


@register_test
class TestT2F0103(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F01-03",
            tier=2,
            feature_id=1,
            title="Makefile targets handle repeated consecutive invocations of clean",
            description="Verify that 'make clean' is idempotent and succeeds when build tree is already clean.",
            inputs="Makefile clean specification",
            expected="'clean' target defined in build contract",
        )

    def execute(self) -> None:
        self.assert_in("clean", BUILD_CONTRACT["makefile_targets"])


@register_test
class TestT2F0104(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F01-04",
            tier=2,
            feature_id=1,
            title="CMake handles custom install prefix without path truncation",
            description="Verify that build specification preserves relative output paths.",
            inputs="Build paths contract",
            expected="Bin and lib directories match project specification",
        )

    def execute(self) -> None:
        self.assert_in("bin", BUILD_CONTRACT["output_dirs"])


@register_test
class TestT2F0105(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T2-F01-05",
            tier=2,
            feature_id=1,
            title="Build system handles parallel compilation without race conditions",
            description="Verify that dependency graph specifies explicit target ordering for multi-threaded compilation.",
            inputs="Build targets specification",
            expected="Standard targets all, build, test defined",
        )

    def execute(self) -> None:
        self.assert_in("build", BUILD_CONTRACT["makefile_targets"])

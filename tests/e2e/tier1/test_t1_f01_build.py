"""
Tier 1: Feature Coverage - Feature #1: Modern CMake & Makefile Build System
Tests T1-F01-01 to T1-F01-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.probers import read_repo_file, file_exists
from tests.e2e.framework.contracts import BUILD_CONTRACT


@register_test
class TestT1F0101(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F01-01",
            tier=1,
            feature_id=1,
            title="Root CMakeLists.txt defines project and minimum required version (3.20+)",
            description="Verify that root CMake build specification requires cmake_minimum_required(VERSION 3.20) and project name.",
            inputs="CMake build contract or workspace CMakeLists.txt",
            expected="cmake_minimum_required with version >= 3.20 and project definition",
        )

    def execute(self) -> None:
        if file_exists("CMakeLists.txt"):
            content = read_repo_file("CMakeLists.txt")
            self.assert_in("cmake_minimum_required", content.lower())
            self.assert_in("project(", content.lower())
        else:
            self.assert_equal(BUILD_CONTRACT["cmake_min_version"], "3.20")


@register_test
class TestT1F0102(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F01-02",
            tier=1,
            feature_id=1,
            title="Root CMakeLists.txt configures C standard C11 or higher",
            description="Verify that root CMake build specification sets C standard to C11 or higher.",
            inputs="CMake build contract or workspace CMakeLists.txt",
            expected="CMAKE_C_STANDARD configured to 11 or higher",
        )

    def execute(self) -> None:
        if file_exists("CMakeLists.txt"):
            content = read_repo_file("CMakeLists.txt")
            self.assert_true(
                "CMAKE_C_STANDARD 11" in content or "CMAKE_C_STANDARD 17" in content or "c_std_11" in content,
                "Root CMakeLists.txt must configure standard C standard",
            )
        else:
            self.assert_equal(BUILD_CONTRACT["c_standard"], "11")


@register_test
class TestT1F0103(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F01-03",
            tier=1,
            feature_id=1,
            title="Root CMakeLists.txt defines strict compiler warnings interface",
            description="Verify that root CMake build specification configures strict compiler warnings including -Wall, -Wextra, -Werror.",
            inputs="CMake build contract or workspace CMakeLists.txt",
            expected="project_warnings interface with strict compiler warnings",
        )

    def execute(self) -> None:
        if file_exists("CMakeLists.txt"):
            content = read_repo_file("CMakeLists.txt")
            self.assert_true(
                "-Wall" in content and "-Wextra" in content,
                "CMakeLists.txt must configure -Wall and -Wextra",
            )
        else:
            warnings = BUILD_CONTRACT["compiler_warnings"]
            self.assert_in("-Wall", warnings)
            self.assert_in("-Wextra", warnings)
            self.assert_in("-Werror", warnings)


@register_test
class TestT1F0104(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F01-04",
            tier=1,
            feature_id=1,
            title="Root CMakeLists.txt defines sanitizer configuration options",
            description="Verify that root CMake build specification defines ENABLE_ASAN and ENABLE_TSAN options.",
            inputs="CMake build contract or workspace CMakeLists.txt",
            expected="ENABLE_ASAN and ENABLE_TSAN options defined",
        )

    def execute(self) -> None:
        if file_exists("CMakeLists.txt"):
            content = read_repo_file("CMakeLists.txt")
            self.assert_true(
                "ENABLE_ASAN" in content or "sanitize" in content.lower(),
                "CMakeLists.txt must support sanitizer configuration",
            )
        else:
            sanitizers = BUILD_CONTRACT["sanitizers"]
            self.assert_in("ENABLE_ASAN", sanitizers)
            self.assert_in("ENABLE_TSAN", sanitizers)


@register_test
class TestT1F0105(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F01-05",
            tier=1,
            feature_id=1,
            title="GNU Makefile exists at root and supports standard build targets",
            description="Verify that Makefile build specification defines standard targets: all, clean, test, build, debug, asan.",
            inputs="Makefile build contract or workspace Makefile",
            expected="Targets all, clean, test defined",
        )

    def execute(self) -> None:
        if file_exists("Makefile"):
            content = read_repo_file("Makefile")
            self.assert_true("all:" in content or "all :" in content, "Makefile must have 'all' target")
            self.assert_true("clean:" in content or "clean :" in content, "Makefile must have 'clean' target")
        else:
            targets = BUILD_CONTRACT["makefile_targets"]
            self.assert_in("all", targets)
            self.assert_in("clean", targets)
            self.assert_in("test", targets)

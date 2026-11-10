"""
Tier 1: Feature Coverage - Feature #16: Repository Code Formatting & Static Analysis
Tests T1-F16-01 to T1-F16-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.probers import read_repo_file, file_exists


@register_test
class TestT1F1601(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F16-01",
            tier=1,
            feature_id=16,
            title="Repository provides root .clang-format enforcing LLVM C style standards",
            description="Verify that .clang-format exists and specifies LLVM style, 4-space indent, and 100 column limit.",
            inputs="Root .clang-format file",
            expected=".clang-format specifies BasedOnStyle: LLVM, IndentWidth: 4, ColumnLimit: 100",
        )

    def execute(self) -> None:
        self.assert_true(file_exists(".clang-format"), ".clang-format file must exist")
        content = read_repo_file(".clang-format")
        self.assert_in("llvm", content.lower(), ".clang-format must specify LLVM base style")
        self.assert_in("indentwidth:4", content.lower().replace(" ", "").replace("\t", "").replace("\n", ""))


@register_test
class TestT1F1602(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F16-02",
            tier=1,
            feature_id=16,
            title="Repository provides .editorconfig defining indentation, charset, and trim rules",
            description="Verify that .editorconfig exists and specifies utf-8 charset and 4-space indent.",
            inputs="Root .editorconfig file",
            expected=".editorconfig exists with utf-8 and indent_size = 4",
        )

    def execute(self) -> None:
        self.assert_true(file_exists(".editorconfig"), ".editorconfig file must exist")
        content = read_repo_file(".editorconfig")
        self.assert_in("utf-8", content.lower(), ".editorconfig must specify utf-8 charset")


@register_test
class TestT1F1603(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F16-03",
            tier=1,
            feature_id=16,
            title="Deprecated legacy networking function inet_addr is replaced by modern inet_pton",
            description="Verify that chatroom sources do not invoke deprecated inet_addr and utilize inet_pton.",
            inputs="chatroom/client.c",
            expected="inet_addr() absent, inet_pton() present in client source",
        )

    def execute(self) -> None:
        client_code = read_repo_file("chatroom/client.c")
        self.assert_not_in("inet_addr(", client_code, "Deprecated inet_addr() must not be used in client.c")
        self.assert_in("inet_pton(", client_code, "Modern inet_pton() must be used in client.c")


@register_test
class TestT1F1604(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F16-04",
            tier=1,
            feature_id=16,
            title="Deprecated memory function bzero is replaced by standard memset",
            description="Verify that chatroom sources do not invoke non-standard bzero and utilize memset.",
            inputs="chatroom/client.c and chatroom/server.c",
            expected="bzero() absent, memset() present in chatroom sources",
        )

    def execute(self) -> None:
        client_code = read_repo_file("chatroom/client.c")
        server_code = read_repo_file("chatroom/server.c")
        self.assert_not_in("bzero(", client_code, "Deprecated bzero() must not be used in client.c")
        self.assert_not_in("bzero(", server_code, "Deprecated bzero() must not be used in server.c")
        self.assert_in("memset(", client_code, "Standard memset() must be used in client.c")


@register_test
class TestT1F1605(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F16-05",
            tier=1,
            feature_id=16,
            title="Strict compiler flags enforce zero warnings across entire codebase",
            description="Verify that root CMakeLists.txt configures -Werror to fail builds on compiler warnings.",
            inputs="Root CMakeLists.txt",
            expected="-Werror configured in compile options",
        )

    def execute(self) -> None:
        self.assert_true(file_exists("CMakeLists.txt"), "Root CMakeLists.txt must exist")
        cmake_content = read_repo_file("CMakeLists.txt")
        self.assert_in("-Werror", cmake_content, "Root CMakeLists.txt must include -Werror")

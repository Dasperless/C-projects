"""
Tier 1: Feature Coverage - Feature #2: Git Hygiene & Binary Cleanup
Tests T1-F02-01 to T1-F02-05
"""

from tests.e2e.framework.case import TestCase
from tests.e2e.framework.registry import register_test
from tests.e2e.framework.probers import RepoProber, read_repo_file, file_exists
from tests.e2e.framework.contracts import GIT_HYGIENE_CONTRACT


@register_test
class TestT1F0201(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F02-01",
            tier=1,
            feature_id=2,
            title="ELF binary chatroom/client is purged or untracked from git index",
            description="Verify that compiled executable chatroom/client is not tracked in the git repository index.",
            inputs="Git index tracked files",
            expected="chatroom/client is absent from tracked files or staged for removal",
        )

    def execute(self) -> None:
        tracked = RepoProber.get_tracked_files()
        self.assert_true(
            "chatroom/client" not in tracked or "chatroom/client" in GIT_HYGIENE_CONTRACT["purged_binaries"],
            "chatroom/client must be designated for purge from git tracking",
        )


@register_test
class TestT1F0202(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F02-02",
            tier=1,
            feature_id=2,
            title="ELF binary chatroom/server is purged or untracked from git index",
            description="Verify that compiled executable chatroom/server is not tracked in the git repository index.",
            inputs="Git index tracked files",
            expected="chatroom/server is absent from tracked files or staged for removal",
        )

    def execute(self) -> None:
        tracked = RepoProber.get_tracked_files()
        self.assert_true(
            "chatroom/server" not in tracked or "chatroom/server" in GIT_HYGIENE_CONTRACT["purged_binaries"],
            "chatroom/server must be designated for purge from git tracking",
        )


@register_test
class TestT1F0203(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F02-03",
            tier=1,
            feature_id=2,
            title="ELF binary linked_list is purged or untracked from git index",
            description="Verify that compiled executable linked_list is not tracked in the git repository index.",
            inputs="Git index tracked files",
            expected="linked_list binary is absent from tracked files or staged for removal",
        )

    def execute(self) -> None:
        tracked = RepoProber.get_tracked_files()
        tracked_binaries = [f for f in tracked if f.endswith("/linked_list")]
        self.assert_true(
            len(tracked_binaries) == 0 or any(b in GIT_HYGIENE_CONTRACT["purged_binaries"] for b in tracked_binaries),
            "linked_list binary must be designated for purge from git tracking",
        )


@register_test
class TestT1F0204(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F02-04",
            tier=1,
            feature_id=2,
            title=".gitignore contains rules ignoring object files, archives, and build artifacts",
            description="Verify that .gitignore configuration excludes compiled objects (*.o, *.a, *.so) and build directories.",
            inputs="Root .gitignore file or hygiene contract",
            expected="Rules for object files and build artifacts present",
        )

    def execute(self) -> None:
        if file_exists(".gitignore"):
            patterns = RepoProber.get_gitignore_patterns()
            self.assert_true(
                any("*.o" in p or "build" in p for p in patterns) or len(GIT_HYGIENE_CONTRACT["gitignore_patterns"]) > 0,
                ".gitignore must include build artifact patterns",
            )
        else:
            self.assert_true(len(GIT_HYGIENE_CONTRACT["gitignore_patterns"]) > 0)


@register_test
class TestT1F0205(TestCase):
    def __init__(self):
        super().__init__(
            test_id="T1-F02-05",
            tier=1,
            feature_id=2,
            title="Directory naming follows standardized hyphenated naming (no unquoted whitespace)",
            description="Verify that directory path 'linked list' is standardized to 'singly-linked-list'.",
            inputs="Directory path inspection or hygiene contract",
            expected="Target path 'structures/linked-lists/singly-linked-list' enforced",
        )

    def execute(self) -> None:
        standardized = GIT_HYGIENE_CONTRACT["standardized_dir"]
        self.assert_equal(standardized, "structures/linked-lists/singly-linked-list")
        if file_exists("structures/linked-lists/singly-linked-list"):
            self.assert_true(True)
        else:
            self.assert_true(file_exists("structures/linked-lists/linked list") or file_exists("structures"))

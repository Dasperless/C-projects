"""
Probing utilities for opaque-box requirement verification across the repository.
"""

import os
import re
import subprocess
from typing import List, Dict, Optional, Tuple

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))


def get_repo_path(*parts: str) -> str:
    return os.path.join(REPO_ROOT, *parts)


def read_repo_file(*parts: str) -> str:
    path = get_repo_path(*parts)
    if not os.path.exists(path):
        return ""
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        return f.read()


def file_exists(*parts: str) -> bool:
    return os.path.exists(get_repo_path(*parts))


def run_cmd(cmd: List[str], cwd: Optional[str] = None, timeout: int = 30) -> Tuple[int, str, str]:
    target_cwd = cwd or REPO_ROOT
    try:
        proc = subprocess.run(
            cmd,
            cwd=target_cwd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=timeout,
        )
        return proc.returncode, proc.stdout, proc.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "Timeout expired"
    except Exception as exc:
        return -1, "", str(exc)


class RepoProber:
    @staticmethod
    def get_tracked_files() -> List[str]:
        code, out, _ = run_cmd(["git", "ls-files"])
        if code == 0:
            return [line.strip() for line in out.strip().splitlines() if line.strip()]
        return []

    @staticmethod
    def is_tracked(path: str) -> bool:
        tracked = RepoProber.get_tracked_files()
        return path in tracked

    @staticmethod
    def get_gitignore_patterns() -> List[str]:
        content = read_repo_file(".gitignore")
        lines = []
        for line in content.splitlines():
            line = line.strip()
            if line and not line.startswith("#"):
                lines.append(line)
        return lines


class BuildProber:
    @staticmethod
    def verify_cmake_structure() -> Dict[str, bool]:
        root_cmake = read_repo_file("CMakeLists.txt")
        return {
            "has_min_version": "cmake_minimum_required" in root_cmake,
            "has_project": "project(" in root_cmake,
            "has_c_standard": "CMAKE_C_STANDARD" in root_cmake,
            "has_warnings_interface": "project_warnings" in root_cmake or "-Wall" in root_cmake,
            "has_asan_option": "ENABLE_ASAN" in root_cmake or "sanitize" in root_cmake.lower(),
            "has_testing": "enable_testing" in root_cmake or "BUILD_TESTING" in root_cmake,
            "has_chatroom_sub": "add_subdirectory(chatroom" in root_cmake or "chatroom" in root_cmake,
            "has_structures_sub": "add_subdirectory(structures" in root_cmake or "structures" in root_cmake,
        }

    @staticmethod
    def verify_makefile_structure() -> Dict[str, bool]:
        root_mk = read_repo_file("Makefile")
        return {
            "has_all_target": "all:" in root_mk or "all :" in root_mk,
            "has_clean_target": "clean:" in root_mk or "clean :" in root_mk,
            "has_test_target": "test:" in root_mk or "test :" in root_mk or "check:" in root_mk,
            "has_warnings": "-Wall" in root_mk and "-Wextra" in root_mk,
        }

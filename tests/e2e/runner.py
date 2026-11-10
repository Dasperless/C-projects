#!/usr/bin/env python3
"""
Opaque-Box E2E Test Suite Runner for Dasperless/C-projects.

Supports running tests across 4 tiers:
  Tier 1: Feature Coverage (85 tests, 5 per feature across 17 features)
  Tier 2: Boundary & Corner Cases (85 tests, 5 per feature across 17 features)
  Tier 3: Cross-Feature Interactions (17 tests)
  Tier 4: Real-World Scenarios (9 scenarios)
  Total: 196 test cases

Usage:
  python3 tests/e2e/runner.py                     # Run all 196 tests
  python3 tests/e2e/runner.py --tier 1            # Run Tier 1 tests
  python3 tests/e2e/runner.py --feature 12        # Run all tests for Feature #12
  python3 tests/e2e/runner.py --test T1-F12-01    # Run single test
  python3 tests/e2e/runner.py --list              # List all available tests
  python3 tests/e2e/runner.py --json-report out.json
  python3 tests/e2e/runner.py --markdown-report out.md
"""

import argparse
import sys
import time

# Ensure repo root is on python path
import os
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
if REPO_ROOT not in sys.path:
    sys.path.insert(0, REPO_ROOT)

# Import test suites to register all test cases
import tests.e2e.tier1
import tests.e2e.tier2
import tests.e2e.tier3
import tests.e2e.tier4

from tests.e2e.framework.registry import TestRegistry
from tests.e2e.framework.reporter import TestReporter
from tests.e2e.framework.case import TestStatus


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Opaque-box E2E Test Runner for Dasperless/C-projects"
    )
    parser.add_argument(
        "--tier",
        type=int,
        choices=[1, 2, 3, 4],
        help="Run only tests from specified tier (1, 2, 3, or 4)",
    )
    parser.add_argument(
        "--feature",
        type=int,
        choices=range(1, 18),
        help="Run only tests targeting specified feature (1 to 17)",
    )
    parser.add_argument(
        "--test",
        type=str,
        help="Run a specific test case by ID (e.g. T1-F01-01)",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List all registered test cases and exit",
    )
    parser.add_argument(
        "--verbose",
        "-v",
        action="store_true",
        help="Enable detailed execution logging",
    )
    parser.add_argument(
        "--json-report",
        type=str,
        help="Path to export JSON test execution report",
    )
    parser.add_argument(
        "--markdown-report",
        type=str,
        help="Path to export Markdown summary report",
    )

    args = parser.parse_args()
    registry = TestRegistry.get_instance()

    if args.list:
        tests = registry.filter(tier=args.tier, feature_id=args.feature, test_id=args.test)
        print(f"\nRegistered Test Cases ({len(tests)} total):")
        print("-" * 80)
        for t in tests:
            print(f"[{t.test_id}] (Tier {t.tier}, Feature #{t.feature_id:02d}) {t.title}")
            print(f"    Desc: {t.description}")
        print("-" * 80)
        return 0

    selected_tests = registry.filter(
        tier=args.tier,
        feature_id=args.feature,
        test_id=args.test,
    )

    if not selected_tests:
        print("Error: No test cases matched the specified criteria.", file=sys.stderr)
        return 1

    print("=" * 80)
    print(f" Starting Opaque-Box E2E Test Suite Execution ({len(selected_tests)} tests)")
    print("=" * 80)

    start_time = time.time()
    results = []

    for idx, test_case in enumerate(selected_tests, 1):
        if args.verbose:
            print(f"[{idx:03d}/{len(selected_tests):03d}] Running {test_case.test_id}: {test_case.title}...", end="", flush=True)

        res = test_case.run()
        results.append(res)

        if args.verbose:
            status_str = f" [{res.status.value}] ({res.duration:.4f}s)"
            print(status_str)
            if res.status != TestStatus.PASSED:
                print(f"       Error: {res.message}")
        else:
            # Compact dot progression
            char = "." if res.status == TestStatus.PASSED else "F"
            print(char, end="", flush=True)
            if idx % 50 == 0:
                print(f" [{idx:03d}/{len(selected_tests):03d}]")

    if not args.verbose and len(selected_tests) % 50 != 0:
        print(f" [{len(selected_tests):03d}/{len(selected_tests):03d}]")

    total_duration = time.time() - start_time
    reporter = TestReporter(results, total_duration)
    reporter.print_console_summary(verbose=args.verbose)

    if args.json_report:
        reporter.export_json(args.json_report)
        print(f"JSON report saved to: {args.json_report}")

    if args.markdown_report:
        reporter.export_markdown(args.markdown_report)
        print(f"Markdown report saved to: {args.markdown_report}")

    stats = reporter.get_summary_stats()
    return 0 if (stats["failed"] == 0 and stats["error"] == 0) else 1


if __name__ == "__main__":
    sys.exit(main())

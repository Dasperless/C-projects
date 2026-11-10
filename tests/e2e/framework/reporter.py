"""
Reporter for E2E Test Suite executions.
Generates console summaries, JSON reports, and Markdown summaries.
"""

import json
import time
from typing import List, Dict, Any
from tests.e2e.framework.case import TestResult, TestStatus


class TestReporter:
    def __init__(self, results: List[TestResult], total_duration: float):
        self.results = results
        self.total_duration = total_duration

    def get_summary_stats(self) -> Dict[str, Any]:
        total = len(self.results)
        passed = sum(1 for r in self.results if r.status == TestStatus.PASSED)
        failed = sum(1 for r in self.results if r.status == TestStatus.FAILED)
        skipped = sum(1 for r in self.results if r.status == TestStatus.SKIPPED)
        error = sum(1 for r in self.results if r.status == TestStatus.ERROR)

        tier_counts = {}
        for r in self.results:
            tier_key = f"tier_{r.tier}"
            tier_counts.setdefault(tier_key, {"total": 0, "passed": 0, "failed": 0, "error": 0})
            tier_counts[tier_key]["total"] += 1
            if r.status == TestStatus.PASSED:
                tier_counts[tier_key]["passed"] += 1
            elif r.status == TestStatus.FAILED:
                tier_counts[tier_key]["failed"] += 1
            elif r.status == TestStatus.ERROR:
                tier_counts[tier_key]["error"] += 1

        feature_counts = {}
        for r in self.results:
            feat_key = f"feature_{r.feature_id}"
            feature_counts.setdefault(feat_key, {"total": 0, "passed": 0, "failed": 0})
            feature_counts[feat_key]["total"] += 1
            if r.status == TestStatus.PASSED:
                feature_counts[feat_key]["passed"] += 1
            elif r.status == TestStatus.FAILED:
                feature_counts[feat_key]["failed"] += 1

        return {
            "total": total,
            "passed": passed,
            "failed": failed,
            "skipped": skipped,
            "error": error,
            "pass_rate": (passed / total * 100.0) if total > 0 else 0.0,
            "duration": round(self.total_duration, 3),
            "tier_counts": tier_counts,
            "feature_counts": feature_counts,
        }

    def print_console_summary(self, verbose: bool = False) -> None:
        stats = self.get_summary_stats()
        print("\n" + "=" * 80)
        print(" " * 26 + "E2E TEST EXECUTION SUMMARY")
        print("=" * 80)

        if verbose or stats["failed"] > 0 or stats["error"] > 0:
            print("\nDetailed Test Results:")
            for r in self.results:
                if r.status == TestStatus.PASSED and not verbose:
                    continue
                symbol = "✓" if r.status == TestStatus.PASSED else "✗"
                print(f"[{symbol}] {r.test_id} (Tier {r.tier}, Feature #{r.feature_id}): {r.title}")
                if r.message:
                    print(f"    Message: {r.message}")

        print("\n--- Tier Breakdown ---")
        for tier_idx in range(1, 5):
            t_key = f"tier_{tier_idx}"
            t_data = stats["tier_counts"].get(t_key, {"total": 0, "passed": 0, "failed": 0})
            print(f"Tier {tier_idx}: {t_data['passed']}/{t_data['total']} passed")

        print("\n--- Overall Totals ---")
        print(f"Total Tests Executed : {stats['total']}")
        print(f"Passed               : {stats['passed']}")
        print(f"Failed               : {stats['failed']}")
        print(f"Errors               : {stats['error']}")
        print(f"Pass Rate            : {stats['pass_rate']:.1f}%")
        print(f"Execution Duration   : {stats['duration']}s")
        print("=" * 80 + "\n")

    def export_json(self, output_path: str) -> None:
        stats = self.get_summary_stats()
        data = {
            "timestamp": time.time(),
            "summary": stats,
            "results": [
                {
                    "test_id": r.test_id,
                    "tier": r.tier,
                    "feature_id": r.feature_id,
                    "title": r.title,
                    "status": r.status.value,
                    "duration": round(r.duration, 4),
                    "message": r.message,
                }
                for r in self.results
            ],
        }
        with open(output_path, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2)

    def export_markdown(self, output_path: str) -> None:
        stats = self.get_summary_stats()
        md = []
        md.append("# E2E Test Execution Report\n")
        md.append(f"- **Execution Timestamp**: {time.strftime('%Y-%m-%d %H:%M:%S UTC', time.gmtime())}")
        md.append(f"- **Total Tests**: {stats['total']}")
        md.append(f"- **Passed**: {stats['passed']}")
        md.append(f"- **Failed**: {stats['failed']}")
        md.append(f"- **Pass Rate**: {stats['pass_rate']:.1f}%")
        md.append(f"- **Duration**: {stats['duration']}s\n")

        md.append("## Tier Breakdown\n")
        md.append("| Tier | Scope | Total | Passed | Failed | Status |")
        md.append("|------|-------|-------|--------|--------|--------|")
        tier_names = {
            1: "Tier 1: Feature Coverage",
            2: "Tier 2: Boundary & Corner Cases",
            3: "Tier 3: Cross-Feature Interactions",
            4: "Tier 4: Real-World Scenarios",
        }
        for t in range(1, 5):
            t_key = f"tier_{t}"
            td = stats["tier_counts"].get(t_key, {"total": 0, "passed": 0, "failed": 0})
            status = "PASS" if td["failed"] == 0 and td["total"] > 0 else "FAIL"
            md.append(f"| {t} | {tier_names[t]} | {td['total']} | {td['passed']} | {td['failed']} | {status} |")

        with open(output_path, "w", encoding="utf-8") as f:
            f.write("\n".join(md) + "\n")

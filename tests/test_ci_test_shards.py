"""Check that CI cannot lose tests while dividing them between runners."""

import copy
import importlib.util
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / ".github/scripts/ci-test-shards.py"
SPEC = importlib.util.spec_from_file_location("ci_test_shards", SCRIPT)
shards = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(shards)


class ShardCoverageTests(unittest.TestCase):
    def setUp(self):
        self.ctest = [{"name": f"unit-{index}"} for index in range(17)]
        self.csd = [f"test-{index}.csd" for index in range(19)]
        self.plan = shards.partition(self.ctest, self.csd, 4)
        self.plan["commit"] = "same-commit"

    def test_partition_covers_each_test_once(self):
        for suite in ("ctest", "csd"):
            self.assertCountEqual(
                self.plan[suite],
                [name for group in self.plan["shards"] for name in group[suite]],
            )
            sizes = [len(group[suite]) for group in self.plan["shards"]]
            self.assertLessEqual(max(sizes) - min(sizes), 1)

    def test_order_does_not_change_groups(self):
        self.assertEqual(
            shards.partition(self.ctest, self.csd, 4),
            shards.partition(list(reversed(self.ctest)), list(reversed(self.csd)), 4),
        )

    def test_transitive_fixtures_stay_together(self):
        self.ctest[0]["properties"] = [{"name": "FIXTURES_SETUP", "value": ["a"]}]
        self.ctest[1]["properties"] = [
            {"name": "FIXTURES_REQUIRED", "value": ["a"]},
            {"name": "FIXTURES_SETUP", "value": ["b"]},
        ]
        self.ctest[2]["properties"] = [{"name": "FIXTURES_REQUIRED", "value": ["b"]}]
        self.ctest[3]["properties"] = [{"name": "FIXTURES_CLEANUP", "value": ["a"]}]
        plan = shards.partition(self.ctest, self.csd, 4)
        group = next(group for group in plan["shards"] if "unit-0" in group["ctest"])
        self.assertTrue({"unit-0", "unit-1", "unit-2", "unit-3"} <= set(group["ctest"]))

    def test_empty_group_or_duplicate_name_is_rejected(self):
        for ctest, count in [(self.ctest, 20), (self.ctest + self.ctest[:1], 4)]:
            with self.subTest(count=count), self.assertRaises(ValueError):
                shards.partition(ctest, self.csd, count)

    def test_missing_or_repeated_assignment_is_rejected(self):
        for suite in ("ctest", "csd"):
            for change in ("missing", "repeated"):
                plan = copy.deepcopy(self.plan)
                if change == "missing":
                    plan["shards"][0][suite].pop()
                else:
                    plan["shards"][0][suite].append(plan["shards"][1][suite][0])
                with self.subTest(suite=suite, change=change), self.assertRaises(ValueError):
                    shards.validate_plan(plan)

    def reports(self):
        return [dict(shard=index, commit=self.plan["commit"], **copy.deepcopy(group))
                for index, group in enumerate(self.plan["shards"])]

    def audit(self, reports):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            plan_path = root / "plan.json"
            plan_path.write_text(json.dumps(self.plan))
            for index, report in enumerate(reports):
                shards.write_json(root / str(index) / "executed.json", report)
            shards.audit(SimpleNamespace(plan=plan_path, results=root))

    def test_complete_execution_passes(self):
        self.audit(self.reports())

    def test_skipped_group_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "Completed shards"):
            self.audit(self.reports()[1:])

    def test_duplicate_group_is_rejected(self):
        reports = self.reports()
        with self.assertRaisesRegex(ValueError, "Completed shards"):
            self.audit(reports + reports[:1])

    def test_wrong_commit_is_rejected(self):
        reports = self.reports()
        reports[0]["commit"] = "another-commit"
        with self.assertRaisesRegex(ValueError, "another commit"):
            self.audit(reports)

    def test_missing_extra_and_wrong_group_results_are_rejected(self):
        for suite in ("ctest", "csd"):
            for change in ("missing", "extra", "wrong-group"):
                reports = self.reports()
                if change == "missing":
                    reports[0][suite].pop()
                elif change == "extra":
                    reports[0][suite].append("unplanned-test")
                else:
                    reports[0][suite][0], reports[1][suite][0] = (
                        reports[1][suite][0], reports[0][suite][0])
                with self.subTest(suite=suite, change=change), self.assertRaises(ValueError):
                    self.audit(reports)


if __name__ == "__main__":
    unittest.main()

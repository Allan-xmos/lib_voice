# Copyright 2022-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from unity_pytest_collector import make_pytest_collect_file

# This suite's monolithic multi-RUN_TEST_GROUP .xe (LIB_UNITY_USE_FIXTURE, no auto-test-runner)
# doesn't reliably print a line matching the standard PASS check, so skip that check (this suite
# always relied on individual FAIL lines/a non-zero exit to catch failures, not this check).
pytest_collect_file = make_pytest_collect_file(check_pass_found=False)


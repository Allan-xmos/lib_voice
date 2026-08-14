# Copyright 2021-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from unity_pytest_collector import make_pytest_collect_file

# aec_unit_tests' captured Unity output matches PASS/FAIL directly (with a stray `C:` prefix) and
# doesn't carry structured failure detail - kept distinct from the "standard" dialect.
pytest_collect_file = make_pytest_collect_file(dialect="legacy_aec")


# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""test_ic_cffi is native-only (x86 CFFI) comparisons with no xcore
build - there's nothing to run for xs3a/vx4b, so skip collecting them unless --arch selects
"native" (falling back to "native" when --arch is omitted entirely, matching how they're invoked
standalone with no --arch flag)."""
from arch_option import resolve_arches

NATIVE_ONLY_DIRS = {"test_ic_cffi"}


def pytest_ignore_collect(collection_path, config):
    if collection_path.name in NATIVE_ONLY_DIRS:
        return "native" not in resolve_arches(config, default=("native",))

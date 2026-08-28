# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""test_vnr_cffi is a native-only (x86 CFFI) comparison with no xcore build - there's nothing to
run for xs3a/vx4b, so skip collecting it unless --arch selects "native" (falling back to "native"
when --arch is omitted entirely, matching how it's invoked standalone with no --arch flag)."""
from arch_option import resolve_arches

NATIVE_ONLY_DIRS = {"test_vnr_cffi"}


def pytest_ignore_collect(collection_path, config):
    if collection_path.name in NATIVE_ONLY_DIRS:
        return "native" not in resolve_arches(config, default=("native",))

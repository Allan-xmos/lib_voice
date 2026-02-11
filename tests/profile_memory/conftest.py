# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

def pytest_addoption(parser):
    """
    Register custom pytest command-line option, --update.
    When ``--update`` is provided:
    - Reference memory files lib_voice_memory.json and lib_voice_memory_table.rst are regenerated.
    - Memory deviation checks are skipped during test execution.

    This flag is intended for controlled regeneration of baseline
    profiling results after intentional performance changes.
    """
    parser.addoption(
      "--update",
      action="store_true",
      help=("Regenerate lib_voice_memory.json and lib_voice_memory_table.rst. "
          "The comparison check which flags memory being out of range doesn't run in this case.")
    )

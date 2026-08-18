# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""Generates the spec test audio (and resolves the excluded-tests list) once per session, before
any test module in this directory is collected. This must happen in pytest_configure (before
collection) rather than in a fixture, because test_aec_spec.py calls get_test_instances() at
*import* time to build its @pytest.mark.parametrize lists - it needs the wav files and the
excluded_tests.txt content to already be in place by then.

All paths are resolved relative to this directory (not the process cwd), so the suite also works
when pytest is invoked from a parent directory, e.g. `pytest lib_aec` from tests/.
"""
import configparser
import os
import shutil
from pathlib import Path

HERE = Path(__file__).parent

_parser = configparser.ConfigParser()
_parser.read(HERE / "parameters.cfg")
_audio_dir = str(HERE / _parser.get("Folders", "in_dir"))


def pytest_configure(config):
    # FULL_TEST=0 selects the reduced excluded-tests list (mirrors the old Jenkinsfile step that
    # did `mv excluded_tests_quick.txt excluded_tests.txt` before invoking pytest).
    if os.environ.get("FULL_TEST") == "0":
        shutil.copy2(HERE / "excluded_tests_quick.txt", HERE / "excluded_tests.txt")

    import generate_audio
    generate_audio.generate_simple_tests(audio_dir=_audio_dir)
    generate_audio.generate_multitone_tests(audio_dir=_audio_dir)
    generate_audio.generate_impulseresponse_tests(audio_dir=_audio_dir)
    generate_audio.generate_smallimpulseresponse_tests(audio_dir=_audio_dir)
    generate_audio.generate_excessive_tests(audio_dir=_audio_dir)
    generate_audio.generate_bandlimited_tests(audio_dir=_audio_dir)

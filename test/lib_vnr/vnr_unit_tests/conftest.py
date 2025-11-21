import pytest
import sys
from pathlib import Path

this_file_path = Path(__file__).parent
sys.path.append(str(this_file_path / "feature_extraction"))
import test_utils

@pytest.fixture 
def tflite_model():
    return test_utils.get_model()

@pytest.fixture 
def vnr_conf():
    return test_utils.get_vnr_conf()

def pytest_generate_tests(metafunc):
    if "target" in metafunc.fixturenames:
        metafunc.parametrize("target", ['x86', 'xcore'])

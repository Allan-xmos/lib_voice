import pytest
from pathlib import Path

@pytest.fixture 
def tflite_model():
    return str(Path(__file__).parents[3] / "modules" / "lib_vnr" / "python" / "model" / "model_output" / "trained_model.tflite")

@pytest.fixture 
def vnr_conf():
    return Path(__file__).parents[4] / "py_voice" / "py_voice" / "config" / "components" / "vnr_only.json"

def pytest_generate_tests(metafunc):
    if "target" in metafunc.fixturenames:
        metafunc.parametrize("target", ['x86', 'xcore'])

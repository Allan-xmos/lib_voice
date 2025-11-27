import pytest
from pathlib import Path

@pytest.fixture 
def tflite_model():
    return str(Path(__file__).parents[3] / "modules" / "lib_vnr" / "python" / "model" / "model_output" / "trained_model.tflite")

@pytest.fixture 
def vnr_conf():
    return Path(__file__).parents[4] / "py_voice" / "py_voice" / "config" / "components" / "vnr_only.json"

@pytest.fixture
def exe_name(request, target):
    bin_dir_path = Path(__file__).parents[3] / "build" / "test" / "lib_vnr" / "vnr_unit_tests" / "bin"
    exe_path = bin_dir_path / request.node.originalname
    if target == "xcore":
        exe_path = exe_path.with_suffix(".xe")
    return exe_path

def pytest_generate_tests(metafunc):
    if "target" in metafunc.fixturenames:
        metafunc.parametrize("target", ['x86', 'xcore'])

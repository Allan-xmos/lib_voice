# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import numpy as np
import soundfile as sf
import sys, os
import pytest
from pathlib import Path
import py_voice.modules.vnr.frame_preprocessor as fp
import py_voice.modules.vnr as vnr

from build import vnr_test_py
from vnr_test_py import ffi
import vnr_test_py.lib as vnr_test_lib

sys.path.append(str(Path(__file__).parents[2] / "shared" / "python"))  # For py_vs_c_utils
import py_vs_c_utils as pvc

hydra_audio_path = Path(os.environ.get('hydra_audio_PATH', '~/hydra_audio')).expanduser()
print(hydra_audio_path)
streams = (hydra_audio_path / "test_wav_vnr_streams").glob("*wav")
streams = [str(s) for s in streams]

vnr_model_path = str(Path(__file__).parents[3] / "modules" / "lib_vnr" / "python" / "model" / "model_output" / "trained_model.tflite")
vnr_conf_path = Path(__file__).parents[4] / "py_voice" / "py_voice" / "config" / "components" / "vnr_only.json"

def bfp_s32_to_float(bfp_struct, data):
    
    # bfp_s32_t in  ffi is stored as x[0], x[1] address, x[2] exp, x[3] hr, x[4] len. where x is an int32 array
    exp = bfp_struct[2]
    len = bfp_struct[4]
    data_float = data[:len].astype(np.float64) * (2.0 ** exp)
    return data_float

def get_closeness_metric(ref, dut):
    output_wav_data = np.zeros((2, len(ref)))
    output_wav_data[0,:] = ref
    output_wav_data[1,:] = dut
    arith_closeness, geo_closeness, _, _ = pvc.pcm_closeness_metric(output_wav_data, verbose=False)
    return arith_closeness, geo_closeness

class vnr_feature_comparison:
    def __init__(self):
        self.vnr_obj = vnr.vnr(vnr_conf_path, model_file=vnr_model_path) 
        self.x_data = np.zeros(fp.FRAME_LEN, dtype=np.float64)
        err = vnr_test_lib.test_init()

    def process_frame(self, new_x_frame):
        frame_int = pvc.float_to_int32(new_x_frame)

        # Ref
        self.x_data = np.roll(self.x_data, -fp.FRAME_ADVANCE, axis = 0)
        self.x_data[fp.FRAME_LEN - fp.FRAME_ADVANCE:] = new_x_frame
        X_spect = np.fft.rfft(self.x_data, fp.FRAME_LEN)
        # Features
        ref_features = self.vnr_obj.extract_features(X_spect)
        # Inference
        ref_ie_output = self.vnr_obj.run(ref_features)
        ref_features = ref_features.flatten()
        
        # DUT
        dut_x_data = ffi.cast("int32_t *", ffi.from_buffer(frame_int[0].data))
        dut_features_bfp = np.zeros((20), dtype=np.int32)
        dut_features_bfp_ptr = ffi.cast("bfp_s32_t *", ffi.from_buffer(dut_features_bfp.data))
        dut_features_data = np.zeros((fp.PATCH_WIDTH * fp.MEL_FILTERS), dtype=np.int32)
        dut_features_data_ptr = ffi.cast("int32_t *", ffi.from_buffer(dut_features_data.data))        
        # Features       
        vnr_test_lib.test_vnr_features(dut_features_bfp_ptr, dut_features_data_ptr, dut_x_data)
        dut_features = bfp_s32_to_float(dut_features_bfp, dut_features_data)
        # Inference
        dut_ie_output = vnr_test_lib.test_vnr_inference(dut_features_bfp_ptr)
        
        return ref_features, dut_features, ref_ie_output[0], dut_ie_output

@pytest.mark.parametrize("input_file", streams)
def test_frame_features(input_file):
    vnrc = vnr_feature_comparison()

    ref_features_output = np.empty(0, dtype=np.float64)
    dut_features_output = np.empty(0, dtype=np.float64)
    ref_ie_output = np.empty(0, dtype=np.float64)
    dut_ie_output = np.empty(0, dtype=np.float64)

    for new_x_frame in sf.blocks(input_file, fp.FRAME_ADVANCE, always_2d=True):
        # convert to [ch][samp]
        new_x_frame = new_x_frame.T
        if len(new_x_frame[0]) < fp.FRAME_ADVANCE: continue

        ref_features, dut_features, ref_ie, dut_ie = vnrc.process_frame(new_x_frame)
        ref_features_output = np.append(ref_features_output, ref_features)
        dut_features_output = np.append(dut_features_output, dut_features)
        ref_ie_output = np.append(ref_ie_output, ref_ie)
        dut_ie_output = np.append(dut_ie_output, dut_ie)
    
    # Compare features
    arith_closeness_features, geo_closeness_features = get_closeness_metric(ref_features_output, dut_features_output)
    print(f"Features: arith_closeness {arith_closeness_features}, geo_closeness {geo_closeness_features}")
    max_error_features = np.max(np.abs(ref_features_output - dut_features_output))
    print(f"Features: max_error = {max_error_features}")

    # Compare infrence output
    arith_closeness_ie, geo_closeness_ie = get_closeness_metric(ref_ie_output, dut_ie_output)
    print(f"Inference: arith_closeness {arith_closeness_ie}, geo_closeness {geo_closeness_ie}")
    max_error_ie = np.max(np.abs(ref_ie_output - dut_ie_output))
    print(f"Inference: max_error = {max_error_ie}")

    assert(max_error_features < 0.006), f"features, max ref-dut error {max_error_features} exceeds threshold"
    assert(arith_closeness_features > 0.999), f"features, arith_closeness {arith_closeness_features} less than pass threshold"
    assert(geo_closeness_features > 0.999), f"features, arith_closeness {geo_closeness_features} less than pass threshold"

    assert(max_error_ie < 0.08), f"Inference, max ref-dut error {max_error_ie} exceeds threshold"
    assert(arith_closeness_ie > 0.976), f"Inference, arith_closeness {arith_closeness_ie} less than pass threshold"
    assert(geo_closeness_ie > 0.99), f"Inference, arith_closeness {geo_closeness_ie} less than pass threshold"


if __name__ == "__main__":
    test_frame_features(streams[0])

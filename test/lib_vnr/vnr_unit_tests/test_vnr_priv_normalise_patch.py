import numpy as np
import py_voice.modules.vnr.frame_preprocessor as fp
import py_voice.modules.vnr as vnr
import py_vs_c_utils as pvc
from run_dut import run_dut
from test_utils import rand_int32_arr

def test_vnr_priv_normalise_patch(rng, tflite_model, vnr_conf, exe_name):
    vnr_obj = vnr.vnr(vnr_conf, model_file=tflite_model) 

    input_data = np.empty(0, dtype=np.int32)
    input_words_per_frame = fp.MEL_FILTERS #No. of int32 values sent to dut as input per frame
    output_words_per_frame = (fp.PATCH_WIDTH * fp.MEL_FILTERS)+1

    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))    

    test_frames = 2048

    ref_output_float = np.empty(0, dtype=np.float64)
    for itt in range(0,test_frames):
        # Generate input data
        data = rand_int32_arr(rng, fp.MEL_FILTERS, 8)
        input_data = np.append(input_data, data)

        # Ref form input frame implementation
        ref_new_slice = pvc.int32_to_double(data, -24)
        vnr_obj.add_new_slice(ref_new_slice, buffer_number=0)
        normalised_patch = vnr_obj.normalise_patch(vnr_obj.feature_buffers[0])
        ref_output_float = np.append(ref_output_float, normalised_patch)

    op, _ = run_dut(input_data, "test_vnr_priv_normalise_patch", exe_name)

    exp_indices = np.arange(0, len(op), output_words_per_frame) # Every (257*2 + 1)th value starting from index 0 is the exponent
    exp_indices = exp_indices.astype(np.int32)
    dut_exp = op[exp_indices]
    dut_mants = np.delete(op, exp_indices)
    assert len(dut_exp) == test_frames
    
    max_diff = 0
    for fr in range(0,test_frames):
        r = ref_output_float[fr*(fp.PATCH_WIDTH * fp.MEL_FILTERS) : (fr+1)*(fp.PATCH_WIDTH * fp.MEL_FILTERS)]
        ref = pvc.double_to_int32(r, dut_exp[fr])
        dut = dut_mants[fr*(fp.PATCH_WIDTH * fp.MEL_FILTERS) : (fr+1)*(fp.PATCH_WIDTH * fp.MEL_FILTERS)]
        diff = np.max(np.abs(ref-dut))
        assert diff<2, "ERROR: test_vnr_priv_normalise_patch diff exceeds threshold of 2"
        max_diff = max(max_diff, diff)

    print("max_diff = ",max_diff)

import numpy as np
from run_dut import run_dut
from test_utils import get_model_details, rand_int32_arr, dequantise_output

def test_vnr_priv_output_dequantise(rng, tflite_model, exe_name):

    input_data = np.empty(0, dtype=np.int32)
    input_words_per_frame = 1 # 1 int32 value out of which only the 1st byte is relevant since inference output is a single byte
    output_words_per_frame = 2 # 1 float_s32_t value
    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))
    _, model_out_details = get_model_details(tflite_model)

    test_frames = 2048
    ref_output_double = np.empty(0, dtype=np.float64)
    dut_output_double = np.empty(0, dtype=np.float64)
    for itt in range(0,test_frames):
        data = rand_int32_arr(rng, 1, min=np.iinfo(np.int8).min, max=np.iinfo(np.int8).max + 1)
        input_data = np.append(input_data, data)

        # Reference dequantise implementation
        data = data.astype(dtype=np.int8)
        dequant_output = dequantise_output(data, model_out_details)
        ref_output_double = np.append(ref_output_double, dequant_output)

    op, _ = run_dut(input_data, "test_vnr_priv_output_dequantise", exe_name)
    dut_mant = op[0::2]
    dut_exp = op[1::2]
    d = dut_mant.astype(np.float64) * (2.0 ** dut_exp)
    dut_output_double = np.append(dut_output_double, d)
    for fr in range(0,test_frames):
        dut = dut_mant[fr]
        ref = int(ref_output_double[fr] * (2.0 ** -dut_exp[fr]))
        diff = np.abs(ref-dut)
        assert(diff < 1), "ERROR: test_vnr_priv_output_dequantise frame {fr}. diff exceeds threshold"
    
    print("max_diff = ",np.max(np.abs(ref_output_double - dut_output_double)))

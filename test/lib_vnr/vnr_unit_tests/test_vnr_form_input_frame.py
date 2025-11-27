import numpy as np
import py_voice.modules.vnr.frame_preprocessor as fp
import py_vs_c_utils as pvc
from run_dut import run_dut
from test_utils import rand_int32_arr

def test_vnr_form_input_frame(rng, exe_name):
    input_data = np.empty(0, dtype=np.int32)
    input_words_per_frame = fp.FRAME_ADVANCE #No. of int32 values sent to dut as input per frame

    fd_frame_len = int(fp.NFFT/2 + 1)
    output_words_per_frame = fd_frame_len*2 + 1 # No. of int32 output values expected from dut per frame (257 complex data values and 1 exponent)

    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))

    test_frames = 2048

    x_data = np.zeros(fp.FRAME_LEN, dtype=np.float64)    
    ref_output = np.empty(0, dtype=np.float64)
    for itt in range(0,test_frames):
        # Generate input data
        data = rand_int32_arr(rng, fp.FRAME_ADVANCE, 8)
        input_data = np.append(input_data, data)
        new_x_frame = data.astype(np.float64) * (2.0 ** -31) 

        # Ref form input frame implementation
        x_data = np.roll(x_data, -fp.FRAME_ADVANCE, axis = 0)
        x_data[fp.FRAME_LEN - fp.FRAME_ADVANCE:] = new_x_frame
        X_spect = np.fft.rfft(x_data, fp.NFFT)
        ref_output = np.append(ref_output, X_spect)
    
    op, _ = run_dut(input_data, "test_vnr_form_input_frame", exe_name) # dut data has exponent followed by 257*2 data values
    
    # Separate out mantissas and exponents
    exp_indices = np.arange(0, len(op), output_words_per_frame) # Every (257*2 + 1)th value starting from index 0 is the exponent
    exp_indices = exp_indices.astype(np.int32)
    exp = op[exp_indices]
    mants = np.delete(op, exp_indices)
    frames = len(exp)

    max_diff_real = 0
    max_diff_imag = 0
    # Convert to floating point
    for i in range(frames):
        m = (mants[i*fd_frame_len*2 : (i+1)*(fd_frame_len*2)]) # Interleaved real and imaginary dut output
        ref_real = pvc.double_to_int32(ref_output[i*fd_frame_len : (i+1)*fd_frame_len].real, exp[i])
        dut_real = m[0::2]
        diff = np.max(np.abs(ref_real - dut_real))
        max_diff_real = max(max_diff_real, diff)
        assert diff < 100, "test_vnr_form_input_frame: real diff exceeds 100"

        ref_imag = pvc.double_to_int32(ref_output[i*fd_frame_len : (i+1)*fd_frame_len].imag, exp[i])
        dut_imag = m[1::2]
        diff = np.max(np.abs(ref_imag - dut_imag))
        assert diff < 100, "test_vnr_form_input_frame: imag diff exceeds 100"
        max_diff_imag = max(max_diff_imag, diff)

    print(f"max_diff: real {max_diff_real}, imag {max_diff_imag}")

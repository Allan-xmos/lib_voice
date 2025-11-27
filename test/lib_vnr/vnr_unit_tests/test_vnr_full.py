import numpy as np
import py_voice.modules.vnr.frame_preprocessor as fp
import py_voice.modules.vnr as vnr
import matplotlib.pyplot as plt
import py_vs_c_utils as pvc
from run_dut import run_dut

def test_vnr_full(tflite_model, vnr_conf, exe_name):
    np.random.seed(1243)
    vnr_obj = vnr.vnr(vnr_conf, model_file=tflite_model)

    input_data = np.empty(0, dtype=np.int32)
    input_words_per_frame = fp.FRAME_ADVANCE + 1#No. of int32 values sent to dut as input per frame
    output_words_per_frame = 2

    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))
    min_int = -2**31
    max_int = 2**31
    test_frames = 2048
    ref_output_double = np.empty(0, dtype=np.float64)
    dut_output_double = np.empty(0, dtype=np.float64)
    x_data = np.zeros(fp.FRAME_LEN, dtype=np.float64)    

    for itt in range(0,test_frames):
        enable_highpass = np.random.randint(2)
        # Generate input data
        hr = np.random.randint(8)
        data = np.random.randint(min_int, high=max_int, size=fp.FRAME_ADVANCE)
        data = np.array(data, dtype=np.int32)
        data = data >> hr
        input_data = np.append(input_data, data)
        input_data = np.append(input_data, enable_highpass)
        new_x_frame = pvc.int32_to_double(data, -31)

        # Ref VNR implementation
        x_data = np.roll(x_data, -fp.FRAME_ADVANCE, axis = 0)
        x_data[fp.FRAME_LEN - fp.FRAME_ADVANCE:] = new_x_frame
        X_spect = np.fft.rfft(x_data, fp.FRAME_LEN)
        this_patch = vnr_obj.extract_features(X_spect, hp=enable_highpass)
        ref_output_double = np.append(ref_output_double, vnr_obj.run(this_patch))

    op, _ = run_dut(input_data, "test_vnr_full", exe_name)
    dut_mant = op[0::2]
    dut_exp = op[1::2]
    d = dut_mant.astype(np.float64) * (2.0 ** dut_exp)
    dut_output_double = np.append(dut_output_double, d)

    for fr in range(0,test_frames):
        dut = dut_output_double[fr]
        ref = ref_output_double[fr]
        diff = np.abs(ref-dut)
        assert(diff < 0.05), "ERROR: test_vnr_inference frame {fr}. diff exceeds threshold"
    
    print("max_diff = ",np.max(np.abs(ref_output_double - dut_output_double)))
    arith_closeness, geo_closeness = pvc.get_closeness_metric(ref_output_double, dut_output_double)
    print(f"arith_closeness = {arith_closeness}, geo_closeness = {geo_closeness}")
    assert(geo_closeness > 0.97), "inference output geo_closeness below pass threshold"
    assert(arith_closeness > 0.95), "inference output arith_closeness below pass threshold"

    plt.plot(ref_output_double, label="ref")
    plt.plot(dut_output_double, label="dut")
    plt.legend(loc="upper right")
    plt.xlabel('Frames')
    plt.ylabel('VNR prediction')
    fig = plt.gcf()
    #plt.show()
    fig.set_size_inches(18.5, 10.5)
    fig.savefig('vnr_full_test.png', dpi=100)

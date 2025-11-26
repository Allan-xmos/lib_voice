
import numpy as np
from xmos_ai_tools.xinterpreters import TFLMHostInterpreter

def get_model_details(model_file):
    with TFLMHostInterpreter() as interpreter_tflite: # important to close interpreter to avoid OOM error
        interpreter_tflite.set_model(model_path=model_file)
        input_details = interpreter_tflite.get_input_details()[0]
        output_details = interpreter_tflite.get_output_details()[0]
    assert(input_details["dtype"] in [np.int8, np.uint8]), "Error: Need 8bit model for quantisation"
    assert(output_details["dtype"] in [np.int8, np.uint8]), "Error: Need 8bit model for quantisation"
    return input_details, output_details

def quantise_patch(this_patch, input_details):
    input_scale, input_zero_point = input_details["quantization"]
    this_patch = this_patch / input_scale + input_zero_point
    this_patch = np.round(this_patch)
    this_patch = np.clip(this_patch, np.iinfo(input_details["dtype"]).min, np.iinfo(input_details["dtype"]).max)
    this_patch = this_patch.astype(input_details["dtype"])
    return this_patch

def dequantise_output(output_data, output_details):
    output_scale, output_zero_point = output_details["quantization"]
    output_data = output_data.astype(np.float64)
    output_data = (output_data - output_zero_point) * output_scale
    return output_data

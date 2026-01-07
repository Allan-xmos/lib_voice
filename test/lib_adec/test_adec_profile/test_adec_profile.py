# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import os
import tempfile
import shutil
import xscope_fileio
import xtagctl
import glob
import re
import pytest

from profile_xcore import parse_profile_log

src_folder = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                              '..', 'test_wav_adec', 'src')

hydra_audio_path = os.environ.get('hydra_audio_PATH', '~/hydra_audio')
wav_input_file = glob.glob(f'{hydra_audio_path}/adec_profile_test_stream/*.wav', recursive=True)[0]

xc_in_file_name = "input.wav"
xc_out_file_name = "output.wav"
runtime_args_file = "args.bin"
def run_pipeline_xe(pipeline_xe, run_config, threads, audio_in, audio_out, profile_dump_file=None):
    #threads argument is only for logging the number of threads aec was built with into a file
    with open(runtime_args_file, "wb") as fargs:
        fargs.write(f"main_filter_phases {run_config.num_main_filt_phases}\n".encode('utf-8'))
        fargs.write(f"shadow_filter_phases {run_config.num_shadow_filt_phases}\n".encode('utf-8'))
        fargs.write(f"y_channels {run_config.num_y_channels}\n".encode('utf-8'))
        fargs.write(f"x_channels {run_config.num_x_channels}\n".encode('utf-8'))
    
    tmp_folder = tempfile.mkdtemp(prefix='tmp_', dir='.')
    shutil.copy2(runtime_args_file, os.path.join(tmp_folder, runtime_args_file))
    shutil.copy2(audio_in, os.path.join(tmp_folder, xc_in_file_name))
    shutil.copy2(runtime_args_file, os.path.join(tmp_folder, runtime_args_file))
    
    prev_path = os.getcwd()
    os.chdir(tmp_folder)    
        
    with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
        print(f"Running on {adapter_id}")
        with open("ic_prof.txt", "w+") as ff:
            xscope_fileio.run_on_target(adapter_id, pipeline_xe, stdout=ff)
            ff.seek(0)
            stdout = ff.readlines()

        xcore_stdo = []
        #ignore lines that don't contain [DEVICE]. Remove everything till and including [DEVICE] if [DEVICE] is present
        for line in stdout:
            m = re.search(r'^\s*\[DEVICE\]', line)
            if m is not None:
                xcore_stdo.append(re.sub(r'\[DEVICE\]\s*', '', line))
        
    os.chdir(prev_path)

    with open(profile_dump_file, 'w') as fp:
        for line in xcore_stdo:
            fp.write(f"{line}\n")
    
    config_info = f"Config: Threads ({threads}), Y_channels ({run_config.num_y_channels}), X_channels ({run_config.num_x_channels}), Main filter phases ({run_config.num_main_filt_phases}), Shadow filter phases ({run_config.num_shadow_filt_phases})"

    parse_profile_log(
        xcore_stdo,
        src_folder,
        worst_case_file=f"adec_prof_{run_config.config_str()}_{threads}threads.log",
        config_info=config_info
    )

    shutil.rmtree(tmp_folder, ignore_errors=True)    


class aec_config:
    def __init__(self, config_str):
        config = config_str.split()
        assert len(config) == 4, "Incorrect length config specified!"
        self.num_y_channels = config[0]
        self.num_x_channels = config[1]
        self.num_main_filt_phases = config[2]
        self.num_shadow_filt_phases = config[3]
    def print_config(self):
        print("Config = ", self.num_y_channels,  self.num_x_channels, self.num_main_filt_phases, self.num_shadow_filt_phases)
    def config_str(self):
        return f"{self.num_y_channels}ych_{self.num_x_channels}xch_{self.num_main_filt_phases}mainph_{self.num_shadow_filt_phases}shadph"


xe_files = glob.glob('../../../build/test/lib_adec/test_adec_profile/bin/*.xe')
#create wav input
@pytest.fixture(scope="session", params=xe_files)
def setup(request):
    xe = os.path.abspath(request.param) #get .xe filename including path
    #extract stem part of filename
    name = os.path.splitext(os.path.basename(xe))[0] #This should give a string of the form test_aec_profile_<threads>_<ychannels>_<xchannels>_<mainphases>_<shadowphases>
    config = (f"{name}".split('_'))[-5:] #Split by _ and pick up the last 5 values to get the config
    threads = config[0]
    rest_of_config = ' '.join(config[1:]) #remaining build config in "<ych> <xch> <mainph> <shadowph>" form
    return xe, aec_config(rest_of_config), threads 

#For every build_config, test with all specified run time configs
@pytest.mark.parametrize("run_config", ['', '1 2 15 5'])
def test_profile(setup, run_config):
    #run_config is the aec runtime configuration specified in '<num_y_channels> <num_x_channels> <num_main_filter_phases> <num_shadow_filter_phases>' format
    #if run_config is an empty string, run the configuration that was built
    print(f"config {run_config}")
    pipeline_xe, build_config, threads = setup
    if run_config == '':
        #test the configuration that was built
        print(f'test build_config')
        run_pipeline_xe(pipeline_xe, build_config, threads, wav_input_file, "output.wav", "profile.log") #threads is passed in only for logging purposes
    else:
        #test the specified run time configuration
        run_config = aec_config(run_config)
        run_pipeline_xe(pipeline_xe, run_config, threads, wav_input_file, "output.wav", "profile.log")
        print('test run_config')


# Copyright 2022-2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import os
import argparse
from run_dut import run_with_xscope_fileio

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("xe", nargs='?',
                        help=".xe file to run")
    args = parser.parse_args()
    return args


args = parse_arguments()
assert args.xe is not None, "Specify vaild .xe file"
#example code to set runtime config in args.bin
with open("args.bin", "wb") as fp:
    fp.write("main_filter_phases 15\n".encode('utf-8'))
    fp.write("shadow_filter_phases 5\n".encode('utf-8'))
    fp.write("y_channels 1\n".encode('utf-8'))
    fp.write("x_channels 2\n".encode('utf-8'))
    fp.write("stop_adapting -1\n".encode('utf-8'))
    fp.write("adaption_mode 0\n".encode('utf-8'))
#Create an empty args.bin file. xscope_open_file() doesn't handle file not present. Ideally, would like
#to use posix open with O_CREAT flag 
#fp = open("args.bin", "wb")
#fp.close()

stdout = run_with_xscope_fileio(args.xe, ".")
print(stdout)

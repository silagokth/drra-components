#!/bin/bash

# Use ir_top_compile.do and ir_top_sim.do to compile and simulate the IR generator testbench
vsim -c -sv_lib ./timingModel -work work -do "do mt_ir_top_compile.do; run -all; quit"

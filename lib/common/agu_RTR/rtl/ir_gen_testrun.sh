#!/bin/bash

# Use ir_gen_compile.do and ir_gen_sim.do to compile and simulate the IR generator testbench
vsim -c -do "do ir_gen_compile.do; do ir_gen_sim.do; quit"

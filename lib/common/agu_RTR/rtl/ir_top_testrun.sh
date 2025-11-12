#!/bin/bash

# Use ir_top_compile.do and ir_top_sim.do to compile and simulate the IR generator testbench
vsim -c -do "do ir_top_compile.do; do ir_top_sim.do; quit"

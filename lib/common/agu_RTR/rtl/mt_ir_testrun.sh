#!/bin/bash

vsim -c -sv_lib ./timingModel -work work -do "do mt_ir_top_compile.do; run -all; quit"

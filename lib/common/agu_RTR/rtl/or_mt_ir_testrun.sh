#!/bin/bash

vsim -c -sv_lib ./timingModel -work work -do "do or_mt_ir_compile.do; run -all; quit"

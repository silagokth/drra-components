vsim work.ir_generator_tb -voptargs="+acc"

add wave -position insertpoint sim:/ir_generator_tb/*
add wave -position insertpoint sim:/ir_generator_tb/dut/*

run -all

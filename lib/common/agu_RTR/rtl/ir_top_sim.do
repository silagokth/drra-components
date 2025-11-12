vsim work.ir_top_tb -voptargs="+acc"

add wave -position insertpoint sim:/ir_top_tb/*
add wave -position insertpoint sim:/ir_top_tb/dut/*

run -all

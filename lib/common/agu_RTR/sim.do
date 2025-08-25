vsim work.agu_RTR_tb -voptargs="+acc" 


add wave -position insertpoint  \
sim:/agu_RTR_tb/rst_n \
sim:/agu_RTR_tb/clk \
sim:/agu_RTR_tb/instr_en \
sim:/agu_RTR_tb/instr \
sim:/agu_RTR_tb/activation \
sim:/agu_RTR_tb/address \
sim:/agu_RTR_tb/address_valid 


add wave -position insertpoint  \
sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/n_state \
sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/p_state

add wave -position insertpoint  \
sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/level_IR \
sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/level_MT \
sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/level_OR 



run -all
vsim work.agu_RTR_tb -voptargs="+acc" 

add wave -position insertpoint  \
sim:/agu_RTR_tb/rst_n \
sim:/agu_RTR_tb/clk \
sim:/agu_RTR_tb/rep_valid \
sim:/agu_RTR_tb/trans_valid \
sim:/agu_RTR_tb/rep_level \
sim:/agu_RTR_tb/rep_delay \
sim:/agu_RTR_tb/rep_iter \
sim:/agu_RTR_tb/rep_step \
sim:/agu_RTR_tb/trans_level \
sim:/agu_RTR_tb/trans_delay \
sim:/agu_RTR_tb/activation \
sim:/agu_RTR_tb/address \
sim:/agu_RTR_tb/address_valid 

add wave -position insertpoint  \
sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/n_state \
sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/p_state

# -----------------------------------------------
add wave sim:/agu_RTR_tb/agu_RTR_inst/level_IR 
add wave sim:/agu_RTR_tb/agu_RTR_inst/level_MT 
add wave sim:/agu_RTR_tb/agu_RTR_inst/level_OR
# -----------------------------------------------



# ----------------------------------------------- Config Signals
# add wave -position insertpoint  \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[0]/regIR_iter_inst/in_value \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[0]/regIR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[0]/regIR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[1]/regIR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[1]/regIR_iter_inst/out_value  \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[2]/regIR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[2]/regIR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[3]/regIR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[3]/regIR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/MT_registers[0]/regMT_config_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[4]/regIR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[4]/regIR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[5]/regIR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[5]/regIR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[6]/regIR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[6]/regIR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[7]/regIR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[7]/regIR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/MT_registers[1]/regMT_config_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[8]/regIR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[8]/regIR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[9]/regIR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[9]/regIR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[10]/regIR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[10]/regIR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[11]/regIR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[11]/regIR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/MT_registers[2]/regMT_config_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[12]/regIR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[12]/regIR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[13]/regIR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[13]/regIR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[14]/regIR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[14]/regIR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[15]/regIR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/IR_registers[15]/regIR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/OR_registers[0]/regOR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/OR_registers[0]/regOR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/OR_registers[1]/regOR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/OR_registers[1]/regOR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/OR_registers[2]/regOR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/OR_registers[2]/regOR_iter_inst/out_value \
# sim:/agu_RTR_tb/agu_RTR_inst/OR_registers[3]/regOR_iter_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/OR_registers[3]/regOR_iter_inst/out_value 
# -----------------------------------------------
# add wave -position insertpoint  \
# sim:/agu_RTR_tb/agu_RTR_inst/counter_config_IR/count \
# sim:/agu_RTR_tb/agu_RTR_inst/counter_config_IR/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/counter_config_IR/init0 \
# sim:/agu_RTR_tb/agu_RTR_inst/counter_config_OR/count \
# sim:/agu_RTR_tb/agu_RTR_inst/counter_config_OR/enable
# add wave -position insertpoint  \
# sim:/agu_RTR_tb/agu_RTR_inst/decoder_IR_option/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/decoder_IR_option/in_data \
# sim:/agu_RTR_tb/agu_RTR_inst/decoder_IR_option/out_data \
# sim:/agu_RTR_tb/agu_RTR_inst/decoder_MT_option/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/decoder_MT_option/in_data \
# sim:/agu_RTR_tb/agu_RTR_inst/decoder_MT_option/out_data \
# sim:/agu_RTR_tb/agu_RTR_inst/decoder_OR_option/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/decoder_OR_option/in_data \
# sim:/agu_RTR_tb/agu_RTR_inst/decoder_OR_option/out_data
# -----------------------------------------------

# add wave -position insertpoint  \
# sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/counter_iter_IR_inst/co \
# sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/counter_iter_IR_inst/count \
# sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/counter_iter_IR_inst/enable \
# sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/counter_iter_IR_inst/init \
# sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/counter_iter_IR_inst/init_value
# 
# add wave -position insertpoint  \
# {sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/IR_counter_iter[1]/counter_iter_IR_inst/co} \
# {sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/IR_counter_iter[1]/counter_iter_IR_inst/count} \
# {sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/IR_counter_iter[1]/counter_iter_IR_inst/enable} \
# {sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/IR_counter_iter[1]/counter_iter_IR_inst/init} \
# {sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/IR_counter_iter[1]/counter_iter_IR_inst/init_value}

add wave -position insertpoint  \
sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/counter_level_MT/count \
sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/counter_level_MT/enable \
sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/counter_level_MT/init0
add wave -position insertpoint  \
sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/counter_delay_IR_inst/co \
sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/counter_delay_IR_inst/count \
sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/counter_delay_IR_inst/enable \
sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/counter_delay_IR_inst/init \
sim:/agu_RTR_tb/agu_RTR_inst/controller_inst/counter_delay_IR_inst/init_value


run -all
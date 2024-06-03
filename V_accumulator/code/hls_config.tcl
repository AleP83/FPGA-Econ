# Put constraints on compiler
config_interface -m_axi_max_widen_bitwidth 512 
# set_param route.enableGlobalHoldIter true 
# set_param EXPLORATION TECHNIQUE for routing, recommendation Xilinx -- may be not important
# config_interface Maximize bus utilization from global (read by HOST and FPGA) to BRAM/URAM (FPGA) -- not imporant, recommended by Xilinx
# set_directive_allocation -limit 1 -type function sim_alm_coeff regression   # Avoid to allocate multiple times the same hardware for the function regression

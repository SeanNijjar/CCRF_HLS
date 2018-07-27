############################################################
## This file is generated automatically by Vivado HLS.
## Please DO NOT edit it.
## Copyright (C) 1986-2017 Xilinx, Inc. All Rights Reserved.
############################################################
open_project RunCcrf
set_top Run_CCRF
add_files ../CCRF_HLS/src/ccrf/ccrf.cpp -cflags "-std=c++11 -DZYNQ_COMPILE -DHW_COMPILE -I/home/sean_01/CCRF_HLS/CCRF_HLS/include/common/. -I/home/sean_01/CCRF_HLS/CCRF_HLS/include/common/job_descriptor/. -I/home/sean_01/CCRF_HLS/CCRF_HLS/include/scheduler/. -I/home/sean_01/CCRF_HLS/CCRF_HLS/include/ccrf"
open_solution "solution1"
set_part {xczu19eg-ffvc1760-2-i} -tool vivado
create_clock -period 10 -name default
config_interface -m_axi_addr64 -m_axi_offset off -register_io off
#source "./RunCcrf/solution1/directives.tcl"
#csim_design
csynth_design
#cosim_design
export_design -rtl verilog -format ip_catalog

############################################################
## This file is generated automatically by Vivado HLS.
## Please DO NOT edit it.
## Copyright (C) 1986-2017 Xilinx, Inc. All Rights Reserved.
############################################################
open_project CcrfSchedulerTopLevel
set_top CcrfSchedulerTopLevel
add_files ../CCRF_HLS/src/scheduler/ccrf_scheduler.cpp -cflags "-DZYNQ_COMPILE -DHW_COMPILE -I/home/sean_01/CCRF_HLS/CCRF_HLS/include/common/. -I/home/sean_01/CCRF_HLS/CCRF_HLS/include/common/job_descriptor/. -I/home/sean_01/CCRF_HLS/CCRF_HLS/include/scheduler/. -I/home/sean_01/CCRF_HLS/CCRF_HLS/include/ccrf -std=c++11"
add_files -tb ../CCRF_HLS/test/scheduler/ccrf_scheduler_top_level.cpp -cflags "-DZYNQ_COMPILE -DHW_COMPILE -I../CCRF_HLS/include/common/. -I../CCRF_HLS/include/common/job_descriptor/. -I../CCRF_HLS/include/scheduler/. -I../CCRF_HLS/include/ccrf -std=c++11"
open_solution "solution1"
set_part {xczu19eg-ffvc1760-2-i} -tool vivado
create_clock -period 10 -name default
config_interface -m_axi_addr64 -m_axi_offset off -register_io off
#source "./CcrfSchedulerTopLevel/solution1/directives.tcl"
csim_design -clean -compiler clang
csynth_design
cosim_design -compiler clang -trace_level all
export_design -rtl verilog -format ip_catalog

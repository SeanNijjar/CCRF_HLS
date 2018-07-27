############################################################
## This file is generated automatically by Vivado HLS.
## Please DO NOT edit it.
## Copyright (C) 1986-2017 Xilinx, Inc. All Rights Reserved.
############################################################
open_project CcrfSubtaskScheduler
set_top CcrfSubtaskScheduler
add_files ../CCRF_HLS/src/scheduler/ccrf_scheduler.cpp -cflags "-std=c++11 -DZYNQ_COMPILE -DHW_COMPILE -I/home/sean_01/CCRF_HLS/CCRF_HLS/include/common/. -I/home/sean_01/CCRF_HLS/CCRF_HLS/include/common/job_descriptor/. -I/home/sean_01/CCRF_HLS/CCRF_HLS/include/scheduler/. -I/home/sean_01/CCRF_HLS/CCRF_HLS/include/ccrf"
add_files -tb ../CCRF_HLS/test/scheduler/sw_test_ccrf_scheduler.cpp -cflags "-I../CCRF_HLS/include/common/. -I../CCRF_HLS/include/common/job_descriptor/. -I../CCRF_HLS/include/scheduler/. -I../CCRF_HLS/include/ccrf -DZYNQ_COMPILE -DHW_COMPILE -std=c++11"
open_solution "solution1"
set_part {xczu19eg-ffvc1760-2-i}
create_clock -period 10 -name default
#source "./CcrfSubtaskScheduler/solution1/directives.tcl"
csim_design -argv {3} -clean -compiler clang
csynth_design
cosim_design -compiler gcc
export_design -rtl verilog -format ip_catalog

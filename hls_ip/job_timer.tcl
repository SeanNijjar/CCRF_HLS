############################################################
## This file is generated automatically by Vivado HLS.
## Please DO NOT edit it.
## Copyright (C) 1986-2017 Xilinx, Inc. All Rights Reserved.
############################################################
open_project CCRF_JOB_TIMER
set_top JobTimerModule
add_files ../CCRF_HLS/src/scheduler/ccrf_scheduler.cpp -cflags "-std=c++11 -DZYNQ_COMPILE -DHW_COMPILE -I/home/sean_01/CCRF_HLS/CCRF_HLS/include/common/. -I/home/sean_01/CCRF_HLS/CCRF_HLS/include/common/job_descriptor/. -I/home/sean_01/CCRF_HLS/CCRF_HLS/include/scheduler/. -I/home/sean_01/CCRF_HLS/CCRF_HLS/include/ccrf"
add_files -tb ../CCRF_HLS/test/scheduler/job_timer_test.cpp
open_solution "solution1"
set_part {xczu19eg-ffvc1760-2-i} -tool vivado
create_clock -period 10 -name default
#source "./CCRF_JOB_TIMER/solution1/directives.tcl"
csim_design -clean -compiler clang
csynth_design
cosim_design
export_design -rtl vhdl -format ip_catalog

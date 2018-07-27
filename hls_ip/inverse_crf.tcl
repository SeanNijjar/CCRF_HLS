############################################################
## This file is generated automatically by Vivado HLS.
## Please DO NOT edit it.
## Copyright (C) 1986-2017 Xilinx, Inc. All Rights Reserved.
############################################################
open_project INVERSE_CRF_2
set_top InverseCRF
add_files ../CCRF_HLS/src/ccrf/ccrf.cpp -cflags "-DZYNQ_COMPILE -DHW_COMPILE -I../CCRF_HLS/include/common/. -I../CCRF_HLS/include/common/job_descriptor/. -I../CCRF_HLS/include/scheduler/. -I../CCRF_HLS/include/ccrf -std=c++11"
add_files -tb ../CCRF_HLS/test/scheduler/inverse_crf_test.cpp -cflags "-I../CCRF_HLS/include/common/. -I../CCRF_HLS/include/common/job_descriptor/. -I../CCRF_HLS/include/scheduler/. -I../CCRF_HLS/include/ccrf -DZYNQ_COMPILE -DHW_COMPILE -std=c++11"
open_solution "solution1"
set_part {xczu19eg-ffvc1760-2-i} -tool vivado
create_clock -period 10 -name default
config_interface -m_axi_addr64 -m_axi_offset off -register_io off
source "./INVERSE_CRF_2/solution1/directives.tcl"
csim_design -clean -compiler clang
csynth_design
cosim_design -O -compiler clang -tool xsim
export_design -rtl verilog -format ip_catalog

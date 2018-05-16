#ifndef SOFTWARE_TEST_CCRF_HPP
#define SOFTWARE_TEST_CCRF_HPP

#include "ccrf.hpp"
#include "types.hpp"
#include "helper.hpp"

#include <string>
#include <hls_stream.h>
#include <thread>
#include <iostream>

void Run_SoftwareDummyCCRF(CCRF_UNIT_STATUS_SIGNALS *status_signals, 
            hls::stream<JOB_SUBTASK> &input_subtask_queue, 
            hls::stream<uintptr_t> &output_subtask_queue);


#endif
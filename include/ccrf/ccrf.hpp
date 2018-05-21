#ifndef CCRF_H
#define CCRF_H

#include "types.hpp"
#include "helper.hpp"

#include <hls_stream.h>

struct CCRF_UNIT_STATUS_SIGNALS
{
    JOB_SUBTASK job_info;
    // uintptr_t task_dep_ptr1 = (uintptr_t)nullptr;
    // uintptr_t task_dep_ptr2 = (uintptr_t)nullptr;
    // uintptr_t task_dep_ptr3 = (uintptr_t)nullptr;
    // JOB_ID_T associated_job = 0;
    // int image_size;
    bool is_processing = false;
    bool running = false;

};

void Run_CCRF(CCRF_UNIT_STATUS_SIGNALS &status_signals, 
              hls::stream<JOB_SUBTASK> &input_subtask_queue, 
              hls::stream<uintptr_t> &output_subtask_queue
              //,BYTE_T *const memory_bus
              );




#endif
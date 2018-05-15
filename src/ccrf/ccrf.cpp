#include "ccrf.hpp"

void Run_CCRF(CCRF_UNIT_STATUS_SIGNALS &status_signals, 
              hls::stream<JOB_SUBTASK> &input_subtask_queue, 
              hls::stream<uintptr_t> &output_subtask_queue) 
{
    #pragma HLS STREAM variable=input_subtask_queue depth=1
    #pragma HLS STREAM variable=output_subtask_queue depth=1
    static JOB_SUBTASK ccrf_task_details;
    ccrf_task_details.input1 = (uintptr_t)nullptr;
    ccrf_task_details.input2 = (uintptr_t)nullptr;
    ccrf_task_details.input2 = (uintptr_t)nullptr;
    status_signals.is_processing = false;
    status_signals.running = true;
    while (1) {
        if (!input_subtask_queue.empty()) {
            ASSERT(!status_signals.is_processing, "Tried to start a new job on an already busy CCRF unit");
            ASSERT(input_subtask_queue.size() <= 1, "Multiple subtasks were pushed to a CCRF at the same time");
            status_signals.is_processing = true;
            ccrf_task_details = input_subtask_queue.read();
            status_signals.task_dep_ptr1 = ccrf_task_details.input1;
            status_signals.task_dep_ptr2 = ccrf_task_details.input2;
            status_signals.task_dep_ptr3 = ccrf_task_details.output;

            PIXEL_T *input1 = (PIXEL_T*)ccrf_task_details.input1;
            PIXEL_T *input2 = (PIXEL_T*)ccrf_task_details.input2;
            PIXEL_T *output = (PIXEL_T*)ccrf_task_details.output;
            for (int i = 0; i < ccrf_task_details.image_size; i++) {
                output[i][0] = input1[i][0] + input2[i][0];
                output[i][1] = input1[i][1] + input2[i][1];
                output[i][2] = input1[i][2] + input2[i][2];
            }

            output_subtask_queue.write(ccrf_task_details.output);
            while (!output_subtask_queue.empty());// spin until the results have been read back
            ccrf_task_details.input1 = (uintptr_t)nullptr;
            ccrf_task_details.input2 = (uintptr_t)nullptr;
            ccrf_task_details.output = (uintptr_t)nullptr;
            status_signals.task_dep_ptr1 = (uintptr_t)nullptr;
            status_signals.task_dep_ptr2 = (uintptr_t)nullptr;
            status_signals.task_dep_ptr3 = (uintptr_t)nullptr;
            status_signals.is_processing = false;
        }
    }
}
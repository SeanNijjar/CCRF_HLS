#include "ccrf.hpp"

void Run_CCRF(CCRF_UNIT_STATUS_SIGNALS &status_signals, 
              hls::stream<JOB_SUBTASK> &input_subtask_queue, 
              hls::stream<uintptr_t> &output_subtask_queue) 
{
    #pragma HLS STREAM variable=input_subtask_queue depth=1
    #pragma HLS STREAM variable=output_subtask_queue depth=1
    status_signals.is_processing = false;
    status_signals.running = true;
    while (1) {
        if (!input_subtask_queue.empty()) {
            ASSERT(!status_signals.is_processing, "Tried to start a new job on an already busy CCRF unit");
            ASSERT(input_subtask_queue.size() <= 1, "Multiple subtasks were pushed to a CCRF at the same time");
            status_signals.is_processing = true;
            status_signals.job_info = input_subtask_queue.read();


            ///////////////////////////////////////////////////////////////////
            //                  INSERT YOUR CCRF LOGIC HERE                  //
            ///////////////////////////////////////////////////////////////////
            char *input1 = (char*)(status_signals.job_info.input1);
            char *input2 = (char*)(status_signals.job_info.input2);
            char *output = (char*)(status_signals.job_info.output);
            for (int i = 0; i < status_signals.job_info.image_size; i++) {
                output[i * 3 + 0] = (BYTE_T)(((uint32_t)input1[i * 3 + 0] + (uint32_t)input2[i * 3 + 0]) / 2);
                output[i* 3 + 1] = (BYTE_T)(((uint32_t)input1[i * 3 + 1] + (uint32_t)input2[i * 3 + 1]) / 2);
                output[i* 3 + 2] = (BYTE_T)(((uint32_t)input1[i * 3 + 2] + (uint32_t)input2[i * 3 + 2]) / 2);
            }
            ///////////////////////////////////////////////////////////////////

            output_subtask_queue.write(status_signals.job_info.output);
            while (!output_subtask_queue.empty());// spin until the results have been read back
            
            status_signals.job_info.input1 = (uintptr_t)nullptr;
            status_signals.job_info.input2 = (uintptr_t)nullptr;
            status_signals.job_info.output = (uintptr_t)nullptr;
            status_signals.job_info.image_size = 0;
            status_signals.job_info.job_ID = 0;
            status_signals.is_processing = false;
        }
    }
}
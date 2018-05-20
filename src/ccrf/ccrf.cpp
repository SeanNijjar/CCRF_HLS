#include "ccrf.hpp"

void Run_CCRF(CCRF_UNIT_STATUS_SIGNALS &status_signals, 
              hls::stream<JOB_SUBTASK> &input_subtask_queue, 
              hls::stream<uintptr_t> &output_subtask_queue,
              BYTE_T *const memory_bus) 
{
    #pragma HLS INTERFACE axis port=memory_bus depth=2147483648 offset=slave
    #pragma HLS STREAM variable=input_subtask_queue depth=1
    #pragma HLS STREAM variable=output_subtask_queue depth=1
    do {
        if (!input_subtask_queue.empty()) {
            ASSERT(!status_signals.is_processing, "Tried to start a new job on an already busy CCRF unit");
            status_signals.is_processing = true;
            status_signals.job_info = input_subtask_queue.read();


            ///////////////////////////////////////////////////////////////////
            //                  INSERT YOUR CCRF LOGIC HERE                  //
            ///////////////////////////////////////////////////////////////////
            
            BYTE_T * input1 = &memory_bus[status_signals.job_info.input1];
            BYTE_T * input2 = &memory_bus[status_signals.job_info.input2];
            BYTE_T * output = &memory_bus[status_signals.job_info.output];
            #pragma HLS INTERFACE ap_memory port=input1
            #pragma HLS INTERFACE ap_memory port=input2
            #pragma HLS INTERFACE ap_memory port=output
            for (int i = 0; i < status_signals.job_info.image_size; i++) {
                output[i * 3 + 0] = ((uint32_t)input1[i * 3 + 0] + (uint32_t)input2[i * 3 + 0]) / 2;
                output[i* 3 + 1] = ((uint32_t)input1[i * 3 + 1] + (uint32_t)input2[i * 3 + 1]) / 2;
                output[i* 3 + 2] = ((uint32_t)input1[i * 3 + 2] + (uint32_t)input2[i * 3 + 2]) / 2;
            }
            
            ///////////////////////////////////////////////////////////////////

            output_subtask_queue.write(status_signals.job_info.output);
            
            status_signals.job_info.input1 = (uintptr_t)nullptr;
            status_signals.job_info.input2 = (uintptr_t)nullptr;
            status_signals.job_info.output = (uintptr_t)nullptr;
            status_signals.job_info.image_size = 0;
            status_signals.job_info.job_ID = 0;
            status_signals.is_processing = false;
            

        }
    } while(0);
}
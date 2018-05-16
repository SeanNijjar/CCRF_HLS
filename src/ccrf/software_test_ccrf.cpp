#include "software_test_ccrf.hpp"

#include "types.hpp"

#include <cstring>
#include <string>
void Run_SoftwareDummyCCRF(CCRF_UNIT_STATUS_SIGNALS &status_signals, 
            hls::stream<JOB_SUBTASK> &input_subtask_queue, 
            hls::stream<uintptr_t> &output_subtask_queue) 
{
    status_signals.is_processing = false;
    status_signals.running = true;
    while (1) {
        if (!input_subtask_queue.empty()) {
            ASSERT(!status_signals.is_processing, "Tried to start a new job on an already busy CCRF unit");
            ASSERT(input_subtask_queue.size() <= 1, "Multiple subtasks were pushed to a CCRF at the same time");
            status_signals.running = false;
            status_signals.is_processing = true;
            status_signals.job_info = input_subtask_queue.read();
            status_signals.running = true;

            ASSERT(((char*)status_signals.job_info.input1) != nullptr, "BAD CCRF SUBTASK");
            ASSERT(((char*)status_signals.job_info.input2) != nullptr, "BAD CCRF SUBTASK");
            ASSERT(((char*)status_signals.job_info.output) != nullptr, "BAD CCRF SUBTASK");


            ///////////////////////////////////////////////////////////////////
            //                  INSERT YOUR CCRF LOGIC HERE                  //
            ///////////////////////////////////////////////////////////////////
            PIXEL_T *output_image = (PIXEL_T*)status_signals.job_info.output;
            PIXEL_T *input1_image = (PIXEL_T*)status_signals.job_info.input1;
            PIXEL_T *input2_image = (PIXEL_T*)status_signals.job_info.input2;

            for (int i = 0; i < status_signals.job_info.image_size; i++) {
                (output_image[i])[0] = (input1_image[i])[0] + (input2_image[i])[0];
                (output_image[i])[1] = (input1_image[i])[1] + (input2_image[i])[1];
                (output_image[i])[2] = (input1_image[i])[2] + (input2_image[i])[2];
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

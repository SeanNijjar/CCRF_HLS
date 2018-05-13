#ifndef CCRF_H
#define CCRF_H

#include "types.hpp"
#include "helper.hpp"

#include <hls_stream.h>
class CCRF 
{
  public:
    CCRF() : is_processing(false), running(false)
    {   
        #pragma HLS STREAM variable=input_subtask_queue depth=1
        #pragma HLS STREAM variable=output_subtask_queue depth=1
    }

    const bool is_idle() 
    {
        return (!is_processing && input_subtask_queue.empty());
    }

    const PIXEL_T *const GetTaskDependence(const int i) const
    {
        assert(i > 0 && i < 3);
        
        switch(i) {
            case 0:
                return (PIXEL_T*)ccrf_task_details.input1;
            case 1:
                return (PIXEL_T*)ccrf_task_details.input2;
            case 2:
                return (PIXEL_T*)ccrf_task_details.output;
        };
        return nullptr;
    }

    virtual void run() 
    {
        #pragma HLS STREAM variable=input_subtask_queue depth=1
        #pragma HLS STREAM variable=output_subtask_queue depth=1
        is_processing = false;
        running = true;
        while (1) {
            if (!input_subtask_queue.empty()) {
                ASSERT(!is_processing, "Tried to start a new job on an already busy CCRF unit");
                ASSERT(input_subtask_queue.size() <= 1, "Multiple subtasks were pushed to a CCRF at the same time");
                is_processing = true;
                ccrf_task_details = input_subtask_queue.read();

                PIXEL_T *input1 = (PIXEL_T*)ccrf_task_details.input1;
                PIXEL_T *input2 = (PIXEL_T*)ccrf_task_details.input2;
                PIXEL_T *output = (PIXEL_T*)ccrf_task_details.output;
                for (int i = 0; i < ccrf_task_details.image_size; i++) {
                    output[i][0] = input1[i][0] + input2[i][0];
                    output[i][1] = input1[i][1] + input2[i][1];
                    output[i][2] = input1[i][2] + input2[i][2];
                    //output[i].data[0] = input1[i].data[0] + input2[i].data[0];
                    //output[i].data[1] = input1[i].data[1] + input2[i].data[1];
                    //output[i].data[2] = input1[i].data[2] + input2[i].data[2];
                    ////output[i].b = input1[i].b + input2[i].b;
                    ////output[i].g = input1[i].g + input2[i].g;
                    ////output[i].r = input1[i].r + input2[i].r;
                }

                output_subtask_queue.write(ccrf_task_details.output);
                while (!output_subtask_queue.empty());// spin until the results have been read back
                ccrf_task_details.input1 = (uintptr_t)nullptr;
                ccrf_task_details.input2 = (uintptr_t)nullptr;
                ccrf_task_details.output = (uintptr_t)nullptr;
                is_processing = false;
            }
        }
    }

  public:
    bool running;
    bool is_processing;

    hls::stream<JOB_SUBTASK> input_subtask_queue;
    hls::stream<uintptr_t> output_subtask_queue;

    JOB_SUBTASK ccrf_task_details;
};



#endif
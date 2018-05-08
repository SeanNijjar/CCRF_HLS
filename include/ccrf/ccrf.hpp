#ifndef CCRF_H
#define CCRF_H

#include "types.hpp"
#include "helper.hpp"

#include <string>
#include <hls_stream.h>
#include <thread>
#include <iostream>
class CCRF 
{
  public:
    CCRF(hls::stream<JOB_SUBTASK> &input_queue, hls::stream<PIXEL_T*> &output_queue) :
        /*input_subtask_queue(input_queue), output_subtask_queue(output_queue),*/ is_processing(false)
    {
    }

    const bool is_idle() 
    {
        return is_processing || input_subtask_queue.size() > 0;
    }

    const PIXEL_T *const GetTaskDependence(const int i) const
    {
        assert(i > 0 && i < 3);
        
        switch(i) {
            case 0:
                return ccrf_task_details.input1;
            case 1:
                return ccrf_task_details.input2;
            case 2:
                return ccrf_task_details.output;
        };
    }

    virtual void run() 
    {
        UNIMPLEMENTED();
    }

  public:
    bool is_processing;

    hls::stream<JOB_SUBTASK> input_subtask_queue;
    hls::stream<PIXEL_T*> output_subtask_queue;

    JOB_SUBTASK ccrf_task_details;
};

class SoftwareDummyCCRF 
{
  public:

    const bool is_idle() 
    {
        return (!is_processing && input_subtask_queue.size() == 0);
    }

    const PIXEL_T *const GetTaskDependence(const int i) const
    {
        assert(i >= 0 && i < 3);
        
        switch(i) {
            case 0:
                return ccrf_task_details.input1;
            case 1:
                return ccrf_task_details.input2;
            case 2:
                return ccrf_task_details.output;
        };
    }

    //SoftwareDummyCCRF(hls::stream<JOB_SUBTASK> &input_queue, hls::stream<PIXEL_T*> &output_queue) :
        //input_subtask_queue(input_queue), output_subtask_queue(output_queue)
        // : CCRF(input_queue, output_queue)
    //SoftwareDummyCCRF() : is_processing(false)
    //{
    //}

    void Initialize() 
    {
        ccrf_thread = std::thread(&SoftwareDummyCCRF::run, this);
    }

    void run() 
    {
        is_processing = false;
        while (1) {
            if (!input_subtask_queue.empty()) {
                ASSERT(!is_processing, "Tried to start a new job on an already busy CCRF unit");
                ASSERT(input_subtask_queue.size() <= 1, "Multiple subtasks were pushed to a CCRF at the same time");
                is_processing = true;
                ccrf_task_details = input_subtask_queue.read();
                std::string task_output(" (");
                task_output.append(std::string((const char*)ccrf_task_details.input1));
                task_output.append("+");
                task_output.append(std::string((const char*)ccrf_task_details.input2));
                task_output.append(") ");
                std::cout << "CCRF TASK OUTPUT: " << task_output << std::endl;
                strcpy((char *)ccrf_task_details.output, task_output.c_str());
                is_processing = false;
            }
        }
    }


    bool is_processing;

    hls::stream<JOB_SUBTASK> input_subtask_queue;
    hls::stream<PIXEL_T*> output_subtask_queue;

    JOB_SUBTASK ccrf_task_details;

  private:


    std::thread ccrf_thread;
};

#endif
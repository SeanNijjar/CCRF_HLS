#ifndef SOFTWARE_TEST_CCRF_HPP
#define SOFTWARE_TEST_CCRF_HPP

#include "ccrf.hpp"
#include "types.hpp"
#include "helper.hpp"

#include <string>
#include <hls_stream.h>
#include <thread>
#include <iostream>


class SoftwareDummyCCRF 
{
  public:


    const bool is_idle() 
    {
        return (!is_processing && input_subtask_queue.empty() && output_subtask_queue.empty());
    }

    const PIXEL_T *const GetTaskDependence(const int i) const
    {
        assert(i >= 0 && i < 3);
        
        switch(i) {
            case 0:
                return (PIXEL_T*)ccrf_task_details.input1;
            case 1:
                return (PIXEL_T*)ccrf_task_details.input2;
            case 2:
                return (PIXEL_T*)ccrf_task_details.output;
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
        running = true;
    }

    void run() 
    {
        is_processing = false;
        running = true;
        while (1) {
            if (!input_subtask_queue.empty()) {
                ASSERT(!is_processing, "Tried to start a new job on an already busy CCRF unit");
                ASSERT(input_subtask_queue.size() <= 1, "Multiple subtasks were pushed to a CCRF at the same time");
                is_processing = true;
                ccrf_task_details = input_subtask_queue.read();
                std::string task_output(" (");
                int strlen_i1 = std::string((char*)ccrf_task_details.input1).length();
                int strlen_i2 = std::string((char*)ccrf_task_details.input2).length();
                std::string error1_string = "Got 0 length string as input1 to software test CCRF: \"";
                error1_string.append((char*)ccrf_task_details.input1).append("\"").append(std::to_string((uint64_t)ccrf_task_details.input1));
                std::string error2_string = "Got 0 length string as input2 to software test CCRF: \"";
                error2_string.append((char*)ccrf_task_details.input2).append("\"").append(std::to_string((uint64_t)ccrf_task_details.input2));
                ASSERT(strlen_i1 > 1, error1_string.c_str());
                ASSERT(strlen_i2 > 1, error2_string.c_str());
                task_output.append(std::string((const char*)ccrf_task_details.input1));
                task_output.append("+");
                task_output.append(std::string((const char*)ccrf_task_details.input2));
                task_output.append(") ");
                std::string print_me = "{input1=";
                print_me += std::to_string((uint64_t)ccrf_task_details.input1) + ", input2=" + std::to_string((uint64_t)ccrf_task_details.input2) + ", output=" + std::to_string((uint64_t)ccrf_task_details.output);
                print_me.append(task_output);

                std::cout << print_me << std::endl;
                //std::this_thread::sleep_for(std::chrono::milliseconds(200));

                strcpy((char *)ccrf_task_details.output, task_output.c_str());
                output_subtask_queue.write((PIXEL_T*)ccrf_task_details.output); 
                while (!output_subtask_queue.empty());// spin until the results have been read back
                ccrf_task_details.input1 = (uintptr_t)nullptr;
                ccrf_task_details.input2 = (uintptr_t)nullptr;
                ccrf_task_details.output = (uintptr_t)nullptr;
                is_processing = false;
            }
        }
    }

    volatile bool running = false;
    volatile bool is_processing = true;

    hls::stream<JOB_SUBTASK> input_subtask_queue;
    hls::stream<PIXEL_T*> output_subtask_queue; // MIGHT have to change to stream<uintptr_t>

    JOB_SUBTASK ccrf_task_details;

  private:


    std::thread ccrf_thread;
};

#endif
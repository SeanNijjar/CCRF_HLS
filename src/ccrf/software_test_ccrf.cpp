#include "software_test_ccrf.hpp"

#include <cstring>

void Run_SoftwareDummyCCRF(CCRF_UNIT_STATUS_SIGNALS &status_signals, 
            hls::stream<JOB_SUBTASK> &input_subtask_queue, 
            hls::stream<uintptr_t> &output_subtask_queue) 
{
    static JOB_SUBTASK ccrf_task_details;
    ccrf_task_details.input1 = (uintptr_t)nullptr;
    ccrf_task_details.input2 = (uintptr_t)nullptr;
    ccrf_task_details.output = (uintptr_t)nullptr;
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

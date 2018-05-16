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

            PIXEL_T *output_image = (PIXEL_T*)status_signals.job_info.output;
            PIXEL_T *input1_image = (PIXEL_T*)status_signals.job_info.input1;
            PIXEL_T *input2_image = (PIXEL_T*)status_signals.job_info.input2;

            for (int i = 0; i < status_signals.job_info.image_size; i++) {
                (output_image[i])[0] = (input1_image[i])[0] + (input2_image[i])[0];
                (output_image[i])[1] = (input1_image[i])[1] + (input2_image[i])[1];
                (output_image[i])[2] = (input1_image[i])[2] + (input2_image[i])[2];
            }

            /*            
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
            std::cout << ((char*)ccrf_task_details.output)[0] << " ";
            std::cout << ((char*)ccrf_task_details.output)[task_output.length()] << std::endl;
            std::cout << task_output << std::endl;
            memcpy((char *)ccrf_task_details.output, task_output.c_str(), task_output.length() + 1);
            //strcpy((char *)ccrf_task_details.output, task_output.c_str());

            //std::this_thread::sleep_for(std::chrono::seconds(1000));
            */

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

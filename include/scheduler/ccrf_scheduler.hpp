#ifndef CCRF_SCHEDULER_H
#define CCRF_SCHEDULER_H

#include "types.hpp"

#define NUM_CCRF_UNITS 6
const int CCRF_COMPUTE_UNIT_COUNT = NUM_CCRF_UNITS;

const int DISPATCHER_STREAM_DEPTH = 15; // Enough to start an entire task list for a 5 LDR image stack
const int INPUT_JOB_STREAM_DEPTH = 5; // technically a size of 1 should be sufficient

// TODO: Ideally this can be passed to the FPGA
PIXEL_T *CCRF_SCRATCHPAD_START_ADDR = (PIXEL_T*)(0x10000000);

template <bool hw_implement = true>
void JobResultNotifier(hls::stream<JOB_COMPLETION_PACKET> &completed_job_queue, 
                       hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress, 
                       hls::stream<PIXEL_T*> &CCRF_completed_outputs[CCRF_COMPUTE_UNIT_COUNT])
{
    PRAGMA_HLS(HLS stream depth=DISPATCHER_STREAM_DEPTH variable=CCRF_completed_outputs)
    PRAGMA_HLS(HLS stream depth=2 variable=completed_job_queue)
    PRAGMA_HLS(HLS stream depth=1 variable=jobs_in_progress); // Force only one job allowed at a time    

    static bool job_info_valid = false;
    static JOB_COMPLETION_PACKET job_info;

    while(hw_implement) {
        while (!job_info_valid) {
            if (!jobs_in_progress.empty()) {
                job_info = jobs_in_progress.read();
                job_info_valid = true;
            }
        }

        while (job_info_valid) {
            if (!completed_job_queue.full()) {
                contiue;
            }

            bool job_completed = false;
            for (int i = 0; !job_completed && i < CCRF_COMPUTE_UNIT_COUNT; i++) {
                if (CCRF_completed_outputs[i].empty()) {
                    continue;
                }

                const PIXEL_T *const output_addr = CCRF_completed_outputs[i].read();
                const bool job_completed = (output_addr == job_info.output_address);
                if (job_completed) {
                    completed_job_queue.write(job_info);
                    job_info_valid = false;
                    job_completed = true;
                }
            }
        }

    }
}

template <bool hw_implement>
void CcrfScheduler(hls::stream<JobDescriptor::JOB_DESCRIPTOR_T> &input_jobs, 
                   hls::stream<JOB_SUBTASK> &subtask_queue, 
                   hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress) 
{
    PRAGMA_HLS(HLS stream depth=DISPATCHER_STREAM_DEPTH variable=subtask_queue)
    PRAGMA_HLS(HLS stream depth=INPUT_JOB_STREAM_DEPTH variable=input_jobs)
    PRAGMA_HLS(HLS stream depth=1 variable=jobs_in_progress); // Force only one job allowed at a time
    
    //JOB_DESCRIPTOR_T[CCRF_COMPUTE_UNIT_COUNT] active_job_descriptors;
    const int max_active_jobs = CCRF_COMPUTE_UNIT_COUNT;

    bool current_job_valid = false;
    static JobDescriptor::JOB_DESCRIPTOR_T current_job;

    while (hw_implement) {
        if (!current_job_valid && !input_jobs.empty()) {
            current_job = input_jobs.read();
            current_job_valid = true;
        }

        const int ADDR_BUFFER_SIZE = 10;
        PIXEL_T *input_addresses[ADDR_BUFFER_SIZE];
        PIXEL_T *output_addresses[ADDR_BUFFER_SIZE];

        if (current_job_valid) {
            const int image_size = current_job.IMAGE_SIZE;
            const int ldr_image_count = current_job.LDR_IMAGE_COUNT;
            for (int input = 0; input < ldr_image_count; input++) {
                input_addresses[input] = current_job.INPUT_IMAGES[input];
            }

            PIXEL_T *output_addr = CCRF_SCRATCHPAD_START_ADDR;
            for (int output = 0; output < ldr_image_count - 1; output++) {
                output_addresses[output] = output_addr + image_size;
                output_addr += image_size;
            }

            while(jobs_in_progress.full());

            for (int layer = 0; layer < ldr_image_count; layer++) {
                const int num_outputs = ldr_image_count - layer - 1;
                for (int input = 0; input < ldr_image_count - layer - 1; input++) {

                    while(subtask_queue.full()); 

                    const bool last_task = (layer == ldr_image_count - 1);
                    // The final output should go to the actual specified output location, not the scratchpad
                    const PIXEL_T *const output_addr = (last_task) ? current_job.OUTPUT_IMAGE_LOCATION : output_addresses[input];
                    subtask_queue.write( {  input_addresses[input], 
                                            input_addresses[input + 1], 
                                            output_addr, 
                                            image_size
                                        }
                                        );
                }
                std::swap(input_addresses, output_addresses);
            }
        }
    }
}


template <typename T, int ccrf_id>
int GetAvailableCCRFUnit() 
{
    constexpr if (ccrf_id >= 0) 
        return CCRF[ccrf_id].isIdle() ? ccrf_id : (GetAvailableCCRFUnit<T, ccrf_id-1>());
    else 
        return -1;
}

/* Assumes all HDR images are the same size. Doesn't perform an overlap test */
bool DoesTaskWaitForDependencies(JOB_SUBTASK task_to_check, CCRF &ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT]) {
    const int dependency_count = 3;
    typedef void* dependency_type;
    const PIXEL_T *dependencies[3] = {0,};
    dependencies[0] = task_to_check.input1;
    dependencies[1] = task_to_check.input2;
    dependencies[2] = task_to_check.output;
    for (int i = 0; i < dependency_count; i++) {
        const PIXEL_T *task_dependence = dependencies[i];
        for (int ccrf_unit = 0; ccrf_unit < CCRF_COMPUTE_UNIT_COUNT; ccrf_unit) {    
            if (ccrf_compute_units[ccrf_unit].GetTaskDependence(i) == task_dependence) {
                return true;
            }
        }
    }
    return false;
}

template <bool hw_implement = true>
void Dispatch(hls::stream<JOB_SUBTASK> &dispatcher_stream_in, CCRF ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT]) 
{
    PRAGMA_HLS(HLS stream depth=DISPATCHER_STREAM_DEPTH variable=dispatcher_stream_in)
    static JOB_SUBTASK_T task_to_add;
    static bool task_to_add_pending = false; // If we popped the task from the stream but couldn't run it last call
    
    while (hw_implement) {
        if (!task_to_add_pending && !dispatcher_stream_in.empty()) {
            task_to_add = dispatcher_in_stream.read();
            task_to_add_pending = true;
        }

        if (task_to_add_pending) {
            bool dependence_is_processing = DoesTaskWaitForDependencies(task_to_check);
            int available_ccrf_unit = GetAvailableCCRFUnit<int, CCRF_COMPUTE_UNIT_COUNT>();
            if (!dependence_is_processing && available_ccrf_unit > 0) {
                ccrf_compute_units[available_ccrf_unit].run(task_to_add.input_width, task_to_add.input_height, task_to_add.input1, task_to_add.input2, task_to_add.output);
                task_to_add_pending = false;
            }
        }
    }
}

#endif
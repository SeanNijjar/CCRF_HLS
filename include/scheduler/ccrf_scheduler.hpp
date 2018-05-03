#ifndef CCRF_SCHEDULER_H
#define CCRF_SCHEDULER_H

#include "types.hpp"
#include "helper.hpp"
#include "job_descriptor.hpp"
#include "ccrf.hpp"
#include <hls_stream.h>

#define NUM_CCRF_UNITS 6
const int CCRF_COMPUTE_UNIT_COUNT = NUM_CCRF_UNITS;

const int DISPATCHER_STREAM_DEPTH = 15; // Enough to start an entire task list for a 5 LDR image stack
const int INPUT_JOB_STREAM_DEPTH = 5; // technically a size of 1 should be sufficient
const int RESPONSE_QUEUE_DEPTH = 32;
const int COMPLETED_JOBS_QUEUE_DEPTH = 16;
static_assert(RESPONSE_QUEUE_DEPTH >= COMPLETED_JOBS_QUEUE_DEPTH, "RESPONSE_QUEUE_DEPTH must be >= COMPLETED_JOBS_QUEUE_DEPTH");
const int JOBS_TO_SCHEDULE_QUEUE_DEPTH = 16;
// TODO: Ideally this can be passed to the FPGA
extern PIXEL_T *CCRF_SCRATCHPAD_START_ADDR;

template <bool hw_implement>
void CCRF_Scheduler(hls::stream<JobPackage> incoming_job_requests, 
                    hls::stream<JOB_STATUS_MESSAGE> response_message_queue,
                    hls::stream<JobDescriptor::JOB_DESCRIPTOR_T> jobs_to_schedule_queue,
                    hls::stream<JOB_COMPLETION_PACKET> completed_jobs_queue)
{
    DO_PRAGMA(HLS stream depth=RESPONSE_QUEUE_DEPTH variable=response_message_queue)
    DO_PRAGMA(HLS stream depth=RESPONSE_QUEUE_DEPTH variable=job_requests)
    DO_PRAGMA(HLS stream depth=JOBS_TO_SCHEDULE_QUEUE_DEPTH variable=jobs_to_schedule_queue)
    DO_PRAGMA(HLS stream depth=COMPLETED_JOBS_QUEUE_DEPTH variable=completed_jobs_queue)

    do {
        while(!completed_jobs.empty()) {
            JOB_COMPLETION_PACKET completed_job;
            JOB_STATUS_MESSAGE completion_packet_for_host;
            completion_packet_for_host.packet_message_type = JOB_STATUS_MESSAGE::JOB_DONE_PACKET;
            completion_packet_for_host.job_ID = completed_job.job_ID;
            ASSERT(!response_message_queue.full(), "response message queue is full");
            response_message_queue.write(completion_packet_for_host);
        }

        if (!incoming_job_requests.empty()) {
            JOB_STATUS_MESSAGE response_packet;
            JobPackage job_request = incoming_job_requests.read();
            response_packet.packet_message_type = (jobs_to_schedule.full()) ? JOB_STATUS_MESSAGE::JOB_REJECT_PACKET : JOB_STATUS_MESSAGE::JOB_ACCEPT_PACKET;
            response_packet.job_ID = job_request.job_ID;
            response_message_queue.write(reponse_packet);
        }
    } while (hw_implement);
}

template <bool hw_implement = true>
void JobResultNotifier(hls::stream<JOB_COMPLETION_PACKET> &completed_job_queue, 
                       hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress, 
                       hls::stream<PIXEL_T*> CCRF_completed_outputs[CCRF_COMPUTE_UNIT_COUNT])
{
    DO_PRAGMA(HLS stream depth=DISPATCHER_STREAM_DEPTH variable=CCRF_completed_outputs)
    DO_PRAGMA(HLS stream depth=COMPLETED_JOBS_QUEUE_DEPTH variable=completed_job_queue)
    DO_PRAGMA(HLS stream depth=JOBS_TO_SCHEDULE_QUEUE_DEPTH variable=jobs_in_progress); // Force only one job allowed at a time    

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
                continue;
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
    DO_PRAGMA(HLS stream depth=DISPATCHER_STREAM_DEPTH variable=subtask_queue)
    DO_PRAGMA(HLS stream depth=INPUT_JOB_STREAM_DEPTH variable=input_jobs)
    DO_PRAGMA(HLS stream depth=1 variable=jobs_in_progress); // Force only one job allowed at a time
    
    //JOB_DESCRIPTOR_T[CCRF_COMPUTE_UNIT_COUNT] active_job_descriptors;
    const int max_active_jobs = CCRF_COMPUTE_UNIT_COUNT;

    bool current_job_valid = false;
    static JobDescriptor::JOB_DESCRIPTOR_T current_job;

    do {
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
            jobs_in_progress.write({current_job.OUTPUT_IMAGE_LOCATION, current_job.IMAGE_SIZE});
            for (int layer = 0; layer < ldr_image_count; layer++) {
                const int num_outputs = ldr_image_count - layer - 1;
                for (int input = 0; input < ldr_image_count - layer - 1; input++) {

                    while(subtask_queue.full()); 

                    const bool last_task = (layer == ldr_image_count - 1);
                    // The final output should go to the actual specified output location, not the scratchpad
                    PIXEL_T *output_addr = (last_task) ? current_job.OUTPUT_IMAGE_LOCATION : output_addresses[input];
                    JOB_SUBTASK new_subtask;
                    new_subtask.Initialize(input_addresses[input], input_addresses[input + 1], output_addr, image_size);
                    subtask_queue.write(new_subtask);
                }
                std::swap(input_addresses, output_addresses);
            }

            current_job_valid = false;
        }
    } while(hw_implement);
}


template <typename T, int ccrf_id>
int GetAvailableCCRFUnit(CCRF ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT]) 
{
    if (ccrf_id >= 0) 
        return ccrf_compute_units[ccrf_id].is_idle() ? ccrf_id : (GetAvailableCCRFUnit<T, ccrf_id-1>());
    else 
        return -1;
}

/* Assumes all HDR images are the same size. Doesn't perform an overlap test */
bool DoesTaskWaitForDependencies(JOB_SUBTASK task_to_check, CCRF ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT]);

template <bool hw_implement = true>
void Dispatch(hls::stream<JOB_SUBTASK> &dispatcher_stream_in, CCRF ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT]) 
{
    DO_PRAGMA(HLS stream depth=DISPATCHER_STREAM_DEPTH variable=dispatcher_stream_in)
    static JOB_SUBTASK task_to_add;
    static bool task_to_add_pending = false; // If we popped the task from the stream but couldn't run it last call
    
    while (hw_implement) {
        if (!task_to_add_pending && !dispatcher_stream_in.empty()) {
            task_to_add = dispatcher_stream_in.read();
            task_to_add_pending = true;
        }

        if (task_to_add_pending) {
            bool dependence_is_processing = DoesTaskWaitForDependencies(task_to_add, ccrf_compute_units);
            int available_ccrf_unit = GetAvailableCCRFUnit<int, CCRF_COMPUTE_UNIT_COUNT>(ccrf_compute_units);
            if (!dependence_is_processing && available_ccrf_unit > 0) {
                ccrf_compute_units[available_ccrf_unit].run(task_to_add);
                task_to_add_pending = false;
            }
        }
    }
}

#endif
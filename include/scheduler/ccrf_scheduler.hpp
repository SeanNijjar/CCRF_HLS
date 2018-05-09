#ifndef CCRF_SCHEDULER_H
#define CCRF_SCHEDULER_H

#include "types.hpp"
#include "helper.hpp"
#include "job_descriptor.hpp"
#include "job_package.hpp"
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
void CcrfSchedulerTopLevel(hls::stream<JobPackage> *incoming_job_requests, 
                           hls::stream<JOB_STATUS_MESSAGE> *response_message_queue,
                           hls::stream<JobPackage> *jobs_to_schedule_queue,
                           hls::stream<JOB_COMPLETION_PACKET> *completed_jobs_queue)
{
    DO_PRAGMA(HLS stream depth=RESPONSE_QUEUE_DEPTH variable=response_message_queue)
    DO_PRAGMA(HLS stream depth=RESPONSE_QUEUE_DEPTH variable=job_requests)
    DO_PRAGMA(HLS stream depth=JOBS_TO_SCHEDULE_QUEUE_DEPTH variable=jobs_to_schedule_queue)
    DO_PRAGMA(HLS stream depth=COMPLETED_JOBS_QUEUE_DEPTH variable=completed_jobs_queue)

    do {
        while(!completed_jobs_queue->empty()) {
            JOB_COMPLETION_PACKET completed_job = completed_jobs_queue->read();
            JOB_STATUS_MESSAGE completion_packet_for_host;
            completion_packet_for_host.packet_message_type = JOB_STATUS_MESSAGE::JOB_DONE_PACKET;
            completion_packet_for_host.job_ID = completed_job.job_ID;
            ASSERT(!response_message_queue->full(), "response message queue is full");
            response_message_queue->write(completion_packet_for_host);
        }

        if (!incoming_job_requests->empty()) {
            JOB_STATUS_MESSAGE response_packet;
            JobPackage job_request = incoming_job_requests->read();
            response_packet.packet_message_type = (jobs_to_schedule_queue->full()) ? JOB_STATUS_MESSAGE::JOB_REJECT_PACKET : JOB_STATUS_MESSAGE::JOB_ACCEPT_PACKET;
            response_packet.job_ID = job_request.job_ID;
            response_message_queue->write(response_packet);
            if (!jobs_to_schedule_queue->full()) {
                jobs_to_schedule_queue->write(job_request);
            }
        }
    } while (hw_implement);
}

template <bool hw_implement = true>
void JobResultNotifier(hls::stream<JOB_COMPLETION_PACKET> *completed_job_queue, 
                       hls::stream<JOB_COMPLETION_PACKET> *jobs_in_progress, 
                       SoftwareDummyCCRF ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT])
                       //hls::stream<PIXEL_T*> *CCRF_completed_outputs[CCRF_COMPUTE_UNIT_COUNT])
{
    //DO_PRAGMA(HLS stream depth=DISPATCHER_STREAM_DEPTH variable=CCRF_completed_outputs)
    DO_PRAGMA(HLS stream depth=COMPLETED_JOBS_QUEUE_DEPTH variable=completed_job_queue)
    DO_PRAGMA(HLS stream depth=JOBS_TO_SCHEDULE_QUEUE_DEPTH variable=jobs_in_progress); // Force only one job allowed at a time    

    static bool job_info_valid = false;
    static JOB_COMPLETION_PACKET job_info;

    do {
        while (!job_info_valid) {
            if (!jobs_in_progress->empty()) {
                job_info = jobs_in_progress->read();
                job_info_valid = true;
            }
        }

        while (job_info_valid) {
            if (completed_job_queue->full()) {
                continue;
            }

            bool job_completed = false;
            for (int i = 0; !job_completed && i < CCRF_COMPUTE_UNIT_COUNT; i++) {
                //if (CCRF_completed_outputs[i]->empty()) {
                if (!ccrf_compute_units[i].running || ccrf_compute_units[i].output_subtask_queue.empty()) {
                    continue;
                }

                //const PIXEL_T *const output_addr = CCRF_completed_outputs[i]->read();
                PIXEL_T * output_addr = ccrf_compute_units[i].output_subtask_queue.read();
                job_completed = (output_addr == job_info.output_address);
                if (job_completed) {
                    completed_job_queue->write(job_info);
                    job_info_valid = false;
                    job_completed = true;
                }
            }
        }

    } while (hw_implement);
}

template <bool hw_implement>
void CcrfSubtaskScheduler(hls::stream<JobPackage> *input_jobs, 
                          hls::stream<JOB_SUBTASK> *subtask_queue, 
                          hls::stream<JOB_COMPLETION_PACKET> *jobs_in_progress) 
{
    DO_PRAGMA(HLS stream depth=DISPATCHER_STREAM_DEPTH variable=subtask_queue)
    DO_PRAGMA(HLS stream depth=INPUT_JOB_STREAM_DEPTH variable=input_jobs)
    DO_PRAGMA(HLS stream depth=1 variable=jobs_in_progress); // Force only one job allowed at a time
    
    //JOB_DESCRIPTOR_T[CCRF_COMPUTE_UNIT_COUNT] active_job_descriptors;
    const int max_active_jobs = CCRF_COMPUTE_UNIT_COUNT;

    bool current_job_valid = false;
    static JobDescriptor current_job;
    static JOB_ID_T current_job_ID;

    do {
        if (!current_job_valid && !input_jobs->empty()) {
            JobPackage current_job_package = input_jobs->read();
            current_job = current_job_package.job_descriptor;
            current_job_ID = current_job_package.job_ID;
            current_job_valid = true;
        }

        const int ADDR_BUFFER_SIZE = 10;
        PIXEL_T *input_addresses[ADDR_BUFFER_SIZE];
        PIXEL_T *output_addresses[ADDR_BUFFER_SIZE];

        if (current_job_valid) {
            const int image_size = current_job.IMAGE_SIZE();
            const int ldr_image_count = current_job.LDR_IMAGE_COUNT;
            for (int input = 0; input < ldr_image_count; input++) {
                input_addresses[input] = current_job.INPUT_IMAGES[input];
            }

            PIXEL_T *output_addr = CCRF_SCRATCHPAD_START_ADDR;
            for (int output = 0; output < ldr_image_count - 1; output++) {
                output_addresses[output] = output_addr + image_size;
                output_addr += image_size;
            }

            while(jobs_in_progress->full());
            JOB_COMPLETION_PACKET job_completion_packet = {current_job_ID, current_job.OUTPUT_IMAGE_LOCATION, current_job.IMAGE_SIZE()};
            jobs_in_progress->write(job_completion_packet);
            for (int layer = 0; layer < ldr_image_count - 1; layer++) {
                const int num_outputs = ldr_image_count - layer - 1;
                for (int input = 0; input < ldr_image_count - layer - 1; input++) {

                    while(subtask_queue->full()); 

                    const bool last_task = (num_outputs == 1);
                    // The final output should go to the actual specified output location, not the scratchpad
                    PIXEL_T *output_addr = (last_task) ? current_job.OUTPUT_IMAGE_LOCATION : output_addresses[input];
                    JOB_SUBTASK new_subtask;
                    new_subtask.Initialize(input_addresses[input], input_addresses[input + 1], output_addr, image_size);
                    subtask_queue->write(new_subtask);
                }
                std::swap(input_addresses, output_addresses);
            }

            current_job_valid = false;
        }
    } while(hw_implement);
}


template <typename CCRF_TYPE>
int GetAvailableCCRFUnit(CCRF_TYPE ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT]) 
{
    for (int i = 0; i < CCRF_COMPUTE_UNIT_COUNT; i++) {
        if (ccrf_compute_units[i].is_idle()) {
            return i;
        }
    }
    return -1;
}

/* Assumes all HDR images are the same size. Doesn't perform an overlap test */
template <typename CCRF_TYPE>
bool DoesTaskWaitForDependencies(JOB_SUBTASK task_to_check, 
                                 CCRF_TYPE ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT]) {
    const int dependency_count = 3;
    typedef void* dependency_type;
    const PIXEL_T *dependencies[3] = {0,};
    dependencies[0] = task_to_check.input1;
    dependencies[1] = task_to_check.input2;
    dependencies[2] = task_to_check.output;
    for (int i = 0; i < dependency_count; i++) {
        const PIXEL_T *task_dependence = dependencies[i];
        for (int ccrf_unit = 0; ccrf_unit < CCRF_COMPUTE_UNIT_COUNT; ccrf_unit++) {
            if (!ccrf_compute_units[ccrf_unit].running || ccrf_compute_units[ccrf_unit].is_idle()) {
                // neither of these two cases can possibly contribute to a dependence match
                continue;
            }
            if (i == 2) {
                // For inputs, check for dependence against outputs, so we wait for the result
                // to be available
                if (ccrf_compute_units[ccrf_unit].GetTaskDependence(0) == task_dependence ||
                    ccrf_compute_units[ccrf_unit].GetTaskDependence(1) == task_dependence) {
                    return true;
                }
            } else {
                // for outputs, make sure current inputs don't wait for this location.
                // so as not to overwrite a current location that is used as input for
                // another subtask
                if (ccrf_compute_units[ccrf_unit].GetTaskDependence(2) == task_dependence) {
                    return true;
                }
            }
        }
    }
    return false;
}


template <bool hw_implement = true, typename CCRF_TYPE>
void CcrfSubtaskDispatcher(hls::stream<JOB_SUBTASK> *dispatcher_stream_in, 
                           CCRF_TYPE ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT]) 
{
    DO_PRAGMA(HLS stream depth=DISPATCHER_STREAM_DEPTH variable=dispatcher_stream_in)
    static JOB_SUBTASK task_to_add;
    static bool task_to_add_pending = false; // If we popped the task from the stream but couldn't run it last call
    
    while (hw_implement) {
        if (!task_to_add_pending && !dispatcher_stream_in->empty()) {
            task_to_add = dispatcher_stream_in->read();
            task_to_add_pending = true;
        }

        if (task_to_add_pending) {
            bool dependence_is_processing = DoesTaskWaitForDependencies(task_to_add, ccrf_compute_units);
            if (dependence_is_processing) {
                continue;
            }
            int available_ccrf_unit = GetAvailableCCRFUnit(ccrf_compute_units);
            if (available_ccrf_unit >= 0) {
                ccrf_compute_units[available_ccrf_unit].input_subtask_queue.write(task_to_add);
                task_to_add_pending = false;
                task_to_add.input1 = nullptr;
                task_to_add.input2 = nullptr;
                task_to_add.output = nullptr;
                task_to_add.image_size = 0;
            }
        }
    }
}

#endif
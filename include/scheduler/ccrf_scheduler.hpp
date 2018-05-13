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


void CcrfSchedulerTopLevel(hls::stream<JobPackage> &incoming_job_requests, 
                           hls::stream<JOB_STATUS_MESSAGE> &response_message_queue,
                           hls::stream<JobPackage> &jobs_to_schedule_queue,
                           hls::stream<JOB_COMPLETION_PACKET> &completed_jobs_queue);


template <typename CCRF_TYPE>
void JobResultNotifier(hls::stream<JOB_COMPLETION_PACKET> &completed_job_queue, 
                       hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress, 
                       CCRF_TYPE ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT])
                       //hls::stream<PIXEL_T*> *CCRF_completed_outputs[CCRF_COMPUTE_UNIT_COUNT])
{
    //DO_PRAGMA(HLS stream depth=DISPATCHER_STREAM_DEPTH variable=CCRF_completed_outputs)
    DO_PRAGMA(HLS stream depth=COMPLETED_JOBS_QUEUE_DEPTH variable=completed_job_queue)
    DO_PRAGMA(HLS stream depth=JOBS_TO_SCHEDULE_QUEUE_DEPTH variable=jobs_in_progress); // Force only one job allowed at a time    

    static bool job_info_valid = false;
    static JOB_COMPLETION_PACKET job_info;

    do {
        while (!job_info_valid) {
            if (!jobs_in_progress.empty()) {
                JOB_COMPLETION_PACKET tmp_job_info = jobs_in_progress.read();
                job_info = tmp_job_info;
                job_info_valid = true;
            }
        }

        while (job_info_valid) {
            if (completed_job_queue.full()) {
                continue;
            }

            bool job_completed = false;
            for (int i = 0; !job_completed && i < CCRF_COMPUTE_UNIT_COUNT; i++) {
                //if (CCRF_completed_outputs[i]->empty()) {
                if (!ccrf_compute_units[i].running || ccrf_compute_units[i].output_subtask_queue.empty()) {
                    continue;
                }

                //const PIXEL_T *const output_addr = CCRF_completed_outputs[i]->read();
                PIXEL_T * output_addr = (PIXEL_T*)ccrf_compute_units[i].output_subtask_queue.read();
                job_completed = (output_addr == job_info.output_address);
                if (job_completed) {
                    completed_job_queue.write(job_info);
                    job_info_valid = false;
                    job_completed = true;
                }
            }
        }

    } while (1);
}

void JobResultsNotifier_StaticWrapper(hls::stream<JOB_COMPLETION_PACKET> &completed_job_queue, 
                       hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress,
                       CCRF ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT]);

void CcrfSubtaskScheduler(hls::stream<JobPackage> &input_jobs, 
                          hls::stream<JOB_SUBTASK> &subtask_queue, 
                          hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress);

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
    //const uintptr_t dependencies[3] = {task_to_check.input1, task_to_check.input2, task_to_check.output};
    uintptr_t dependencies[3]; //= {task_to_check.input1, task_to_check.input2, task_to_check.output};
     dependencies[0] = task_to_check.input1;
     dependencies[1] = task_to_check.input2;
     dependencies[2] = task_to_check.output;
    for (int i = 0; i < dependency_count; i++) {
        const PIXEL_T *const task_dependence = (PIXEL_T*)dependencies[i];
        for (int ccrf_unit = 0; ccrf_unit < CCRF_COMPUTE_UNIT_COUNT; ccrf_unit++) {
            if (!ccrf_compute_units[ccrf_unit].running || ccrf_compute_units[ccrf_unit].is_idle()) {
                // neither of these two cases can possibly contribute to a dependence match
                continue;
            }
            if (i == 2) {
                // For inputs, check for dependence against outputs, so we wait for the result
                // to be available
                if ((uintptr_t)ccrf_compute_units[ccrf_unit].GetTaskDependence(0) == (uintptr_t)task_dependence ||
                    (uintptr_t)ccrf_compute_units[ccrf_unit].GetTaskDependence(1) == (uintptr_t)task_dependence) {
                    return true;
                }
            } else {
                // for outputs, make sure current inputs don't wait for this location.
                // so as not to overwrite a current location that is used as input for
                // another subtask
                if ((uintptr_t)ccrf_compute_units[ccrf_unit].GetTaskDependence(2) == (uintptr_t)task_dependence) {
                    return true;
                }
            }
        }
    }
    return false;
}


template <typename CCRF_TYPE>
void CcrfSubtaskDispatcher(hls::stream<JOB_SUBTASK> &dispatcher_stream_in, 
                           CCRF_TYPE ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT]) 
{
    DO_PRAGMA(HLS stream depth=DISPATCHER_STREAM_DEPTH variable=dispatcher_stream_in)
    static JOB_SUBTASK task_to_add;
    static bool task_to_add_pending = false; // If we popped the task from the stream but couldn't run it last call
    
    while (1) {
        if (!task_to_add_pending && !dispatcher_stream_in.empty()) {
            task_to_add = dispatcher_stream_in.read();
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
                task_to_add.input1 = (uintptr_t)nullptr;
                task_to_add.input2 = (uintptr_t)nullptr;
                task_to_add.output = (uintptr_t)nullptr;
                task_to_add.image_size = 0;
            }
        }
    }
}


void CcrfSubtaskDispatcher_StaticWrapper(hls::stream<JOB_SUBTASK> &dispatcher_stream_in, 
                                         CCRF ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT]);

#ifdef HW_COMPILE
void CcrfWrapper(hls::stream<JobPackage> incoming_job_requests, 
                 hls::stream<JOB_STATUS_MESSAGE> response_message_queue);
#endif

#endif
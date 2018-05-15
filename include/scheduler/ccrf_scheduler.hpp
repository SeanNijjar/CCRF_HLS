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

template <typename STREAM_CLASS>
bool CcrfQueuesBusy(int queue_id, 
#ifdef HW_COMPILE
    hls::stream<STREAM_CLASS> &subtask_to_ccrf_queue_1,
    hls::stream<STREAM_CLASS> &subtask_to_ccrf_queue_2,
    hls::stream<STREAM_CLASS> &subtask_to_ccrf_queue_3,
    hls::stream<STREAM_CLASS> &subtask_to_ccrf_queue_4,
    hls::stream<STREAM_CLASS> &subtask_to_ccrf_queue_5,
    hls::stream<STREAM_CLASS> &subtask_to_ccrf_queue_6
#else
    hls::stream<STREAM_CLASS> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
#endif
)
{
    bool queue_busy = true;
    #ifdef HW_COMPILE
    switch (queue_id) {
        case 0: queue_busy = subtask_to_ccrf_queue_1.full(); break;
        case 1: queue_busy = subtask_to_ccrf_queue_2.full(); break;
        case 2: queue_busy = subtask_to_ccrf_queue_3.full(); break;
        case 3: queue_busy = subtask_to_ccrf_queue_4.full(); break;
        case 4: queue_busy = subtask_to_ccrf_queue_5.full(); break;
        case 5: queue_busy = subtask_to_ccrf_queue_6.full(); break;
        default: assert(false); queue_busy = true; break;
    };
    #else 
    queue_busy = !subtask_to_ccrf_queues[queue_id].empty();
    #endif
    return queue_busy;
}

void CcrfSchedulerTopLevel(hls::stream<JobPackage> &incoming_job_requests, 
                           hls::stream<JOB_STATUS_MESSAGE> &response_message_queue,
                           hls::stream<JobPackage> &jobs_to_schedule_queue,
                           hls::stream<JOB_COMPLETION_PACKET> &completed_jobs_queue);

void JobResultNotifier(hls::stream<JOB_COMPLETION_PACKET> &completed_job_queue, 
                       hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress, 
                       volatile CCRF_UNIT_STATUS_SIGNALS ccrf_status_signals[CCRF_COMPUTE_UNIT_COUNT],
#ifdef HW_COMPILE
                       hls::stream<uintptr_t> &completed_subtasks_queue_1,
                       hls::stream<uintptr_t> &completed_subtasks_queue_2,
                       hls::stream<uintptr_t> &completed_subtasks_queue_3,
                       hls::stream<uintptr_t> &completed_subtasks_queue_4,
                       hls::stream<uintptr_t> &completed_subtasks_queue_5,
                       hls::stream<uintptr_t> &completed_subtasks_queue_6
#else
                       hls::stream<uintptr_t> completed_queues_from_ccrf_units[CCRF_COMPUTE_UNIT_COUNT]
#endif
);

void JobResultsNotifier_StaticWrapper(hls::stream<JOB_COMPLETION_PACKET> &completed_job_queue, 
                       hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress,
                       volatile CCRF_UNIT_STATUS_SIGNALS ccrf_status_signals[CCRF_COMPUTE_UNIT_COUNT],
#ifdef HW_COMPILE
                       hls::stream<uintptr_t> &completed_subtasks_queue_1,
                       hls::stream<uintptr_t> &completed_subtasks_queue_2,
                       hls::stream<uintptr_t> &completed_subtasks_queue_3,
                       hls::stream<uintptr_t> &completed_subtasks_queue_4,
                       hls::stream<uintptr_t> &completed_subtasks_queue_5,
                       hls::stream<uintptr_t> &completed_subtasks_queue_6
#else
                       hls::stream<uintptr_t> completed_subtasks_queues[CCRF_COMPUTE_UNIT_COUNT]
#endif
);

void CcrfSubtaskScheduler(hls::stream<JobPackage> &input_jobs, 
                          hls::stream<JOB_SUBTASK> &subtask_queue, 
                          hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress);

int GetAvailableCCRFUnit(volatile CCRF_UNIT_STATUS_SIGNALS ccrf_status_signals[CCRF_COMPUTE_UNIT_COUNT],
#ifdef HW_COMPILE
                         hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_1,
                         hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_2,
                         hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_3,
                         hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_4,
                         hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_5,
                         hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_6
#else
                         hls::stream<JOB_SUBTASK> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
#endif
);


/* Assumes all HDR images are the same size. Doesn't perform an overlap test */
bool DoesTaskWaitForDependencies(JOB_SUBTASK task_to_check, 
                                 volatile CCRF_UNIT_STATUS_SIGNALS ccrf_status_signals[CCRF_COMPUTE_UNIT_COUNT],
#ifdef HW_COMPILE
                                 hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_1,
                                 hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_2,
                                 hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_3,
                                 hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_4,
                                 hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_5,
                                 hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_6
#else
                                 hls::stream<JOB_SUBTASK> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
#endif
);

void CcrfSubtaskDispatcher(hls::stream<JOB_SUBTASK> &dispatcher_stream_in, 
                           volatile CCRF_UNIT_STATUS_SIGNALS ccrf_status_signals[CCRF_COMPUTE_UNIT_COUNT],
#ifdef HW_COMPILE
                           hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_1,
                           hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_2,
                           hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_3,
                           hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_4,
                           hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_5,
                           hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_6
#else
                           hls::stream<JOB_SUBTASK> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
#endif
);


void CcrfSubtaskDispatcher_StaticWrapper(hls::stream<JOB_SUBTASK> &dispatcher_stream_in,
                                         CCRF_UNIT_STATUS_SIGNALS ccrf_status_signals[CCRF_COMPUTE_UNIT_COUNT],
#ifdef HW_COMPILE
                                         hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_1,
                                         hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_2,
                                         hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_3,
                                         hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_4,
                                         hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_5,
                                         hls::stream<JOB_SUBTASK> &subtask_to_ccrf_queue_6
#else
                                         hls::stream<JOB_SUBTASK> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
#endif
);

#ifdef HW_COMPILE
void CcrfWrapper(hls::stream<JobPackage> incoming_job_requests, 
                 hls::stream<JOB_STATUS_MESSAGE> response_message_queue);
#endif

#endif
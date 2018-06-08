#ifndef CCRF_SCHEDULER_H
#define CCRF_SCHEDULER_H

#include "types.hpp"
#include "helper.hpp"
#include "job_descriptor.hpp"
#include "job_package.hpp"
#include "ccrf.hpp"
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#define NUM_CCRF_UNITS 6
//const int CCRF_COMPUTE_UNIT_COUNT = NUM_CCRF_UNITS;
#define CCRF_COMPUTE_UNIT_COUNT NUM_CCRF_UNITS

const int DISPATCHER_STREAM_DEPTH = 15; // Enough to start an entire task list for a 5 LDR image stack
const int INPUT_JOB_STREAM_DEPTH = 5; // technically a size of 1 should be sufficient
const int RESPONSE_QUEUE_DEPTH = 32;
const int COMPLETED_JOBS_QUEUE_DEPTH = 16;
static_assert(RESPONSE_QUEUE_DEPTH >= COMPLETED_JOBS_QUEUE_DEPTH, "RESPONSE_QUEUE_DEPTH must be >= COMPLETED_JOBS_QUEUE_DEPTH");
const int JOBS_TO_SCHEDULE_QUEUE_DEPTH = 16;

typedef ap_axis<32, 1, 1, 1> JOB_STATUS_MESSAGE_AXI;
typedef ap_axis<496, 1, 1, 1> JOB_PACKAGE_AXI;

template <typename STREAM_CLASS>
bool CcrfQueuesBusy(int queue_id, 
    hls::stream<STREAM_CLASS> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
)
{
    #pragma HLS INLINE
    bool queue_busy = !subtask_to_ccrf_queues[queue_id].empty();
    return queue_busy;
}

void CcrfSchedulerTopLevel(hls::stream<JobPackage> &incoming_job_requests, 
                           hls::stream<JOB_STATUS_MESSAGE> &response_message_queue,
                           hls::stream<JobPackage> &jobs_to_schedule_queue,
                           hls::stream<JOB_COMPLETION_PACKET> &completed_jobs_queue);

void JobResultNotifier(hls::stream<JOB_COMPLETION_PACKET> &completed_job_queue, 
                       hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress, 
                       CCRF_UNIT_STATUS_SIGNALS ccrf_status_signals[CCRF_COMPUTE_UNIT_COUNT],
                       hls::stream<uintptr_t> completed_queues_from_ccrf_units[CCRF_COMPUTE_UNIT_COUNT]
);

void JobResultsNotifier_StaticWrapper(hls::stream<JOB_COMPLETION_PACKET> &completed_job_queue, 
                       hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress,
                       CCRF_UNIT_STATUS_SIGNALS ccrf_status_signals[CCRF_COMPUTE_UNIT_COUNT],
                       hls::stream<uintptr_t> completed_subtasks_queues[CCRF_COMPUTE_UNIT_COUNT]
);

void CcrfSubtaskScheduler(hls::stream<JobPackage> &input_jobs, 
                          hls::stream<JOB_SUBTASK> &subtask_queue, 
                          hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress);

int GetAvailableCCRFUnit(CCRF_UNIT_STATUS_SIGNALS ccrf_status_signals[CCRF_COMPUTE_UNIT_COUNT],
                         hls::stream<JOB_SUBTASK> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
);


/* Assumes all HDR images are the same size. Doesn't perform an overlap test */
bool DoesTaskWaitForDependencies(JOB_SUBTASK task_to_check, 
                                 CCRF_UNIT_STATUS_SIGNALS ccrf_status_signals[CCRF_COMPUTE_UNIT_COUNT],
                                 hls::stream<JOB_SUBTASK> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
);

void CcrfSubtaskDispatcher(hls::stream<JOB_SUBTASK> &dispatcher_stream_in, 
                           CCRF_UNIT_STATUS_SIGNALS ccrf_status_signals[CCRF_COMPUTE_UNIT_COUNT],
                           hls::stream<JOB_SUBTASK> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
);


void CcrfSubtaskDispatcher_StaticWrapper(hls::stream<JOB_SUBTASK> &dispatcher_stream_in,
                                         CCRF_UNIT_STATUS_SIGNALS ccrf_status_signals[CCRF_COMPUTE_UNIT_COUNT],
                                         hls::stream<JOB_SUBTASK> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
);

void CcrfWrapper(hls::stream<JOB_PACKAGE_AXI> &incoming_job_requests, 
                 hls::stream<JOB_STATUS_MESSAGE_AXI> &response_message_queue
                 //,BYTE_T *const memory_bus
                 );

#endif
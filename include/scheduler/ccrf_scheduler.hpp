#ifndef CCRF_SCHEDULER_H
#define CCRF_SCHEDULER_H

#include "types.hpp"
#include "helper.hpp"
 #include "job_descriptor.hpp"
 #include "job_package.hpp"
 #include "ccrf.hpp"
 #include <hls_stream.h>
#include <ap_axi_sdata.h>

#define NUM_CCRF_UNITS 1
//const int CCRF_COMPUTE_UNIT_COUNT = NUM_CCRF_UNITS;
#define CCRF_COMPUTE_UNIT_COUNT NUM_CCRF_UNITS

const int DISPATCHER_STREAM_DEPTH = 15; // Enough to start an entire task list for a 5 LDR image stack
const int INPUT_JOB_STREAM_DEPTH = 5; // technically a size of 1 should be sufficient
const int RESPONSE_QUEUE_DEPTH = 32;
const int COMPLETED_JOBS_QUEUE_DEPTH = 16;
static_assert(RESPONSE_QUEUE_DEPTH >= COMPLETED_JOBS_QUEUE_DEPTH, "RESPONSE_QUEUE_DEPTH must be >= COMPLETED_JOBS_QUEUE_DEPTH");
const int JOBS_TO_SCHEDULE_QUEUE_DEPTH = 16;

typedef ap_axis<32, 1, 1, 1> JOB_STATUS_MESSAGE_AXI;
typedef ap_axis<sizeof(JobPackage)*8, 1, 1, 1> JOB_PACKAGE_AXI;

template <typename STREAM_CLASS>
bool CcrfQueuesBusy(int queue_id,
    hls::stream<STREAM_CLASS> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
)
{
    #pragma HLS INLINE
    #pragma HLS ARRAY_PARTITION variable=subtask_to_ccrf_queues
    #ifdef __SYNTHESIS__
    bool queue_busy = subtask_to_ccrf_queues[queue_id].full();
    #else
    bool queue_busy = !subtask_to_ccrf_queues[queue_id].empty();
    #endif
    return queue_busy;
 }



 void CcrfSchedulerTopLevel(hls::stream<JOB_PACKAGE_AXI> &incoming_job_requests,
                           hls::stream<JOB_STATUS_MESSAGE_AXI> &response_message_queue,
                            hls::stream<JobPackage> &jobs_to_schedule_queue,
                            hls::stream<JOB_COMPLETION_PACKET> &completed_jobs_queue,

 						   uintptr_t &CCRF_HARDWARE_SCRATCHPAD_START_OUT,
 						   uintptr_t &CCRF_HARDWARE_SCRATCHPAD_END_OUT,

 						   bool &ccrf_top_level_saw_data,
 						   bool &ccrf_top_level_scratchpad,
 						   bool &incoming_job_request_from_top_level_populated,
 						   bool &adding_to_jobs_in_progress,
						   bool &top_level_got_completed_job,
						   bool &can_write_response_message,
						   bool &internal_queue_populated,

                           int &jobs_ID_offset);

 void JobResultNotifier(hls::stream<JOB_COMPLETION_PACKET> &completed_job_queue,
                       hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress,
                       hls::stream<uintptr_t> completed_queues_from_ccrf_units[CCRF_COMPUTE_UNIT_COUNT],

					   bool &job_result_notifier_job_in_progress,
			           bool &job_result_notifier_completed_job_queue
					   );


void CcrfSubtaskScheduler(hls::stream<JobPackage> &input_jobs,
                          hls::stream<JOB_SUBTASK> &subtask_queue, 
                          hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress,
						  bool &ccrf_subtask_scheduler_got_data);

int GetAvailableCCRFUnit(CCRF_UNIT_STATUS_SIGNALS ccrf_status_signals[CCRF_COMPUTE_UNIT_COUNT],
                          hls::stream<JOB_SUBTASK> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
);

/* Assumes all HDR images are the same size. Doesn't perform an overlap test */
 bool DoesTaskWaitForDependencies(JOB_SUBTASK task_to_check,
                                  CCRF_UNIT_STATUS_SIGNALS ccrf_status_signals[CCRF_COMPUTE_UNIT_COUNT],
                                  hls::stream<JOB_SUBTASK> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
);


void CcrfSubtaskDispatcher(hls::stream<JOB_SUBTASK> &dispatcher_stream_in,
                           CCRF_UNIT_STATUS_SIGNALS ccrf_unit_status_signals[CCRF_COMPUTE_UNIT_COUNT],
                           hls::stream<JOB_SUBTASK> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT],
						   bool &ccrf_subtask_dispatcher_got_data);

/*
 void CcrfWrapper(hls::stream<JOB_PACKAGE_AXI> &incoming_job_requests,
                  hls::stream<JOB_STATUS_MESSAGE_AXI> &response_message_queue_axi,
 				 bool &incoming_jobs_queue_populated,
 				 bool &jobs_to_schedule_queue_populated,
 				 bool &subtask_queue_populated,
 				 bool &jobs_in_progress_queue_populated,
 				 bool &completed_jobs_queue_populated,
 				 bool &ccrf_1_has_data,

 				 bool &ccrf_top_level_saw_data,
                  bool &ccrf_top_level_scratchpad,

 				 bool &ccrf_subtask_scheduler_got_data,
 				 bool &ccrf_dispatcher_got_data,
 				 bool &incoming_jobs_populated_in_top_level,

 				 ap_int<4> &counter_out,
 				 ap_int<4> &counter_out_2,

 				 bool &job_result_notifier_job_in_progress,
 				 bool &job_result_notifier_completed_job_queue
                 );
*/
#endif

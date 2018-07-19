#include "ccrf_scheduler.hpp"
#include "job_descriptor.hpp"
#include "job_package.hpp"
#include "ccrf.hpp"
#include "utils.hpp"
//#include "driver.hpp"
#include <hls_stream.h>
#include <cstddef>
//#include <string.h>

using namespace hls;



void Counter(ap_int<4> &counter_out) {
    static ap_int<4> counter = 4;
    counter_out = counter++;
}

void CompletedJobQueueReader(hls::stream<JOB_STATUS_MESSAGE> &response_message_queue,
                             hls::stream<JOB_COMPLETION_PACKET> &completed_jobs_queue
                             )
{
    if(!completed_jobs_queue.empty()) {
        JOB_COMPLETION_PACKET completed_job = completed_jobs_queue.read();
        JOB_STATUS_MESSAGE completion_packet_for_host;
        completion_packet_for_host.packet_message_type = JOB_STATUS_MESSAGE::JOB_DONE_PACKET;
        completion_packet_for_host.job_ID = completed_job.job_ID;
        ASSERT(!response_message_queue.full(), "response message queue is full");
        response_message_queue.write(completion_packet_for_host);
    }
}


void JobTimerModule(hls::stream<JobPackage> &jobs_to_schedule_in,
		            hls::stream<JobPackage> &jobs_to_schedule_out,
					hls::stream<JOB_COMPLETION_PACKET> &completed_jobs_in,
					hls::stream<JOB_COMPLETION_PACKET> &completed_jobs_out
					)
{

	DO_PRAGMA(HLS STREAM variable=completed_jobs_in depth=COMPLETED_JOBS_QUEUE_DEPTH);
	DO_PRAGMA(HLS STREAM variable=jobs_to_schedule_in depth=INPUT_JOB_STREAM_DEPTH)

    #pragma HLS RESOURCE core=axis variable=jobs_to_schedule_in port_map=jobs_to_schedule_in
    #pragma HLS RESOURCE core=axis variable=jobs_to_schedule_out port_map=jobs_to_schedule_out
    #pragma HLS RESOURCE core=axis variable=completed_jobs_in port_map=completed_jobs_in
    #pragma HLS RESOURCE core=axis variable=completed_jobs_out port_map=completed_jobs_out

    #pragma HLS DATA_PACK variable=jobs_to_schedule_in
    #pragma HLS DATA_PACK variable=jobs_to_schedule_out
    #pragma HLS DATA_PACK variable=completed_jobs_in
    #pragma HLS DATA_PACK variable=completed_jobs_out

    static uint32_t job_timers[COMPLETED_JOBS_QUEUE_DEPTH] = {0,};
    static bool active_jobs[COMPLETED_JOBS_QUEUE_DEPTH] = {false,};

    for (int job = 0; job < COMPLETED_JOBS_QUEUE_DEPTH; job++)
	{
        #pragma HLS UNROLL
		if (active_jobs[job-1]) {
			job_timers[job-1] += 1;
		} else {
			job_timers[job-1] = 0;
		}
	}

    if (!jobs_to_schedule_in.empty()) {
    	JobPackage job_to_schedule = jobs_to_schedule_in.read();
    	active_jobs[job_to_schedule.job_ID-1] = true;
    	jobs_to_schedule_out.write(job_to_schedule);
    } else if (!completed_jobs_in.empty()) {
    	JOB_COMPLETION_PACKET completion_packet = completed_jobs_in.read();
    	completion_packet.cycle_count = job_timers[completion_packet.job_ID-1];
    	active_jobs[completion_packet.job_ID-1] = false;
    	completed_jobs_out.write(completion_packet);
    }
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

                           int &jobs_ID_offset)
{
    #pragma HLS DATA_PACK variable=completed_jobs_queue
    #pragma HLS DATA_PACK variable=incoming_job_requests
    #pragma HLS DATA_PACK variable=jobs_to_schedule_queue struct_level
    #pragma HLS DATA_PACK variable=response_message_queue
    #pragma HLS STREAM variable=incoming_job_requests depth=32
    #pragma HLS STREAM variable=response_message_queue depth=16
    DO_PRAGMA(HLS STREAM variable=completed_jobs_queue depth=COMPLETED_JOBS_QUEUE_DEPTH);

    #pragma HLS RESOURCE core=axis variable=incoming_job_requests port_map=incoming_job_requests
    #pragma HLS RESOURCE core=axis variable=response_message_queue port_map=response_message_queue
    #pragma HLS RESOURCE core=axis variable=jobs_to_schedule_queue port_map=jobs_to_schedule_queue
    #pragma HLS RESOURCE core=axis variable=completed_jobs_queue port_map=completed_jobs_queue

    #pragma HLS INTERFACE ap_none port=CCRF_HARDWARE_SCRATCHPAD_START_OUT
    #pragma HLS INTERFACE ap_none port=CCRF_HARDWARE_SCRATCHPAD_END_OUT
    #pragma HLS INTERFACE ap_none port=ccrf_top_level_saw_data
    #pragma HLS INTERFACE ap_none port=ccrf_top_level_scratchpad
    #pragma HLS INTERFACE ap_none port=incoming_job_request_from_top_level_populated


    static uintptr_t CCRF_HARDWARE_SCRATCHPAD_START = 10000;
    static uintptr_t CCRF_HARDWARE_SCRATCHPAD_END = 100000;

    incoming_job_request_from_top_level_populated = !incoming_job_requests.empty();
    ccrf_top_level_saw_data = (!incoming_job_requests.empty()) || (!completed_jobs_queue.empty());

    static bool forward_completion_packet_queue = true;
    bool completed_job_queue_populated = !completed_jobs_queue.empty();
    top_level_got_completed_job = completed_job_queue_populated;
    bool incoming_jobs_requests_populated = !incoming_job_requests.empty();
    bool response_message_queue_full = response_message_queue.full();
    bool can_accept_job = !jobs_to_schedule_queue.full();
	bool read_completion_packet = forward_completion_packet_queue && completed_job_queue_populated && !response_message_queue_full;

    static JobPackage active_job_package;
    #pragma HLS DATA_PACK variable=active_job_package
    static bool job_package_is_active = false;

    if (!job_package_is_active && incoming_jobs_requests_populated) {
    	JOB_PACKAGE_AXI job_package_axi = incoming_job_requests.read();
    	ap_int<sizeof(JobPackage)*8> job_request_raw_data = job_package_axi.data;
    	active_job_package = *(JobPackage*)&(job_request_raw_data);
    	job_package_is_active = true;
    }

    bool job_ID_0 = active_job_package.job_ID == (JOB_ID_T)0;
    bool is_scratchpad_message = job_package_is_active && job_ID_0; ccrf_top_level_scratchpad = is_scratchpad_message;
    can_write_response_message = !response_message_queue_full;
    bool write_active_job_to_queue = job_package_is_active && !is_scratchpad_message;
	bool write_completion_packet = !response_message_queue_full &&
			                       ((forward_completion_packet_queue && read_completion_packet) ||
			                       (!forward_completion_packet_queue && write_active_job_to_queue && can_accept_job));


    // Send response message to the host
    bool update_scratchpad_addr = job_package_is_active &&  is_scratchpad_message;
    if (write_completion_packet) {
    	JOB_STATUS_MESSAGE completion_packet_for_host;
        completion_packet_for_host.packet_message_type =
    				(forward_completion_packet_queue) ? JOB_STATUS_MESSAGE::JOB_DONE_PACKET :
    				                   can_accept_job ? JOB_STATUS_MESSAGE::JOB_ACCEPT_PACKET :
    						                            JOB_STATUS_MESSAGE::JOB_REJECT_PACKET;
        JOB_COMPLETION_PACKET dummy_packet = {0,0,0,0};
        JOB_COMPLETION_PACKET input_completion_packet = read_completion_packet ? completed_jobs_queue.read() : dummy_packet;
   		completion_packet_for_host.job_ID = read_completion_packet ? input_completion_packet.job_ID : active_job_package.job_ID;
   		completion_packet_for_host.cycle_count = read_completion_packet ? input_completion_packet.cycle_count : 0;
    	ap_int<sizeof(JOB_STATUS_MESSAGE)*8> response_message_reply_bits = *(ap_int<sizeof(JOB_STATUS_MESSAGE)*8>*)&completion_packet_for_host;
    	JOB_STATUS_MESSAGE_AXI axi_stream_packet;
    	axi_stream_packet.last = ap_uint<1>(true);
    	axi_stream_packet.data = ap_int<sizeof(JOB_STATUS_MESSAGE)*8>(response_message_reply_bits);
    	axi_stream_packet.id = ap_int<1>(0);
    	axi_stream_packet.keep = 0xFF;
    	axi_stream_packet.strb = 0;
    	axi_stream_packet.dest = 0;
    	axi_stream_packet.user = 0;
    	response_message_queue.write(axi_stream_packet);
    	if (write_active_job_to_queue) {
    		jobs_to_schedule_queue.write(active_job_package);
    		job_package_is_active = false;
    	}
    } else if (update_scratchpad_addr) {
    	CCRF_HARDWARE_SCRATCHPAD_START = update_scratchpad_addr ? active_job_package.job_descriptor.INPUT_IMAGES[0] : CCRF_HARDWARE_SCRATCHPAD_START;
    	CCRF_HARDWARE_SCRATCHPAD_END = update_scratchpad_addr ? active_job_package.job_descriptor.INPUT_IMAGES[1] : CCRF_HARDWARE_SCRATCHPAD_END;
    	job_package_is_active = false;
    }
    // Forward active jobs to the data pipeline
	CCRF_HARDWARE_SCRATCHPAD_START_OUT = CCRF_HARDWARE_SCRATCHPAD_START;
	CCRF_HARDWARE_SCRATCHPAD_END_OUT = CCRF_HARDWARE_SCRATCHPAD_END;
    forward_completion_packet_queue = !forward_completion_packet_queue;

}


void CcrfSubtaskScheduler(hls::stream<JobPackage> &input_jobs,
                          hls::stream<JOB_SUBTASK> &subtask_queue, 
                          hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress,

						  const uintptr_t &CCRF_HARDWARE_SCRATCHPAD_START,
						  const uintptr_t &CCRF_HARDWARE_SCRATCHPAD_END,

						  bool &ccrf_subtask_scheduler_got_data)
{

    DO_PRAGMA(HLS stream depth=INPUT_JOB_STREAM_DEPTH variable=input_jobs)
    #pragma HLS DATA_PACK variable=input_jobs struct_level
    #pragma HLS DATA_PACK variable=subtask_queue struct_level
    #pragma HLS DATA_PACK variable=jobs_in_progress struct_level
    #pragma HLS RESOURCE core=axis variable=input_jobs
    #pragma HLS RESOURCE core=axis variable=subtask_queue
    #pragma HLS RESOURCE core=axis variable=jobs_in_progress
    #pragma HLS INTERFACE ap_none port=ccrf_subtask_scheduler_got_data
    
    const int max_active_jobs = CCRF_COMPUTE_UNIT_COUNT;
    static bool current_job_valid = false;
    static JobDescriptor current_job;
    static JOB_ID_T current_job_ID;
    //uintptr_t output_addr = CCRF_HARDWARE_SCRATCHPAD_START;
    static uint32_t scratchpad_offset = 0;

    ccrf_subtask_scheduler_got_data = !input_jobs.empty();

    if (!current_job_valid && !input_jobs.empty()) {
        JobPackage current_job_package = input_jobs.read();
        current_job = current_job_package.job_descriptor;
        current_job_ID = current_job_package.job_ID;
        current_job_valid = true;
    }

    const int ADDR_BUFFER_SIZE = 10;
    uintptr_t input_addresses[ADDR_BUFFER_SIZE];
    uintptr_t output_addresses[ADDR_BUFFER_SIZE];

    if (current_job_valid) {
        const int image_size = current_job.IMAGE_SIZE();
        const int ldr_image_count = current_job.LDR_IMAGE_COUNT;
        for (int input = 0; input < ldr_image_count; input++) {
            #pragma HLS PIPELINE
            input_addresses[input] = current_job.INPUT_IMAGES[input];
        }

        for (int output = 0; output < ldr_image_count - 1; output++) {
            //if (output_addr + (image_size * sizeof(PIXEL4_T)) >= CCRF_HARDWARE_SCRATCHPAD_END) {
            if (CCRF_HARDWARE_SCRATCHPAD_START + scratchpad_offset + (image_size * sizeof(PIXEL4_T)) >= CCRF_HARDWARE_SCRATCHPAD_END) {
                //output_addr = CCRF_HARDWARE_SCRATCHPAD_START;
                scratchpad_offset = 0;
            }
            //output_addresses[output] = output_addr;
            output_addresses[output] = CCRF_HARDWARE_SCRATCHPAD_START + scratchpad_offset;
            //output_addr += image_size * sizeof(PIXEL4_T);
            scratchpad_offset += (image_size * sizeof(PIXEL4_T));
        }

        //while(jobs_in_progress.full());
        JOB_COMPLETION_PACKET job_completion_packet = {current_job.OUTPUT_IMAGE_LOCATION, 0, current_job.IMAGE_SIZE(), current_job_ID};
        jobs_in_progress.write(job_completion_packet);
        for (int layer = 0; layer < ldr_image_count - 1; layer++) {
            const int num_outputs = ldr_image_count - layer - 1;
            const bool last_task = (num_outputs == 1);
            for (int input = 0; input < ldr_image_count - layer - 1; input++) {

                // The final output should go to the actual specified output location, not the scratchpad
                uintptr_t real_output_addr = (last_task) ? current_job.OUTPUT_IMAGE_LOCATION : output_addresses[input];
                JOB_SUBTASK new_subtask;
                new_subtask.input1 = input_addresses[input];
                new_subtask.input2 = input_addresses[input + 1];
                new_subtask.output = real_output_addr;
                ASSERT(last_task ? true : real_output_addr + (image_size * sizeof(PIXEL4_T)) < CCRF_HARDWARE_SCRATCHPAD_END, "Out of range output");
                new_subtask.image_size = image_size;
                new_subtask.job_ID = current_job_ID;
                ASSERT(new_subtask.image_size != 0, "Invalid subtask image_size");
                ASSERT(new_subtask.input1 != (uintptr_t)nullptr, "Invalid subtask input1");
                ASSERT(new_subtask.input2 != (uintptr_t)nullptr, "Invalid subtask input2");
                ASSERT(new_subtask.output != (uintptr_t)nullptr, "Invalid subtask output");
                subtask_queue.write(new_subtask);
            }
            if (!last_task) {
                std::swap(input_addresses, output_addresses);
            }
        }

        current_job_valid = false;
    }
}


void JobResultNotifier(hls::stream<JOB_COMPLETION_PACKET> &completed_job_queue, 
                       hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress,
                       hls::stream<uintptr_t> completed_queues_from_ccrf_units[CCRF_COMPUTE_UNIT_COUNT],

					   bool &job_result_notifier_job_in_progress,
					   bool &job_result_notifier_completed_job_queue
)
{
    #pragma HLS ARRAY_PARTITION variable=completed_queues_from_ccrf_units
    #pragma HLS DATA_PACK variable=completed_job_queue struct_level
    #pragma HLS DATA_PACK variable=jobs_in_progress struct_level
    DO_PRAGMA(HLS STREAM variable=jobs_in_progress depth=JOBS_TO_SCHEDULE_QUEUE_DEPTH);
    #pragma HLS RESOURCE core=axis variable=completed_job_queue
    #pragma HLS RESOURCE core=axis variable=jobs_in_progress
    #pragma HLS RESOURCE core=axis variable=completed_queues_from_ccrf_units

    #pragma HLS INTERFACE ap_none port=job_result_notifier_job_in_progress
    #pragma HLS INTERFACE ap_none port=job_result_notifier_completed_job_queue

    static bool job_info_valid = false;
    static JOB_COMPLETION_PACKET job_info;

    job_result_notifier_job_in_progress = !jobs_in_progress.empty();
    job_result_notifier_completed_job_queue = false;

    if (!job_info_valid && !jobs_in_progress.empty()) {
        JOB_COMPLETION_PACKET tmp_job_info = jobs_in_progress.read();
        job_info = tmp_job_info;
        job_info_valid = true;
    } else if (job_info_valid && !completed_job_queue.full()) {
        bool job_completed = false;
        for (int i = 0; !job_completed && i < CCRF_COMPUTE_UNIT_COUNT; i++) {
            bool queue_empty = false;
            queue_empty = completed_queues_from_ccrf_units[i].empty();
            if (queue_empty) {
                continue;
            }
            //const PIXEL4_T *const output_addr = CCRF_completed_outputs[i]->read();
            uintptr_t output_addr = (uintptr_t)nullptr;
            output_addr = completed_queues_from_ccrf_units[i].read();
            job_completed = (output_addr == (uintptr_t)job_info.output_address);
            if (job_completed) {
                completed_job_queue.write(job_info);
                job_info_valid = false;
                job_completed = true;
                job_result_notifier_completed_job_queue = true;
            }
        }
    }
}


int GetAvailableCCRFUnit(CCRF_UNIT_STATUS_SIGNALS ccrf_unit_status_signals[CCRF_COMPUTE_UNIT_COUNT],
                         hls::stream<JOB_SUBTASK> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
)
{
    #pragma HLS INLINE
    #pragma HLS STREAM variable=subtask_to_ccrf_queues depth=1
    #pragma HLS ARRAY_PARTITION variable=subtask_to_ccrf_queues
    #pragma HLS ARRAY_PARTITION variable=ccrf_unit_status_signals
    for (int i = 0; i < CCRF_COMPUTE_UNIT_COUNT; i++) {
        #pragma HLS UNROLL
        bool is_processing = false;
        is_processing = ccrf_unit_status_signals[i].is_processing;
        if (!is_processing && !CcrfQueuesBusy<JOB_SUBTASK>(i, subtask_to_ccrf_queues)) {
            return i;
        }
    }
    return -1;
}

bool IntervalsOverlap(uintptr_t  interval_1, int interval_1_byte_count, uintptr_t interval_2, int interval_2_byte_count)
{
    bool interval_1_starts_in_interval_2 = (interval_2 <= interval_1 && interval_1 <= interval_2 + interval_2_byte_count);
    bool interval_2_starts_in_interval_1 = (interval_1 <= interval_2 && interval_2 <= interval_1 + interval_1_byte_count);
    return interval_1_starts_in_interval_2 || interval_2_starts_in_interval_1;
}

bool DoesTaskWaitForDependencies(JOB_SUBTASK task_to_check, 
                                 CCRF_UNIT_STATUS_SIGNALS ccrf_unit_status_signals[CCRF_COMPUTE_UNIT_COUNT],
                                 hls::stream<JOB_SUBTASK> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
)                                 
{
    #pragma HLS STREAM variable=subtask_to_ccrf_queues depth=1
    #pragma HLS ARRAY_PARTITION variable=subtask_to_ccrf_queues
    #pragma HLS ARRAY_PARTITION variable=ccrf_unit_status_signals
    #ifdef HW_COMPILE
    bool has_dependence = false;
    #endif
    const int dependency_count = 3;
    typedef void* dependency_type;
    
    uintptr_t dependencies[3]; //= {task_to_check.input1, task_to_check.input2, task_to_check.output};
    dependencies[0] = task_to_check.input1;
    dependencies[1] = task_to_check.input2;
    dependencies[2] = task_to_check.output;
    
    for (int ccrf_unit = 0; ccrf_unit < CCRF_COMPUTE_UNIT_COUNT; ccrf_unit++) {        
        #pragma HLS UNROLL
        bool queue_busy = CcrfQueuesBusy<JOB_SUBTASK>(ccrf_unit, subtask_to_ccrf_queues);
        JOB_SUBTASK ccrf_job;
        bool is_running = false;
        bool is_processing = false;
        ccrf_job = ccrf_unit_status_signals[ccrf_unit].job_info;
        is_running = ccrf_unit_status_signals[ccrf_unit].running;
        is_processing = ccrf_unit_status_signals[ccrf_unit].is_processing;
        bool is_idle = (!queue_busy && !is_processing) && is_running;
        if (is_idle) { //ccrf_status_signals[ccrf_unit].is_idle()) {
            // since the unit is idle, it can't be processing dependencies for the task
            // wait for the ccrf unit to load the value if queue_busy
            continue;
        }

        /* In the case where job IDs do not match (i.e. we are trying to schedule some subtasks of the)
         * next job while the tail of the current job is finishing), then we need to check every input and output
         * of the task_to_check against every input and output from the ccrf status signals, because 
         * there is no relationship between inputs of one job and the outputs of another (or vice versa).
         * We only have a relationship between the inputs and outputs of subtasks of the same job
         **/
        bool job_IDs_match = (task_to_check.job_ID == ccrf_job.job_ID);
        int ccrf_image_size = ccrf_job.image_size;
        //ASSERT(ccrf_image_size >= 65536, "IMAGE TOO SMALL");
        // Past here indicates the 1
        for (int i = dependency_count - 1; i >= 0; i--) {
            #pragma HLS UNROLL
            const uintptr_t task_dependence = dependencies[i];
            bool matches_dep1 = IntervalsOverlap(ccrf_job.input1, ccrf_image_size * sizeof(PIXEL4_T), 
                                                 task_dependence, task_to_check.image_size * sizeof(PIXEL4_T));
            bool matches_dep2 = IntervalsOverlap(ccrf_job.input2, ccrf_image_size * sizeof(PIXEL4_T), 
                                                 task_dependence, task_to_check.image_size * sizeof(PIXEL4_T));
            bool matches_dep3 = IntervalsOverlap(ccrf_job.output, ccrf_image_size * sizeof(PIXEL4_T), 
                                                 task_dependence, task_to_check.image_size * sizeof(PIXEL4_T));
            if (i == 2) {
                // For inputs, check for dependence against outputs, so we wait for the result
                // to be available
                if (matches_dep1 || matches_dep2 || (!job_IDs_match && matches_dep3)) {
                    #ifdef HW_COMPILE
                    //return true;
                    has_dependence = true;// || has_dependence;
                    #else
                    return true;
                    #endif
                }
            } else {
                // for outputs, make sure current inputs don't wait for this location.
                // so as not to overwrite a current location that is used as input for
                // another subtask
                if (matches_dep3 || (!job_IDs_match && (matches_dep2 || matches_dep1) ) ) {
                    #ifdef HW_COMPILE
                    //return true;
                    has_dependence = true;// || has_dependence;
                    #else
                    return true;
                    #endif
                }
            }
        }
    }
    #ifdef HW_COMPILE
    return has_dependence;
    #else
    return false;
    #endif
}


void CcrfSubtaskDispatcher(hls::stream<JOB_SUBTASK> &dispatcher_stream,
                           CCRF_UNIT_STATUS_SIGNALS ccrf_unit_status_signals[CCRF_COMPUTE_UNIT_COUNT],
                           hls::stream<JOB_SUBTASK> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT],
						   bool &ccrf_subtask_dispatcher_got_data
)                          
{
    #pragma HLS ARRAY_PARTITION variable=subtask_to_ccrf_queues
    #pragma HLS ARRAY_PARTITION variable=ccrf_unit_status_signals
    #pragma HLS DATA_PACK variable=ccrf_unit_status_signals struct_level
    #pragma HLS DATA_PACK variable=dispatcher_stream struct_level
    #pragma HLS DATA_PACK variable=subtask_to_ccrf_queues struct_level
    DO_PRAGMA(HLS STREAM variable=dispatcher_stream depth=DISPATCHER_STREAM_DEPTH);
    #pragma HLS RESOURCE core=axis variable=dispatcher_stream
    #pragma HLS RESOURCE core=axis variable=subtask_to_ccrf_queues

    static JOB_SUBTASK task_to_add;
    static bool task_to_add_pending = false; // If we popped the task from the stream but couldn't run it last call
    
    ccrf_subtask_dispatcher_got_data = !dispatcher_stream.empty();

    if (!task_to_add_pending && !dispatcher_stream.empty()) {
        task_to_add = dispatcher_stream.read();
        task_to_add_pending = true;
    } else if (task_to_add_pending) {
        bool dependence_is_processing = DoesTaskWaitForDependencies(task_to_add, 
                                                                    ccrf_unit_status_signals,
                                                                    subtask_to_ccrf_queues);
        if (dependence_is_processing) {
            return;
        }

        int available_ccrf_unit = GetAvailableCCRFUnit(ccrf_unit_status_signals, subtask_to_ccrf_queues);
        if (available_ccrf_unit >= 0) {

            // Write the subtask to the available ccrf unit
            ASSERT(task_to_add.input1 != (uintptr_t)nullptr, "Tried to push task with null input1");
            ASSERT(task_to_add.input2 != (uintptr_t)nullptr, "Tried to push task with null input2");
            ASSERT(task_to_add.output != (uintptr_t)nullptr, "Tried to push task with null output");
            subtask_to_ccrf_queues[available_ccrf_unit].write(task_to_add);

            // reset state of task to add
            task_to_add_pending = false;
            task_to_add.input1 = (uintptr_t)nullptr;
            task_to_add.input2 = (uintptr_t)nullptr;
            task_to_add.output = (uintptr_t)nullptr;
            task_to_add.image_size = 0;
            task_to_add.image_size = 0;
        }
    }
}


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
                 )
//,                 BYTE_T *const memory_bus)
{
	#pragma HLS RESOURCE core=axis variable=response_message_queue_axi
	#pragma HLS DATA_PACK variable=response_message_queue_axi
	#pragma HLS RESOURCE core=axis variable=incoming_job_requests
	#pragma HLS DATA_PACK variable=incoming_job_requests struct_level
    static hls::stream<JobPackage> jobs_to_schedule_queue;
    static hls::stream<JOB_COMPLETION_PACKET> completed_jobs_queue;
    static hls::stream<JOB_COMPLETION_PACKET> jobs_in_progress;
    static hls::stream<JOB_SUBTASK> subtask_queue;
    static CCRF_UNIT_STATUS_SIGNALS ccrf_unit_status_signals[CCRF_COMPUTE_UNIT_COUNT];
    static hls::stream<uintptr_t> ccrf_output_queues[CCRF_COMPUTE_UNIT_COUNT];
    static hls::stream<JOB_SUBTASK> ccrf_input_queues[CCRF_COMPUTE_UNIT_COUNT];
	#pragma HLS DATA_PACK variable=jobs_to_schedule_queue struct_level

    #pragma HLS STREAM variable ccrf_input_queues depth=1
    #pragma HLS STREAM variable ccrf_output_queues depth=1
    #pragma HLS STREAM variable=jobs_to_schedule_queue depth=8
    #pragma HLS STREAM variable=response_message_queue_axi depth=8
    #pragma HLS STREAM variable=completed_jobs_queue depth=8
    #pragma HLS STREAM variable=jobs_in_progress depth=4
    #pragma HLS STREAM variable=subtask_queue depth=32

    #pragma HLS ARRAY_PARTITION variable=ccrf_unit_status_signals

    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS INTERFACE ap_none port=incoming_jobs_queue_populated
    #pragma HLS INTERFACE ap_none port=jobs_to_schedule_queue_populated
    #pragma HLS INTERFACE ap_none port=subtask_queue_populated
    #pragma HLS INTERFACE ap_none port=jobs_in_progress_queue_populated
    #pragma HLS INTERFACE ap_none port=completed_jobs_queue_populated
    #pragma HLS INTERFACE ap_none port=ccrf_1_has_data

    #pragma HLS INTERFACE ap_none port=ccrf_top_level_saw_data
    #pragma HLS INTERFACE ap_none port=ccrf_top_level_scratchpad

    #pragma HLS INTERFACE ap_none port=ccrf_subtask_scheduler_got_data
    #pragma HLS INTERFACE ap_none port=ccrf_dispatcher_got_data
    #pragma HLS INTERFACE ap_none port=incoming_jobs_populated_in_top_level

    #pragma HLS INTERFACE ap_none port=counter_out
    #pragma HLS INTERFACE ap_none port=counter_out_2

    uintptr_t scratch_start;
	uintptr_t scratch_end;
    static ap_int<4> counter_2 = 0;
    counter_out_2 = ++counter_2;


    incoming_jobs_queue_populated = !incoming_job_requests.empty();
    jobs_to_schedule_queue_populated = !jobs_to_schedule_queue.empty();
    subtask_queue_populated = !subtask_queue.empty();
    jobs_in_progress_queue_populated = !jobs_in_progress.empty();
    completed_jobs_queue_populated = !completed_jobs_queue.empty();
    ccrf_1_has_data = !ccrf_input_queues[0].empty();

    Counter(counter_out);

    bool adding_to_jobs_in_progress;
    int jobs_ID_offset;
    bool top_level_got_completed_job;
    bool can_write_response_message;
	bool internal_queue_populated;

    CcrfSchedulerTopLevel(incoming_job_requests,
    		              response_message_queue_axi,
                          jobs_to_schedule_queue,
                          completed_jobs_queue,

						  scratch_start,
						  scratch_end,

						  ccrf_top_level_saw_data,
		                  ccrf_top_level_scratchpad,
						  incoming_jobs_populated_in_top_level,
						  adding_to_jobs_in_progress,
						  top_level_got_completed_job,

						  can_write_response_message,
						  internal_queue_populated,

						  jobs_ID_offset
						  );

    JobResultNotifier(completed_jobs_queue,
                      jobs_in_progress,
                      ccrf_output_queues,

					  job_result_notifier_job_in_progress,
					  job_result_notifier_completed_job_queue
                      );

    CcrfSubtaskScheduler(jobs_to_schedule_queue,
                         subtask_queue,
                         jobs_in_progress,

						 scratch_start,
                         scratch_end,

						 ccrf_subtask_scheduler_got_data
						 );

    CcrfSubtaskDispatcher(subtask_queue,
                          ccrf_unit_status_signals,
                          ccrf_input_queues,
						  ccrf_dispatcher_got_data
						  );

    bool ccrf_in_flags[CCRF_COMPUTE_UNIT_COUNT];
    bool ccrf_out_flags[CCRF_COMPUTE_UNIT_COUNT];

    for (int i = 0; i < CCRF_COMPUTE_UNIT_COUNT; i++) {
        #pragma HLS UNROLL
        Run_CCRF(ccrf_unit_status_signals[i],
                ccrf_input_queues[i],
                ccrf_output_queues[i],

				ccrf_in_flags[i],
				ccrf_out_flags[i]
				//,memory_bus
        		);
    }

    //
    //if (!response_message_queue.empty() && !incoming_job_requests.empty()) {
    //	JOB_STATUS_MESSAGE response_message_reply = response_message_queue.read();
    //    uint32_t response_message_reply_bits = *(uint16_t*)&response_message_reply;
    //    #pragma HLS DATA_PACK variable=response_message_reply
    //    JOB_STATUS_MESSAGE_AXI axi_stream_packet;
    //    axi_stream_packet.last = ap_uint<1>(true);
    //    axi_stream_packet.data = ap_int<32>(response_message_reply_bits);
    //    axi_stream_packet.id = ap_int<1>(0);
    //    axi_stream_packet.keep = 0xFF;
    //    axi_stream_packet.strb = 0;
    //    axi_stream_packet.dest = 0;
    //    axi_stream_packet.user = 0;
    //
    //    response_message_queue_axi.write(axi_stream_packet);
    //}
    //
}
*/

#include "ccrf_scheduler.hpp"
#include "job_descriptor.hpp"
#include "job_package.hpp"
#include "ccrf.hpp"
#include "utils.hpp"
//#include "driver.hpp"
#include <hls_stream.h>

using namespace hls;

uintptr_t CCRF_HARDWARE_SCRATCHPAD_START;
uintptr_t CCRF_HARDWARE_SCRATCHPAD_END;


void CcrfSchedulerTopLevel(hls::stream<JobPackage> &incoming_job_requests, 
                           hls::stream<JOB_STATUS_MESSAGE> &response_message_queue,
                           hls::stream<JobPackage> &jobs_to_schedule_queue,
                           hls::stream<JOB_COMPLETION_PACKET> &completed_jobs_queue)
{
    DO_PRAGMA(HLS stream depth=RESPONSE_QUEUE_DEPTH variable=response_message_queue)
    #pragma HLS stream depth=8 variable=incoming_job_requests
    DO_PRAGMA(HLS stream depth=JOBS_TO_SCHEDULE_QUEUE_DEPTH variable=jobs_to_schedule_queue)
    DO_PRAGMA(HLS stream depth=COMPLETED_JOBS_QUEUE_DEPTH variable=completed_jobs_queue)
    //#pragma HLS INTERFACE ap_fifo port=incoming_job_requests
    //#pragma HLS INTERFACE ap_fifo port=response_message_queue

    do {
        if(!completed_jobs_queue.empty()) {
            JOB_COMPLETION_PACKET completed_job = completed_jobs_queue.read();
            JOB_STATUS_MESSAGE completion_packet_for_host;
            completion_packet_for_host.packet_message_type = JOB_STATUS_MESSAGE::JOB_DONE_PACKET;
            completion_packet_for_host.job_ID = completed_job.job_ID;
            ASSERT(!response_message_queue.full(), "response message queue is full");
            response_message_queue.write(completion_packet_for_host);
        }

        if (!incoming_job_requests.empty()) {
            JOB_STATUS_MESSAGE response_packet;
            JobPackage job_request = incoming_job_requests.read();
            response_packet.packet_message_type = (jobs_to_schedule_queue.full()) ? JOB_STATUS_MESSAGE::JOB_REJECT_PACKET : JOB_STATUS_MESSAGE::JOB_ACCEPT_PACKET;
            response_packet.job_ID = job_request.job_ID;
            if (job_request.job_ID == 0) { // Initialize the start and end addresses
                CCRF_HARDWARE_SCRATCHPAD_START = job_request.job_descriptor.INPUT_IMAGES[0];
                CCRF_HARDWARE_SCRATCHPAD_END = job_request.job_descriptor.INPUT_IMAGES[1];
            } else {
                response_message_queue.write(response_packet);
                if (!jobs_to_schedule_queue.full()) {
                    jobs_to_schedule_queue.write(job_request);
                } 
            }
        }

    } while (0);
}


void CcrfSubtaskScheduler(hls::stream<JobPackage> &input_jobs, 
                          hls::stream<JOB_SUBTASK> &subtask_queue, 
                          hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress) 
{
    #pragma HLS INTERFACE ap_fifo port=input_jobs
    #pragma HLS INTERFACE ap_fifo port=subtask_queue
    #pragma HLS INTERFACE ap_fifo port=jobs_in_progress
    DO_PRAGMA(HLS stream depth=DISPATCHER_STREAM_DEPTH variable=subtask_queue)
    DO_PRAGMA(HLS stream depth=INPUT_JOB_STREAM_DEPTH variable=input_jobs)
    DO_PRAGMA(HLS stream depth=1 variable=jobs_in_progress); // Force only one job allowed at a time
    
    const int max_active_jobs = CCRF_COMPUTE_UNIT_COUNT;
    static bool current_job_valid = false;
    static JobDescriptor current_job;
    static JOB_ID_T current_job_ID;
    //uintptr_t output_addr = CCRF_HARDWARE_SCRATCHPAD_START;
    static uint32_t scratchpad_offset = 0;

    do {
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
                //if (output_addr + (image_size * sizeof(PIXEL_T)) >= CCRF_HARDWARE_SCRATCHPAD_END) {
                if (CCRF_HARDWARE_SCRATCHPAD_START + scratchpad_offset + (image_size * sizeof(PIXEL_T)) >= CCRF_HARDWARE_SCRATCHPAD_END) {
                    //output_addr = CCRF_HARDWARE_SCRATCHPAD_START;
                    scratchpad_offset = 0;
                }
                //output_addresses[output] = output_addr;
                output_addresses[output] = CCRF_HARDWARE_SCRATCHPAD_START + scratchpad_offset;
                //output_addr += image_size * sizeof(PIXEL_T);
                scratchpad_offset += image_size * sizeof(PIXEL_T);
            }

            //while(jobs_in_progress.full());
            JOB_COMPLETION_PACKET job_completion_packet = {current_job.OUTPUT_IMAGE_LOCATION, current_job_ID, current_job.IMAGE_SIZE()};
            jobs_in_progress.write(job_completion_packet);
            for (int layer = 0; layer < ldr_image_count - 1; layer++) {
                const int num_outputs = ldr_image_count - layer - 1;
                const bool last_task = (num_outputs == 1);
                for (int input = 0; input < ldr_image_count - layer - 1; input++) {

                    //while(subtask_queue.full()); 

                    // The final output should go to the actual specified output location, not the scratchpad
                    uintptr_t real_output_addr = (last_task) ? current_job.OUTPUT_IMAGE_LOCATION : output_addresses[input];
                    JOB_SUBTASK new_subtask;
                    new_subtask.input1 = input_addresses[input];
                    new_subtask.input2 = input_addresses[input + 1];
                    new_subtask.output = real_output_addr;
                    ASSERT(real_output_addr + (image_size * sizeof(PIXEL_T)) < CCRF_HARDWARE_SCRATCHPAD_END, "Out of range output");
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
    } while(0);
}


void JobResultNotifier(hls::stream<JOB_COMPLETION_PACKET> &completed_job_queue, 
                       hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress, 
                       CCRF_UNIT_STATUS_SIGNALS ccrf_unit_status_signals[CCRF_COMPUTE_UNIT_COUNT],
                       hls::stream<uintptr_t> completed_queues_from_ccrf_units[CCRF_COMPUTE_UNIT_COUNT]
)
{
    #pragma HLS STREAM variable=completed_queues_from_ccrf_units depth=1
    DO_PRAGMA(HLS stream depth=COMPLETED_JOBS_QUEUE_DEPTH variable=completed_job_queue)
    DO_PRAGMA(HLS stream depth=JOBS_TO_SCHEDULE_QUEUE_DEPTH variable=jobs_in_progress); // Force only one job allowed at a time


    static bool job_info_valid = false;
    static JOB_COMPLETION_PACKET job_info;

    do {
        if (!job_info_valid) {
            if (!jobs_in_progress.empty()) {
                JOB_COMPLETION_PACKET tmp_job_info = jobs_in_progress.read();
                job_info = tmp_job_info;
                job_info_valid = true;
            } 
        }

        if (job_info_valid) {
            if (completed_job_queue.full()) {
                continue;
            }

            bool job_completed = false;
            for (int i = 0; !job_completed && i < CCRF_COMPUTE_UNIT_COUNT; i++) {
                bool queue_empty = false;
                queue_empty = completed_queues_from_ccrf_units[i].empty();
                if (queue_empty) {
                    continue;
                }
                //const PIXEL_T *const output_addr = CCRF_completed_outputs[i]->read();
                uintptr_t output_addr = (uintptr_t)nullptr;
                output_addr = completed_queues_from_ccrf_units[i].read();
                job_completed = (output_addr == (uintptr_t)job_info.output_address);
                if (job_completed) {
                    completed_job_queue.write(job_info);
                    job_info_valid = false;
                    job_completed = true;
                }
            }
        }

    } while (0);
}


int GetAvailableCCRFUnit(CCRF_UNIT_STATUS_SIGNALS ccrf_unit_status_signals[CCRF_COMPUTE_UNIT_COUNT],
                         hls::stream<JOB_SUBTASK> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
)
{
    #pragma HLS STREAM variable=subtask_to_ccrf_queues depth=1
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
            bool matches_dep1 = IntervalsOverlap(ccrf_job.input1, ccrf_image_size * sizeof(PIXEL_T), 
                                                 task_dependence, task_to_check.image_size * sizeof(PIXEL_T));
            bool matches_dep2 = IntervalsOverlap(ccrf_job.input2, ccrf_image_size * sizeof(PIXEL_T), 
                                                 task_dependence, task_to_check.image_size * sizeof(PIXEL_T));
            bool matches_dep3 = IntervalsOverlap(ccrf_job.output, ccrf_image_size * sizeof(PIXEL_T), 
                                                 task_dependence, task_to_check.image_size * sizeof(PIXEL_T));
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


void CcrfSubtaskDispatcher(hls::stream<JOB_SUBTASK> &dispatcher_stream_in, 
                           CCRF_UNIT_STATUS_SIGNALS ccrf_unit_status_signals[CCRF_COMPUTE_UNIT_COUNT],
                           hls::stream<JOB_SUBTASK> subtask_to_ccrf_queues[CCRF_COMPUTE_UNIT_COUNT]
)                          
{
    #pragma HLS INTERFACE ap_fifo port=dispatcher_stream_in
    #pragma HLS STREAM variable subtask_to_ccrf_queues depth=1
    DO_PRAGMA(HLS stream depth=DISPATCHER_STREAM_DEPTH variable=dispatcher_stream_in)
    static JOB_SUBTASK task_to_add;
    bool task_to_add_pending = false; // If we popped the task from the stream but couldn't run it last call
    
    do {
        if (!task_to_add_pending && !dispatcher_stream_in.empty()) {
            task_to_add = dispatcher_stream_in.read();
            task_to_add_pending = true;
        }

        if (task_to_add_pending) {            
            bool dependence_is_processing = DoesTaskWaitForDependencies(task_to_add, 
                                                                        ccrf_unit_status_signals,
                                                                        subtask_to_ccrf_queues);
            if (dependence_is_processing) {
                continue;
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
    } while (0);
}



void CcrfWrapper(hls::stream<JobPackage> &incoming_job_requests,
                 hls::stream<JOB_STATUS_MESSAGE_AXI> &response_message_queue_axi
                 )
//,                 BYTE_T *const memory_bus)
{
	#pragma HLS RESOURCE core=axis variable=response_message_queue_axi
	#pragma HLS DATA_PACK variable=response_message_queue_axi
	#pragma HLS RESOURCE core=axis variable=incoming_job_requests
	#pragma HLS DATA_PACK variable=incoming_job_requests struct_level
    static hls::stream<JOB_STATUS_MESSAGE> response_message_queue;
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
    #pragma HLS STREAM variable=response_message_queue depth=8
    #pragma HLS STREAM variable=completed_jobs_queue depth=8
    #pragma HLS STREAM variable=jobs_in_progress depth=4
    #pragma HLS STREAM variable=subtask_queue depth=32



    //while(1) {
    #pragma HLS DATAFLOW
    CcrfSchedulerTopLevel(incoming_job_requests, 
                          response_message_queue,
                          jobs_to_schedule_queue,
                          completed_jobs_queue);

    JobResultNotifier(completed_jobs_queue, 
                      jobs_in_progress, 
                      ccrf_unit_status_signals,
                      ccrf_output_queues
                      );

    CcrfSubtaskScheduler(jobs_to_schedule_queue, 
                         subtask_queue, 
                         jobs_in_progress);

    CcrfSubtaskDispatcher(subtask_queue,
                          ccrf_unit_status_signals,
                          ccrf_input_queues
                          );

    for (int i = 0; i < CCRF_COMPUTE_UNIT_COUNT; i++) {
        #pragma HLS UNROLL
        #pragma HLS ARRAY_PARTITION variable=ccrf_unit_status_signals
        Run_CCRF(ccrf_unit_status_signals[i],
                ccrf_input_queues[i],
                ccrf_output_queues[i]
				//,memory_bus
        		);
    }
    
    if (!response_message_queue.empty()) {
        JOB_STATUS_MESSAGE response_message_reply = response_message_queue.read();
        uint32_t response_message_reply_bits = *(uint16_t*)&response_message_reply;
        #pragma HLS DATA_PACK variable=response_message_reply
        JOB_STATUS_MESSAGE_AXI axi_stream_packet;
        axi_stream_packet.last = ap_uint<1>(true);
        axi_stream_packet.data = ap_int<32>(response_message_reply_bits);
        axi_stream_packet.id = ap_int<1>(0);
        axi_stream_packet.keep = 0x00;
        axi_stream_packet.strb = 0;
        axi_stream_packet.dest = 0;
        axi_stream_packet.user = 0;

        response_message_queue_axi.write(axi_stream_packet);
    }
   // }
}

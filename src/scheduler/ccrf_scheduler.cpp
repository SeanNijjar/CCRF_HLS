#include "ccrf_scheduler.hpp"
#include "job_descriptor.hpp"
#include "job_package.hpp"
#include "ccrf.hpp"
#include "utils.hpp"
#include <hls_stream.h>

using namespace hls;


PIXEL_T *CCRF_SCRATCHPAD_START_ADDR = (PIXEL_T*)(0x10000000);
PIXEL_T *CCRF_SCRATCHPAD_END_ADDR = (PIXEL_T*)(0x20000000);



void CcrfSchedulerTopLevel(hls::stream<JobPackage> &incoming_job_requests, 
                           hls::stream<JOB_STATUS_MESSAGE> &response_message_queue,
                           hls::stream<JobPackage> &jobs_to_schedule_queue,
                           hls::stream<JOB_COMPLETION_PACKET> &completed_jobs_queue)
{
    DO_PRAGMA(HLS stream depth=RESPONSE_QUEUE_DEPTH variable=response_message_queue)
    //DO_PRAGMA(HLS stream depth=RESPONSE_QUEUE_DEPTH variable=incoming_job_requests)
    #pragma HLS stream depth=8 variable=incoming_job_requests
    DO_PRAGMA(HLS stream depth=JOBS_TO_SCHEDULE_QUEUE_DEPTH variable=jobs_to_schedule_queue)
    DO_PRAGMA(HLS stream depth=COMPLETED_JOBS_QUEUE_DEPTH variable=completed_jobs_queue)

    do {
        while(!completed_jobs_queue.empty()) {
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
            response_message_queue.write(response_packet);
            if (!jobs_to_schedule_queue.full()) {
                jobs_to_schedule_queue.write(job_request);
            }
        }
    } while (1);
}

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
                       hls::stream<uintptr_t> completed_queues_from_ccrf_units[CCRF_COMPUTE_UNIT_COUNT]
#endif
)
{
    #ifdef HW_COMPILE
    #pragma HLS STREAM variable=completed_subtasks_queue_1 depth=1
    #pragma HLS STREAM variable=completed_subtasks_queue_2 depth=1
    #pragma HLS STREAM variable=completed_subtasks_queue_3 depth=1
    #pragma HLS STREAM variable=completed_subtasks_queue_4 depth=1
    #pragma HLS STREAM variable=completed_subtasks_queue_5 depth=1
    #pragma HLS STREAM variable=completed_subtasks_queue_6 depth=1
    #endif

    JobResultNotifier(completed_job_queue, jobs_in_progress, ccrf_status_signals, 
    #ifdef HW_COMPILE
    completed_subtasks_queue_1, completed_subtasks_queue_2, completed_subtasks_queue_3,
    completed_subtasks_queue_4, completed_subtasks_queue_5, completed_subtasks_queue_6
    #else
    completed_queues_from_ccrf_units
    #endif
    );
}

void CcrfSubtaskScheduler(hls::stream<JobPackage> &input_jobs, 
                          hls::stream<JOB_SUBTASK> &subtask_queue, 
                          hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress) 
{
    DO_PRAGMA(HLS stream depth=DISPATCHER_STREAM_DEPTH variable=subtask_queue)
    DO_PRAGMA(HLS stream depth=INPUT_JOB_STREAM_DEPTH variable=input_jobs)
    DO_PRAGMA(HLS stream depth=1 variable=jobs_in_progress); // Force only one job allowed at a time
    
    const int max_active_jobs = CCRF_COMPUTE_UNIT_COUNT;

    bool current_job_valid = false;
    static JobDescriptor current_job;
    static JOB_ID_T current_job_ID;

    PIXEL_T *output_addr = CCRF_SCRATCHPAD_START_ADDR;
    do {
        if (!current_job_valid && !input_jobs.empty()) {
            JobPackage current_job_package = input_jobs.read();
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
                input_addresses[input] = (PIXEL_T*)current_job.INPUT_IMAGES[input];
            }

            for (int output = 0; output < ldr_image_count - 1; output++) {
                output_addresses[output] = output_addr;
                if (output_addr + image_size > CCRF_SCRATCHPAD_END_ADDR) {
                    output_addr = CCRF_SCRATCHPAD_START_ADDR;
                } else {
                    output_addr += image_size;
                }
            }

            while(jobs_in_progress.full());
            JOB_COMPLETION_PACKET job_completion_packet = {current_job_ID, (PIXEL_T*)current_job.OUTPUT_IMAGE_LOCATION, current_job.IMAGE_SIZE()};
            jobs_in_progress.write(job_completion_packet);
            for (int layer = 0; layer < ldr_image_count - 1; layer++) {
                const int num_outputs = ldr_image_count - layer - 1;
                for (int input = 0; input < ldr_image_count - layer - 1; input++) {

                    while(subtask_queue.full()); 

                    const bool last_task = (num_outputs == 1);
                    // The final output should go to the actual specified output location, not the scratchpad
                    PIXEL_T *output_addr = (last_task) ? (PIXEL_T*)current_job.OUTPUT_IMAGE_LOCATION : output_addresses[input];
                    JOB_SUBTASK new_subtask;
                    new_subtask.input1 = (uintptr_t)input_addresses[input];
                    new_subtask.input2 = (uintptr_t)input_addresses[input + 1];
                    new_subtask.output = (uintptr_t)output_addr;
                    new_subtask.image_size = image_size;
                    subtask_queue.write(new_subtask);
                }
                std::swap(input_addresses, output_addresses);
            }

            current_job_valid = false;
        }
    } while(1);
}


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
)
{
    #ifdef HW_COMPILE
    #pragma HLS STREAM variable=completed_subtasks_queue_1 depth=1
    #pragma HLS STREAM variable=completed_subtasks_queue_2 depth=1
    #pragma HLS STREAM variable=completed_subtasks_queue_3 depth=1
    #pragma HLS STREAM variable=completed_subtasks_queue_4 depth=1
    #pragma HLS STREAM variable=completed_subtasks_queue_5 depth=1
    #pragma HLS STREAM variable=completed_subtasks_queue_6 depth=1
    #endif
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
                if (!ccrf_status_signals[i].running || 
                    !CcrfQueuesBusy<uintptr_t>(i, 
                    #ifdef HW_COMPILE
                    completed_subtasks_queue_1,
                    completed_subtasks_queue_2,
                    completed_subtasks_queue_3,
                    completed_subtasks_queue_4,
                    completed_subtasks_queue_5,
                    completed_subtasks_queue_6
                    #else
                    completed_queues_from_ccrf_units //completed_queues_from_ccrf_units[i].empty()
                    #endif
                    )
                ) {
                    continue;
                }

                //const PIXEL_T *const output_addr = CCRF_completed_outputs[i]->read();
                PIXEL_T * output_addr = nullptr;
                #ifdef HW_COMPILE
                switch(i) {
                    case 0: output_addr = (PIXEL_T*)completed_subtasks_queue_1.read(); break;
                    case 1: output_addr = (PIXEL_T*)completed_subtasks_queue_2.read(); break;
                    case 2: output_addr = (PIXEL_T*)completed_subtasks_queue_3.read(); break;
                    case 3: output_addr = (PIXEL_T*)completed_subtasks_queue_4.read(); break;
                    case 4: output_addr = (PIXEL_T*)completed_subtasks_queue_5.read(); break;
                    case 5: output_addr = (PIXEL_T*)completed_subtasks_queue_6.read(); break;
                    default: assert(false); output_addr = nullptr; break;
                };
                #else
                output_addr = (PIXEL_T*)completed_queues_from_ccrf_units[i].read();
                #endif
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
)
{
    #ifdef HW_COMPILE
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_1 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_2 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_3 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_4 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_5 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_6 depth=1
    #endif
    for (int i = 0; i < CCRF_COMPUTE_UNIT_COUNT; i++) {
        if (!ccrf_status_signals[i].is_processing && 
            !CcrfQueuesBusy<JOB_SUBTASK>(i, 
        #ifdef HW_COMPILE
            subtask_to_ccrf_queue_1, subtask_to_ccrf_queue_2, subtask_to_ccrf_queue_3, subtask_to_ccrf_queue_4,
            subtask_to_ccrf_queue_5, subtask_to_ccrf_queue_6
        #else
            subtask_to_ccrf_queues
        #endif
            )
        ) {
            return i;
        }
    }
    return -1;
}



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
)                                 
{
    #ifdef HW_COMPILE 
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_1 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_2 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_3 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_4 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_5 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_6 depth=1
    bool has_dependence = false;
    #endif
    const int dependency_count = 3;
    typedef void* dependency_type;
    //const uintptr_t dependencies[3] = {task_to_check.input1, task_to_check.input2, task_to_check.output};
    uintptr_t dependencies[3]; //= {task_to_check.input1, task_to_check.input2, task_to_check.output};
    dependencies[0] = task_to_check.input1;
    dependencies[1] = task_to_check.input2;
    dependencies[2] = task_to_check.output;
    
    #pragma HLS UNROLL
    for (int ccrf_unit = 0; ccrf_unit < CCRF_COMPUTE_UNIT_COUNT; ccrf_unit++) {
        bool queue_busy = CcrfQueuesBusy<JOB_SUBTASK>(ccrf_unit, 
            #ifdef HW_COMPILE
                subtask_to_ccrf_queue_1, subtask_to_ccrf_queue_2, subtask_to_ccrf_queue_3,
                subtask_to_ccrf_queue_4, subtask_to_ccrf_queue_5, subtask_to_ccrf_queue_6
            #else 
                subtask_to_ccrf_queues
            #endif
            );
        bool is_idle = !(ccrf_status_signals[ccrf_unit].running);
        is_idle = is_idle || (!queue_busy && !(ccrf_status_signals[ccrf_unit].is_processing));
        if (is_idle) { //ccrf_status_signals[ccrf_unit].is_idle()) {
            // since the unit is idle, it can't be processing dependencies for the task
            continue;
        }
        // Past here indicates the 1
        #pragma HLS UNROLL
        for (int i = 0; i < dependency_count; i++) {
            const PIXEL_T *const task_dependence = (PIXEL_T*)dependencies[i];
            if (i == 2) {
                // For inputs, check for dependence against outputs, so we wait for the result
                // to be available
                if ((uintptr_t)ccrf_status_signals[ccrf_unit].task_dep_ptr1 == (uintptr_t)task_dependence ||
                    (uintptr_t)ccrf_status_signals[ccrf_unit].task_dep_ptr2 == (uintptr_t)task_dependence) {
                    #ifdef HW_COMPILE
                    has_dependence = true || has_dependence;
                    #else
                    return true;
                    #endif
                }
            } else {
                // for outputs, make sure current inputs don't wait for this location.
                // so as not to overwrite a current location that is used as input for
                // another subtask
                if ((uintptr_t)ccrf_status_signals[ccrf_unit].task_dep_ptr3 == (uintptr_t)task_dependence) {
                    #ifdef HW_COMPILE
                    has_dependence = true || has_dependence;
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


//template <typename CCRF_TYPE>
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
)                          
{
    #ifdef HW_COMPILE
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_1 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_2 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_3 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_4 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_5 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_6 depth=1
    #endif
    DO_PRAGMA(HLS stream depth=DISPATCHER_STREAM_DEPTH variable=dispatcher_stream_in)
    static JOB_SUBTASK task_to_add;
    static bool task_to_add_pending = false; // If we popped the task from the stream but couldn't run it last call
    
    while (1) {
        if (!task_to_add_pending && !dispatcher_stream_in.empty()) {
            task_to_add = dispatcher_stream_in.read();
            task_to_add_pending = true;
        }

        if (task_to_add_pending) {
            //bool dependence_is_processing = DoesTaskWaitForDependencies(task_to_add, ccrf_compute_units);
            
            bool dependence_is_processing = DoesTaskWaitForDependencies(task_to_add, ccrf_status_signals, 
            #ifdef HW_COMPILE
                                                subtask_to_ccrf_queue_1, subtask_to_ccrf_queue_2, subtask_to_ccrf_queue_3, 
                                                subtask_to_ccrf_queue_4, subtask_to_ccrf_queue_5, subtask_to_ccrf_queue_6
            #else
                                                subtask_to_ccrf_queues
            #endif
            );
            if (dependence_is_processing) {
                continue;
            }

            //int available_ccrf_unit = GetAvailableCCRFUnit(ccrf_compute_units);
            
            int available_ccrf_unit = GetAvailableCCRFUnit(ccrf_status_signals, 
            #ifdef HW_COMPILE
                                        subtask_to_ccrf_queue_1, subtask_to_ccrf_queue_2, subtask_to_ccrf_queue_3, 
                                        subtask_to_ccrf_queue_4, subtask_to_ccrf_queue_5, subtask_to_ccrf_queue_6
            #else
                                        subtask_to_ccrf_queues
            #endif
            );
            if (available_ccrf_unit >= 0) {

                // Write the subtask to the available ccrf unit
                #ifdef HW_COMPILE
                switch(available_ccrf_unit) {
                    case 0: subtask_to_ccrf_queue_1.write(task_to_add); break;
                    case 1: subtask_to_ccrf_queue_2.write(task_to_add); break;
                    case 2: subtask_to_ccrf_queue_3.write(task_to_add); break;
                    case 3: subtask_to_ccrf_queue_4.write(task_to_add); break;
                    case 4: subtask_to_ccrf_queue_5.write(task_to_add); break;
                    case 5: subtask_to_ccrf_queue_6.write(task_to_add); break;
                    default: assert(false); subtask_to_ccrf_queue_1.write(task_to_add); break;
                };
                #else 
                ASSERT(task_to_add.input1 != NULL, "Tried to push task with null input1");
                ASSERT(task_to_add.input2 != NULL, "Tried to push task with null input2");
                ASSERT(task_to_add.output != NULL, "Tried to push task with null output");
                subtask_to_ccrf_queues[available_ccrf_unit].write(task_to_add);
                #endif

                // reset state of task to add
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
)
{
    #ifdef HW_COMPILE
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_1 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_2 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_3 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_4 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_5 depth=1
    #pragma HLS STREAM variable=subtask_to_ccrf_queue_6 depth=1
    #endif
    
    CcrfSubtaskDispatcher(dispatcher_stream_in, ccrf_status_signals, 
    #ifdef HW_COMPILE
    subtask_to_ccrf_queue_1, subtask_to_ccrf_queue_2,
    subtask_to_ccrf_queue_3, subtask_to_ccrf_queue_4,
    subtask_to_ccrf_queue_5, subtask_to_ccrf_queue_6
    #else
    subtask_to_ccrf_queues
    #endif
    );
}


#ifdef HW_COMPILE
void CcrfWrapper(hls::stream<JobPackage> incoming_job_requests, 
                 hls::stream<JOB_STATUS_MESSAGE> response_message_queue)
{
    #pragma HLS INTERFACE axis register both port=incoming_job_requests
    #pragma HLS INTERFACE axis register both port=response_message_queue
    
    hls::stream<JobPackage> jobs_to_schedule_queue;
    hls::stream<JOB_COMPLETION_PACKET> completed_jobs_queue;
    hls::stream<JOB_COMPLETION_PACKET> jobs_in_progress;
    hls::stream<JOB_SUBTASK> subtask_queue;
    //hls::stream<uintptr_t> ccrf_output_queues[CCRF_COMPUTE_UNIT_COUNT];
    //hls::stream<JOB_SUBTASK> ccrf_input_queues[CCRF_COMPUTE_UNIT_COUNT];
    CCRF_UNIT_STATUS_SIGNALS ccrf_unit_status_signals[CCRF_COMPUTE_UNIT_COUNT];
    hls::stream<uintptr_t> ccrf_output_queues_1;
    hls::stream<JOB_SUBTASK> ccrf_input_queues_1;
    hls::stream<uintptr_t> ccrf_output_queues_2;
    hls::stream<JOB_SUBTASK> ccrf_input_queues_2;
    hls::stream<uintptr_t> ccrf_output_queues_3;
    hls::stream<JOB_SUBTASK> ccrf_input_queues_3;
    hls::stream<uintptr_t> ccrf_output_queues_4;
    hls::stream<JOB_SUBTASK> ccrf_input_queues_4;
    hls::stream<uintptr_t> ccrf_output_queues_5;
    hls::stream<JOB_SUBTASK> ccrf_input_queues_5;
    hls::stream<uintptr_t> ccrf_output_queues_6;
    hls::stream<JOB_SUBTASK> ccrf_input_queues_6;

    #pragma HLS STREAM variable=ccrf_output_queues_1 depth=1
    #pragma HLS STREAM variable=ccrf_input_queues_1 depth=1
    #pragma HLS STREAM variable=ccrf_output_queues_2 depth=1
    #pragma HLS STREAM variable=ccrf_input_queues_2 depth=1
    #pragma HLS STREAM variable=ccrf_output_queues_3 depth=1
    #pragma HLS STREAM variable=ccrf_input_queues_3 depth=1
    #pragma HLS STREAM variable=ccrf_output_queues_4 depth=1
    #pragma HLS STREAM variable=ccrf_input_queues_4 depth=1
    #pragma HLS STREAM variable=ccrf_output_queues_5 depth=1
    #pragma HLS STREAM variable=ccrf_input_queues_5 depth=1
    #pragma HLS STREAM variable=ccrf_output_queues_6 depth=1
    #pragma HLS STREAM variable=ccrf_input_queues_6 depth=1
    #pragma HLS STREAM variable=jobs_to_schedule_queue depth=8
    #pragma HLS STREAM variable=completed_jobs_queue depth=8
    #pragma HLS STREAM variable=jobs_in_progress depth=4
    #pragma HLS STREAM variable=subtask_queue depth=32


    CcrfSchedulerTopLevel(std::ref(incoming_job_requests), 
                          std::ref(response_message_queue),
                          std::ref(jobs_to_schedule_queue),
                          std::ref(completed_jobs_queue));

    JobResultNotifier(std::ref(completed_jobs_queue), 
                      std::ref(jobs_in_progress), 
                      std::ref(ccrf_unit_status_signals),
                      std::ref(ccrf_output_queues_1),
                      std::ref(ccrf_output_queues_2),
                      std::ref(ccrf_output_queues_3),
                      std::ref(ccrf_output_queues_4),
                      std::ref(ccrf_output_queues_5),
                      std::ref(ccrf_output_queues_6)
                      );

    CcrfSubtaskScheduler(std::ref(jobs_to_schedule_queue), 
                         std::ref(subtask_queue), 
                         std::ref(jobs_in_progress));

    CcrfSubtaskDispatcher(std::ref(subtask_queue),
                          std::ref(ccrf_unit_status_signals),
                          std::ref(ccrf_input_queues_1),
                          std::ref(ccrf_input_queues_2),
                          std::ref(ccrf_input_queues_3),
                          std::ref(ccrf_input_queues_4),
                          std::ref(ccrf_input_queues_5),
                          std::ref(ccrf_input_queues_6)
                          );

    /*
    #pragma HLS UNROLL
    for (int i = 0; i < CCRF_COMPUTE_UNIT_COUNT; i++) {
        Run_CCRF(std::ref(ccrf_unit_status_signals[i]),
                std::ref(ccrf_input_queues[i]),
                std::ref(ccrf_output_queues[i]));
    }
    */
    Run_CCRF(std::ref(ccrf_unit_status_signals[0]),
                std::ref(ccrf_input_queues_1),
                std::ref(ccrf_output_queues_1));
    Run_CCRF(std::ref(ccrf_unit_status_signals[1]),
                std::ref(ccrf_input_queues_2),
                std::ref(ccrf_output_queues_2));
    Run_CCRF(std::ref(ccrf_unit_status_signals[2]),
                std::ref(ccrf_input_queues_3),
                std::ref(ccrf_output_queues_3));
    Run_CCRF(std::ref(ccrf_unit_status_signals[3]),
                std::ref(ccrf_input_queues_4),
                std::ref(ccrf_output_queues_4));
    Run_CCRF(std::ref(ccrf_unit_status_signals[4]),
                std::ref(ccrf_input_queues_5),
                std::ref(ccrf_output_queues_5));
    Run_CCRF(std::ref(ccrf_unit_status_signals[5]),
                std::ref(ccrf_input_queues_6),
                std::ref(ccrf_output_queues_6));
}
#endif

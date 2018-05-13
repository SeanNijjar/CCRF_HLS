#include "ccrf_scheduler.hpp"
#include "job_descriptor.hpp"
#include "job_package.hpp"
#include "ccrf.hpp"
#include "utils.hpp"
#include <hls_stream.h>

using namespace hls;


PIXEL_T *CCRF_SCRATCHPAD_START_ADDR = (PIXEL_T*)(0x10000000);


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
    } while (0);
}

void JobResultsNotifier_StaticWrapper(hls::stream<JOB_COMPLETION_PACKET> &completed_job_queue, 
                       hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress,
                       CCRF ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT])
{
    JobResultNotifier<CCRF>(completed_job_queue, jobs_in_progress, ccrf_compute_units);
}

void CcrfSubtaskScheduler(hls::stream<JobPackage> &input_jobs, 
                          hls::stream<JOB_SUBTASK> &subtask_queue, 
                          hls::stream<JOB_COMPLETION_PACKET> &jobs_in_progress) 
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

            PIXEL_T *output_addr = CCRF_SCRATCHPAD_START_ADDR;
            for (int output = 0; output < ldr_image_count - 1; output++) {
                output_addresses[output] = output_addr + image_size;
                output_addr += image_size;
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
                    new_subtask.Initialize(input_addresses[input], input_addresses[input + 1], output_addr, image_size);
                    subtask_queue.write(new_subtask);
                }
                std::swap(input_addresses, output_addresses);
            }

            current_job_valid = false;
        }
    } while(1);
}


void CcrfSubtaskDispatcher_StaticWrapper(hls::stream<JOB_SUBTASK> &dispatcher_stream_in, 
                                         CCRF ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT]) 
{
    CcrfSubtaskDispatcher<CCRF>(dispatcher_stream_in, ccrf_compute_units) ;
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
    #pragma HLS STREAM variable=jobs_to_schedule_queue depth=8
    #pragma HLS STREAM variable=completed_jobs_queue depth=8
    #pragma HLS STREAM variable=jobs_in_progress depth=4
    #pragma HLS STREAM variable=subtask_queue depth=32


    CCRF ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT];/* = {
            CCRF(), 
            CCRF(), 
            CCRF(), 
            CCRF(),
            CCRF(),
            CCRF()
        };*/

    CcrfSchedulerTopLevel(incoming_job_requests, 
                                response_message_queue,
                                jobs_to_schedule_queue,
                                completed_jobs_queue);

    JobResultNotifier(completed_jobs_queue, 
                            jobs_in_progress, 
                            ccrf_compute_units);

    CcrfSubtaskScheduler(jobs_to_schedule_queue, 
                               subtask_queue, 
                               jobs_in_progress);

    CcrfSubtaskDispatcher<CCRF>(subtask_queue, 
                                      ccrf_compute_units);
}
#endif

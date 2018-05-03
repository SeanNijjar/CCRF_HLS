#include "job_package.hpp"
#include "job_descriptor.hpp"
#include "types.hpp"
#include "helper.hpp"
#include "ccrf_scheduler.hpp"

#include <hls_stream.h>

int main() 
{


    hls::stream<JobPackage> jobs_to_scheduler_queue;
    hls::stream<JOB_STATUS_MESSAGE> response_from_scheduler_queue;
    hls::stream<JobDescriptor::JOB_DESCRIPTOR_T> scheduler_to_dispatcher_queue;
    hls::stream<JOB_COMPLETION_PACKET> completed_jobs_to_scheduler_queue;

    
    // First Test: Reject (jobs_to_schedule_queue is full)
    while(!scheduler_to_dispatcher_queue.full()) {
        scheduler_to_dispatcher_queue.write(JobDescriptor::JOB_DESCRIPTOR_T());
    }
    /*
    //jobs_to_scheduler_queue.write();
    CCRF_Scheduler<false>(jobs_to_scheduler_queue, 
                          response_from_scheduler_queue,
                          scheduler_to_dispatcher_queue,
                          completed_jobs_to_scheduler_queue);


    // Second Test: Accept Job (send ACK and add job to jobs_to_schedule_queue)
    CCRF_Scheduler<false>(jobs_to_scheduler_queue, 
                          response_from_scheduler_queue,
                          scheduler_to_dispatcher_queue,
                          completed_jobs_to_scheduler_queue);


    // Third Test: See if the scheduler will send back the job completed message
    CCRF_Scheduler<false>(jobs_to_scheduler_queue, 
                          response_from_scheduler_queue,
                          scheduler_to_dispatcher_queue,
                          completed_jobs_to_scheduler_queue);
    */


}
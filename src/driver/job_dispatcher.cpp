#include "job_dispatcher.hpp"
#include "global_options.hpp"
#include "job_descriptor.hpp"
#include "job_package.hpp"
#include "driver.hpp"
#include "software_driver.hpp"
//#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <iostream>

JobDispatcher::JobDispatcher(E_DISPATCH_MODE _dispatch_mode) :
    next_available_job_ID(0),
    dispatch_mode(_dispatch_mode),
    accelerator_full(false),
    dispatch_request_in_flight(false),
    driver(outgoing_job_queue, incoming_job_status_queue, 5)
{
    ASSERT(_dispatch_mode == DISPATCH_MODE_EXCLUSIVE_BLOCKING, "Created job dispatched in non-exclusive mode. Only DISPATCH_MODE_EXCLUSIVE is currently supported");
    
}

JobDispatcher::~JobDispatcher()
{
    SynchronizeWait();
    try {
        StopDispatcher();
    } catch (...) {
        std::cerr << "FAILED to properly stop JobDispatcher main thread" << std::endl;
    }
}

JOB_ID_T JobDispatcher::GenerateNewJobID()
{
    JOB_ID_T new_job_ID;
    for (new_job_ID = 1; active_jobs.count(new_job_ID) > 0; new_job_ID++);
    return new_job_ID;
}

/** RETURNS: did_something
 */
bool JobDispatcher::TryDispatchJob() 
{
    // For now, always send the job dispatch request.
    // In future, retain state in the dispatcher to track whether or
    // not the FPGA is already full and we are waiting for completion
    // before submitting more work.
    if (pending_jobs.size()) {
        if (!accelerator_full && !dispatch_request_in_flight) {
            outgoing_job_queue.write(pending_jobs.front());
            dispatch_request_in_flight = true;
            return true;
        }
    }

    return false;
}

void JobDispatcher::MainDispatcherThreadLoop()
{
    bool did_something_this_iteration = false;

    do {
        did_something_this_iteration = false;

        if (!incoming_job_queue.empty()) {
            // Process the job and dispatch it

            //JobPackage new_job_package;
            //new_job_package.job_descriptor = incoming_job_queue.read();
            //new_job_package.job_ID = GenerateNewJobID();
            //
            JOB_ID_T new_job_ID = GenerateNewJobID();
            pending_jobs.push({new_job_ID, incoming_job_queue.read()});
            active_jobs.insert(new_job_ID);
            job_start_times[new_job_ID] = std::chrono::high_resolution_clock::now();
            did_something_this_iteration = true;
        }

        did_something_this_iteration = did_something_this_iteration || TryDispatchJob();

        if (!incoming_job_status_queue.empty()) {
            // Remove the job from the set of active jobs
            JOB_STATUS_MESSAGE job_status = incoming_job_status_queue.read();

            switch (job_status.packet_message_type) {
                case JOB_STATUS_MESSAGE::JOB_ACCEPT_PACKET: {
                    ASSERT(job_status.job_ID == pending_jobs.front().job_ID, "received accept message for job that isn't at head of queue");
                    executing_jobs.push(pending_jobs.front());
                    pending_jobs.pop();
                    dispatch_request_in_flight = false;
                    ASSERT(accelerator_full == false, "WEIRD: got a job accept when accelerator already full - doesn't make sense");
                } break;

                case JOB_STATUS_MESSAGE::JOB_REJECT_PACKET: {
                    dispatch_request_in_flight = false;
                    accelerator_full = true;
                    job_start_times.erase(job_status.job_ID);
                } break;

                case JOB_STATUS_MESSAGE::JOB_DONE_PACKET: {
                    if (accelerator_full) 
                        ASSERT(dispatch_request_in_flight == false, "Got job completion package from full accelerator but showing job in flight - doesn't make sense")

                    ASSERT(executing_jobs.front().job_ID == job_status.job_ID, "Got out of order job completion - currently unexpected in the design");
                    auto job_time = std::chrono::system_clock::now() - job_start_times[job_status.job_ID];
                    auto num_microseconds = std::chrono::duration_cast<std::chrono::microseconds> (job_time);

                    finished_jobs.push_back({num_microseconds, job_status.job_ID, 
                                             executing_jobs.front().job_descriptor.IMAGE_SIZE(), 
                                             executing_jobs.front().job_descriptor.LDR_IMAGE_COUNT});

                    executing_jobs.pop();
                    JOB_COMPLETION_PACKET job_completion_packet({(uintptr_t)nullptr, job_status.job_ID, -1});
                    outgoing_finished_job_queue.write(job_completion_packet);
                    active_jobs.erase(job_status.job_ID);
                    accelerator_full = false;
                    
                } break;
            };

            did_something_this_iteration = true;
        }

        did_something_this_iteration = did_something_this_iteration || TryDispatchJob();

        if (!did_something_this_iteration) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

    } while (true);
}

void JobDispatcher::StartDispatcher()
{
    driver_thread = std::thread(&JobDispatcher::MainDispatcherThreadLoop, this);
    driver.Start();
}

void JobDispatcher::StopDispatcher()
{
    driver_thread.join();
}

void JobDispatcher::SynchronizeWait()
{
    while (!incoming_job_queue.empty() || active_jobs.size() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void JobDispatcher::DispatchJobAsync(const JobDescriptor *const job_descriptor) 
{
    incoming_job_queue.write(*job_descriptor);
}

void JobDispatcher::DispatchJob(const JobDescriptor *const job_descriptor) 
{
    DispatchJobAsync(job_descriptor);
    SynchronizeWait();
}


auto JobDispatcher::GetTotalJobExecutionTime(const JOB_ID_T) const 
{
    UNIMPLEMENTED();
}

void JobDispatcher::PrintJobResultStats() 
{
    const int num_jobs = finished_jobs.size();
    for (auto finished_job : finished_jobs) {
        std::cout << "JOB:" << finished_job.job_ID;
        std::cout << ", IMAGE_SIZE:" << finished_job.image_size;
        std::cout << ", LDR_IMAGE_COUNT:" << finished_job.ldr_image_count;
        std::cout << ", TIME(us):" << finished_job.num_microseconds_to_complete.count();
        std::cout << std::endl;
    }
}

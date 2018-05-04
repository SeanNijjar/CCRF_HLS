#include "driver.hpp"
#include <unistd.h>
#include <fcntl.h>
#include "global_options.hpp"
#include "job_descriptor.hpp"
#include "job_package.hpp"
#include <thread>
#include <iostream>

JobDispatcher::JobDispatcher(E_DISPATCH_MODE _dispatch_mode) :
    next_available_job_ID(0),
    dispatch_mode(_dispatch_mode),
    accelerator_full(false),
    dispatch_request_in_flight(false)
{
    ASSERT(_dispatch_mode == DISPATCH_MODE_EXCLUSIVE_BLOCKING, "Created job dispatched in non-exclusive mode. Only DISPATCH_MODE_EXCLUSIVE is currently supported");
}

JobDispatcher::~JobDispatcher()
{
    WaitForJobsToFinish();
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

            JobPackage new_job_package;
            new_job_package.job_descriptor = incoming_job_queue.read();
            new_job_package.job_ID = GenerateNewJobID();
            //
            pending_jobs.push(new_job_package);

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
                } break;

                case JOB_STATUS_MESSAGE::JOB_DONE_PACKET: {
                    if (accelerator_full) 
                        ASSERT(dispatch_request_in_flight == false, "Got job completion package from full accelerator but showing job in flight - doesn't make sense")

                    ASSERT(executing_jobs.front().job_ID == job_status.job_ID, "Got out of order job completion - currently unexpected in the design");
                    executing_jobs.pop();
                    JOB_COMPLETION_PACKET job_completion_packet(job_status.job_ID, nullptr, -1);
                    outgoing_finished_job_queue.write(job_completion_packet);

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
}

void JobDispatcher::StopDispatcher()
{
    driver_thread.join();
}

void JobDispatcher::SynchronizeWait()
{
    while (active_jobs.size() > 0) {
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

void JobDispatcher::WaitForJobsToFinish() 
{
    // volatile - but will this treat it that way?
    // TODO: with interrupts, we should be able to just sleep here until the job queue is empty
    while(active_job_queue.size()) 
    {
        ;// spin_lock - for now, the interrupt handler will be notified
         // when each job is done and it will go ahead and clean that job up accordingly
         // I'm a little worried that this might be leaving too much for the interrupt
         // handler to execute...
    }
}

void JobDispatcher::TransferJobToRemote(const JOB_ID_T job_ID, const JobDescriptor *const job_descriptor) 
{
    UNIMPLEMENTED();
    JobDispatcher::E_JOB_STATUS job_status = JOB_STATUS_COPYING_TO_REMOTE;
    active_job_queue.push_back(job_ID);

    // Create the 
    const int num_LDR_images_in_job = job_descriptor->LDR_IMAGE_COUNT;
    const int num_HDR_outputs_in_job = 1;

    const size_t bytes_needed_for_job_descr = JobDescriptor::BytesNeededForJobDescriptor(num_LDR_images_in_job);
    const size_t bytes_needed_for_job = bytes_needed_for_job_descr + 
                                        (num_LDR_images_in_job + num_HDR_outputs_in_job) * job_descriptor->IMAGE_SIZE;
    const size_t min_remote_buffer_alignment = ROUND_TO_NEXT_POWER_OF_2(bytes_needed_for_job_descr);
    BYTE_T *remote_job_buffer = CreateRemoteBuffer(bytes_needed_for_job, min_remote_buffer_alignment);

    JobPackage::ConsolidateJob(remote_job_buffer, job_descriptor); // Will compact all job data into a contiguous array

    CopyJobDataToRemote(job_descriptor);
    for (int i = 0; i < job_descriptor->LDR_IMAGE_COUNT; i++) {
        DmaToRemoteBuffer(BYTE_T *const remote_buffer, const BYTE_T *const local_buffer, const size_t bytes_to_copy);
        SignalDoneDmaToRemoteForLdrImage(job_ID, ldr_image);
    }
    
    

    //Send
}

void JobDispatcher::SignalDoneDmaToRemoteForLdrImage(const JOB_ID_T job_ID, const int ldr_image_num) const 
{
    UNIMPLEMENTED_QUIET("Function SignalDoneDmaToRemoteForLdrImage currently doesn't do anything...");
}

void JobDispatcher::DispatchJobExclusive(const JOB_ID_T job_ID, const JobDescriptor *const job_descriptor) 
{
    UNIMPLEMENTED();
}

void JobDispatcher::DispatchJobASAP(const JOB_ID_T job_ID, const JobDescriptor *const job_descriptor) 
{
    UNIMPLEMENTED();
    /*
    E_JOB_STATUS job_status = JOB_STATUS_INVALID;
    if(TryJobDispatch() == false) {
        job_status = JOB_STATUS_NOT_STARTED;
        waiting_jobs_queue.push_back(job_ID);
    } else {
        TransferJobToRemote(job_ID, job_descriptor)

        


    }

    */
}



JobDispatcher::E_JOB_STATUS GetJobStatus(const JOB_ID_T)
{
    UNIMPLEMENTED();
}

BYTE_T *CreateRemoteBuffer(const size_t buffer_size_in_bytes, const size_t min_address_alignment) 
{
    if (global_options.operating_mode == OPERATING_MODE_SW_ONLY) {
        UNIMPLEMENTED();
    } else if (global_options.operating_mode == OPERATING_MODE_HW_AND_SW) {
        ASSERT(false, "OPERATING_MODE_HW_AND_SW unimplemented in CreateRemoteBuffer");
    } else {
        if (global_options.operating_mode <= OPERATING_MODE_LAST) {
            ASSERT(false, "Implementation for operating mode" << global_options.operating_mode << " is missing");
        } else if (global_options.operating_mode == OPERATING_MODE_INVALID) {
            ASSERT(false, "INVALID OPERATING MODE");
        } else {
            ASSERT(false, "CONFUSED ABOUT OPERATING MODE");
        }
    }
}

void DmaToRemoteBuffer(BYTE_T * const remote_buffer, 
                       const BYTE_T * const local_buffer, 
                       const size_t bytes_to_copy, 
                       const uint32_t remote_job_start_address)
{
    std::string dma_to = global_options.dma_to_device;
    std::string control_device = global_options.control_device;

    int dma_to_device_fd = open(dma_to.c_str(), O_RDWR | O_NONBLOCK);
    lseek(dma_to_device_fd, remote_job_start_address, SEEK_SET);
    write(dma_to_device_fd, remote_buffer, bytes_to_copy);
    INFO("Wrote inputs via DMA");
}

void DmaFromRemoteBuffer(BYTE_T * const local_buffer, 
                         const BYTE_T * const remote_buffer, 
                         const size_t bytes_to_read, 
                         const uint32_t remote_job_output_address)
{
    std::string dma_from_device = global_options.dma_from_device;
    std::string control_device = global_options.control_device;    

    int dma_from_device_fd  = open(dma_from_device.c_str(), O_RDWR | O_NONBLOCK);
    lseek(dma_from_device_fd, remote_job_output_address, SEEK_SET);
    read(dma_from_device_fd, local_buffer, bytes_to_read);
    INFO("Outputs read via DMA");
}
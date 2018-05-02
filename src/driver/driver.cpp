#include "driver.hpp"
#include <unistd.h>
#include <fcntl.h>
#include "global_options.hpp"
#include "job_descriptor.hpp"
#include "job_package.hpp"

JobDispatcher::JobDispatcher(E_DISPATCH_MODE _dispatch_mode) :
    next_available_job_ID(0),
    dispatch_mode(_dispatch_mode)
{
    ASSERT(_dispatch_mode == DISPATCH_MODE_EXCLUSIVE_BLOCKING, "Created job dispatched in non-exclusive mode. Only DISPATCH_MODE_EXCLUSIVE is currently supported");
}

JOB_ID_T JobDispatcher::DispatchJob(const JobDescriptor::JOB_DESCRIPTOR_T *const job_descriptor) 
{
    UNIMPLEMENTED();
    const JOB_ID_T job_ID = next_available_job_ID++;
    if (dispatch_mode == DISPATCH_MODE_EXCLUSIVE_BLOCKING) {

    } else if (dispatch_mode == DISPATCH_MODE_EXCLUSIVE) {
        DispatchJobExclusive(job_ID, job_descriptor);
    } else {
        UNIMPLEMENTED();
    }

    return job_ID;
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

void JobDispatcher::TransferJobToRemote(const JOB_ID_T job_ID, const JobDescriptor::JOB_DESCRIPTOR_T *const job_descriptor) 
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

void JobDispatcher::DispatchJobExclusive(const JOB_ID_T job_ID, const JobDescriptor::JOB_DESCRIPTOR_T *const job_descriptor) 
{
    UNIMPLEMENTED();
}

void JobDispatcher::DispatchJobASAP(const JOB_ID_T job_ID, const JobDescriptor::JOB_DESCRIPTOR_T *const job_descriptor) 
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
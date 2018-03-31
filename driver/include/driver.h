#include "common/include/job_descriptor/job_descriptor.h"

#include <chrono>
#include <vector>
#include <map>
typedef unsigned int JOB_ID_T;



class RemoteHostDescriptor
{

  public:
    //size_t max_
};


/**
 * The job packager will package the contents of a job (i.e. the job descriptor and the input 
 * image data) into a byte array for DMA. It will also be used as a common interface to consistently
 * store and retrieve data from the same location in the byte buffer.
 **/
class JobPackager
{
    // Assumes byte_buffer is entirely in locally addressable memory
    void CopyJobToBuffer(BYTE_T *byte_buffer, const JOB_DESCRIPTOR_IMPL *const job_descriptor) {

    }


}

/**
 * This is not a thread-safe class, so don't try to dispatch jobs from multiple threads!!!
 * 
 * To add thread-safe functionality, a thread-safe job queue will need to be used instead
 * of the current implementation
 **/
class JobDispatcher
{

  public:
    enum E_DISPATCH_MODE
    {
        DISPATCH_MODE_FIRST = 0,
        DISPATCH_MODE_EXCLUSIVE = 0,
        DISPATCH_MODE_ASAP = DISPATCH_MODE_EXCLUSIVE + 1
    }

    enum E_JOB_STATUS 
    {
        JOB_STATUS_INVALID = -1,
        JOB_STATUS_FIRST = 0,

        JOB_STATUS_NOT_STARTED = JOB_STATUS_FIRST,
        JOB_STATUS_COPYING_TO_REMOTE,
        JOB_STATUS_DISPATCHED_TO_REMOTE,
        JOB_STATUS_IN_PROGRESS_ON_REMOTE,
        JOB_STATUS_DONE_ON_REMOTE,
        JOB_STATUS_COPY_BACK_FROM_REMOTE,
        JOB_STATUS_CLOSED_ON_REMOTE,
        JOB_STATUS_CLEANING_UP_REMOTE,
        JOB_STATUS_TELL_APPLICATION_DONE,
        JOB_STATUS_CLEANUP_FINAL,
        JOB_STATUS_DONE,

        JOB_STATUS_LAST = JOB_STATUS_DONE
    };

    JobDispatcher(E_DISPATCH_MODE _dispatch_mode) :
        next_available_job_ID(0),
        dispatch_mode(_dispatch_mode)
    {
        ASSERT(_dispatch_mode == DISPATCH_MODE_EXCLUSIVE, "Created job dispatched in non-exclusive mode. Only DISPATCH_MODE_EXCLUSIVE is currently supported");
    }

    /** This function will attempt to dispatch the job to the remote HDR processor.
     *  However, if the remote HDR processor is unable to currently accept this job
     *  (i.e. it doesn't have enough resources to manage it) then this function will
     *  push the job to the waiting jobs queue. The job will *actually* be dispatched
     *  to the remote processor when the resources are available
     **/
    JOB_ID_T DispatchJob(const JOB_DESCRIPTOR_IMPL *const job_descriptor) 
    {
        UNIMPLEMENTED();
        const JOB_ID_T job_ID = next_available_job_ID++;
        if (dispatch_mode == DISPATCH_MODE_EXCLUSIVE) {

        } else {

        }

        return job_ID;
    }

    auto GetTotalJobExecutionTime(const JOB_ID_T) const {
        UNIMPLEMENTED();
    }

  private:
    void TransferJobToRemote(const JOB_ID_T job_ID, const JOB_DESCRIPTOR_IMPL *const job_descriptor) {
        UNIMPLEMENTED();
        job_status = JOB_STATUS_COPYING_TO_REMOTE;
        active_job_queue.push_back(job_ID);

        // Create the 
        const int num_LDR_images_in_job = job_descriptor->LDR_IMAGE_COUNT;
        const int num_HDR_outputs_in_job = 1;

        const size_t bytes_needed_for_job_descr = JobDescriptor::BytesNeededForJobDescriptor(num_LDR_images_in_job);
        const size_t bytes_needed_for_job = bytes_needed_for_job_descr + 
                                            (num_LDR_images_in_job + num_HDR_outputs_in_job) * job_descriptor->IMAGE_SIZE;
        const size_t min_remote_buffer_alignment = ROUND_TO_NEXT_POWER_OF_2(bytes_needed_for_job_descr);
        BYTE_T *remote_job_buffer = CreateRemoteBuffer(bytes_needed_for_job, min_remote_buffer_alignment);

        CopyJobDataToRemote(job_descriptor);
        for (int i = 0; i < job_descriptor->LDR_IMAGE_COUNT; i++) {
            DmaToRemoteBuffer(BYTE_T *const remote_buffer, const BYTE_T *const local_buffer, const size_t bytes_to_copy);
            SignalDoneDmaToRemoteForLdrImage(job_ID, ldr_image);
        }
        
        

        //Send
    }

    void SignalDoneDmaToRemoteForLdrImage(const JOB_ID_T job_ID, const int ldr_image_num) const {
        UNIMPLEMENTED_QUIET("Function SignalDoneDmaToRemoteForLdrImage currently doesn't do anything...");
    }

    void DispatchJobExclusive(const JOB_ID_T job_ID, const JOB_DESCRIPTOR_IMPL *const job_descriptor) {
        UNIMPLEMENTED();
    }

    void DispatchJobASAP(const JOB_ID_T job_ID, const JOB_DESCRIPTOR_IMPL *const job_descriptor) {
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

  private:
    std::map<JOB_ID_T, 
    std::map<JOB_ID_T, JOB_DESCRIPTOR_IMPL*> job_descriptor_map;

    // These are the jobs currently being processed on the remote processor
    std::vector<JOB_ID_T> active_job_queue;

    // These are the jobs that could not immediately be dispatched to the remote processor and so 
    // are waiting on the local host to be dispatched at a later time when resources are available
    std::vector<JOB_ID_T> waiting_jobs_queue;  
    JOB_ID_T next_available_job_ID; 

    E_DISPATCH_MODE dispatch_mode;

    RemoteHostDescriptor remote_host_descriptor;
};


E_JOB_STATUS GetJobStatus(const JOB_ID_T)
{
    UNIMPLEMENTED();
}

// SUPPORTS COMPILATION FOR SW ONLY AND HW_SW MODES FOR TESTING
BYTE_T *CreateRemoteBuffer(const size_t buffer_size_in_bytes, const size_t min_address_alignment) {
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

// SUPPORTS COMPILATION FOR SW ONLY AND HW_SW MODES FOR TESTING
// BUT FOR NOW, JUST TRY USING THE SAME DMA LOGIC FOR BOTH PATHS, SEE IF IT WORKS
void DmaToRemoteBuffer(BYTE_T *const remote_buffer, const BYTE_T *const local_buffer, const size_t bytes_to_copy)
{
    UNIMPLEMENTED();
}

// SUPPORTS COMPILATION FOR SW ONLY AND HW_SW MODES FOR TESTING
// BUT FOR NOW, JUST TRY USING THE SAME DMA LOGIC FOR BOTH PATHS, SEE IF IT WORKS
void DmaFromRemoteBuffer(BYTE_T *const local_buffer, const BYTE_T *const remote_buffer, const size_t bytes_to_copy)
{
    UNIMPLEMENTED();
}


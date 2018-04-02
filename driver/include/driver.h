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
    void CopyJobToBuffer(BYTE_T *byte_buffer, const JobDescriptor::JOB_DESCRIPTOR_T *const job_descriptor) {

    }


};

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
        DISPATCH_MODE_EXCLUSIVE_BLOCKING,
        DISPATCH_MODE_ASAP
    };

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
        
        // More tasks can be deployed once we hit this state, it's just
        // that resources need to be cleaned up
        JOB_STATUS_CLEANUP_FINAL,
        JOB_STATUS_DONE,

        JOB_STATUS_LAST = JOB_STATUS_DONE
    };

    JobDispatcher(E_DISPATCH_MODE _dispatch_mode);

    /** This function will attempt to dispatch the job to the remote HDR processor.
     *  However, if the remote HDR processor is unable to currently accept this job
     *  (i.e. it doesn't have enough resources to manage it) then this function will
     *  push the job to the waiting jobs queue. The job will *actually* be dispatched
     *  to the remote processor when the resources are available
     **/
    JOB_ID_T DispatchJob(const JobDescriptor::JOB_DESCRIPTOR_T *const job_descriptor);

    auto GetTotalJobExecutionTime(const JOB_ID_T) const;

    void WaitForJobsToFinish();

  private:
    void TransferJobToRemote(const JOB_ID_T job_ID, const JobDescriptor::JOB_DESCRIPTOR_T *const job_descriptor);

    void SignalDoneDmaToRemoteForLdrImage(const JOB_ID_T job_ID, const int ldr_image_num) const;

    void DispatchJobExclusive(const JOB_ID_T job_ID, const JobDescriptor::JOB_DESCRIPTOR_T *const job_descriptor);

    void DispatchJobASAP(const JOB_ID_T job_ID, const JobDescriptor::JOB_DESCRIPTOR_T *const job_descriptor);

  private:
    std::map<JOB_ID_T, JobDescriptor::JOB_DESCRIPTOR_T*> job_descriptor_map;

    // These are the jobs currently being processed on the remote processor
    std::vector<JOB_ID_T> active_job_queue;

    // These are the jobs that could not immediately be dispatched to the remote processor and so 
    // are waiting on the local host to be dispatched at a later time when resources are available
    std::vector<JOB_ID_T> waiting_jobs_queue;  
    JOB_ID_T next_available_job_ID; 

    E_DISPATCH_MODE dispatch_mode;

    RemoteHostDescriptor remote_host_descriptor;
};


JobDispatcher::E_JOB_STATUS GetJobStatus(const JOB_ID_T);

// SUPPORTS COMPILATION FOR SW ONLY AND HW_SW MODES FOR TESTING
BYTE_T *CreateRemoteBuffer(const size_t buffer_size_in_bytes, const size_t min_address_alignment);

// SUPPORTS COMPILATION FOR SW ONLY AND HW_SW MODES FOR TESTING
// BUT FOR NOW, JUST TRY USING THE SAME DMA LOGIC FOR BOTH PATHS, SEE IF IT WORKS
// TODO: see if there is a better way to remotely map the address rather than having 
//       to manually pass it in
void DmaToRemoteBuffer(const std::weak_ptr<BYTE_T> const remote_buffer, 
                       const std::weak_ptr<BYTE_T> const local_buffer, 
                       const size_t bytes_to_copy, 
                       const uint32_t remote_job_start_address);

// SUPPORTS COMPILATION FOR SW ONLY AND HW_SW MODES FOR TESTING
// BUT FOR NOW, JUST TRY USING THE SAME DMA LOGIC FOR BOTH PATHS, SEE IF IT WORKS
void DmaFromRemoteBuffer(const std::weak_ptr<BYTE_T> const local_buffer, 
                         const std::weak_ptr<BYTE_T> const remote_buffer, 
                         const size_t bytes_to_read, 
                         const uint32_t remote_job_output_address);


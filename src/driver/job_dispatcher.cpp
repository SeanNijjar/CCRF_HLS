#include "job_dispatcher.hpp"
#include "global_options.hpp"
#include "job_descriptor.hpp"
#include "job_package.hpp"
#include "driver.hpp"

#ifndef ZYNQ_COMPILE
#include "software_driver.hpp"
#endif
//#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <iostream>





JobDispatcher::JobDispatcher(E_DISPATCH_MODE _dispatch_mode) :
    next_available_job_ID(0),
    dispatch_mode(_dispatch_mode),
    accelerator_full(false),
    dispatch_request_in_flight(false),
    driver(outgoing_job_queue, incoming_job_status_queue
    #ifndef ZYNQ_COMPILE
    , 5
    #endif
    )
{
    ASSERT(_dispatch_mode == DISPATCH_MODE_EXCLUSIVE_BLOCKING, "Created job dispatched in non-exclusive mode. Only DISPATCH_MODE_EXCLUSIVE is currently supported");
    
    const int max_image_size = 4000 * 4000 * sizeof(PIXEL4_T);
    for (int i = 0; i < 6; ++i) {
        axidma_input_image_transfer_buffers[i] = (uintptr_t)driver.AxidmaMalloc(max_image_size);
    }
    axidma_output_image_transfer_buffer = (uintptr_t)driver.AxidmaMalloc(max_image_size);
    run_main_loop = true;
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
    #ifdef LOOPBACK_TEST
    static JOB_ID_T new_job_ID = 1;
    return new_job_ID++;
    #else
    JOB_ID_T new_job_ID;
    for (new_job_ID = JobPackage::INITIALIZATION_PACKET_ID() + 1; active_jobs.count(new_job_ID) > 0; new_job_ID++);
    return new_job_ID;
    #endif
}



bool JobDispatcher::TransferInputImagesToDevice(JobDescriptor &job_descriptor)
{
	std::cout<<"TransferInputImagesToDevice"<<std::endl;
    const int input_image_count = job_descriptor.LDR_IMAGE_COUNT;
    const int transfer_size = job_descriptor.IMAGE_SIZE() * sizeof(PIXEL4_T);
    for (int i = 0; i < input_image_count; i++) {
        const uintptr_t image_pl_addr = (uintptr_t)driver.DeviceMalloc(transfer_size);

        //first DMA write PS mem data to second DMA
        //driver.AxidmaSendData((void*)axidma_input_image_transfer_buffers[i], (void*)job_descriptor.INPUT_IMAGES[i], transfer_size);

        //second DMA write data to PL DMA///////////////////////////////////////////////////////////////
        bool pl_dma_write_success = driver.PlDmaWrite((void *const)axidma_input_image_transfer_buffers[i], job_descriptor.INPUT_IMAGES[i], image_pl_addr, transfer_size);
        if (!pl_dma_write_success) {
            std::cout << "pl_dma_write failed" << std::endl;
		    return false;
        }

        // update the PL to PS DDR address mappings
        pl_to_ps_output_addr_map[image_pl_addr] = job_descriptor.INPUT_IMAGES[i];
        job_descriptor.INPUT_IMAGES[i] = image_pl_addr;
    }
	std::cout<<"/TransferInputImagesToDevice"<<std::endl;
	return true;
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
            #ifdef ZYNQ_COMPILE
            std::cout << "Sending Job Request" << std::endl;
            JobDescriptor &job_to_submit = pending_jobs.front().job_descriptor;
            uintptr_t ps_addr = job_to_submit.OUTPUT_IMAGE_LOCATION;
            uintptr_t pl_addr = (uintptr_t)DeviceMalloc(job_to_submit.IMAGE_SIZE() * sizeof(PIXEL4_T));
            pl_to_ps_output_addr_map[pl_addr] = ps_addr;
            job_to_submit.OUTPUT_IMAGE_LOCATION = pl_addr;
            bool initial_transfers_successful = TransferInputImagesToDevice(job_to_submit);
            if (!initial_transfers_successful) {
                run_main_loop = false;
                std::cout<<"TryDispatchJob initial transfer failed"<<std::endl;
                return false;
            }

            bool submitted = driver.SendJobRequest(pending_jobs.front());
            if (submitted) {
                dispatch_request_in_flight = true;
                return true;
            } else {
                std::cout<<"TryDispatchJob failure submit failed"<<std::endl;
                return false;
            }
            #else
            outgoing_job_queue.push_back(pending_jobs.front());
            dispatch_request_in_flight = true;
            return true;
            #endif
        }
    }

    return false;
}

bool JobDispatcher::JobResponseQueueHasData()
{
    #ifdef ZYNQ_COMPILE
    return driver.ResponseQueueHasData(sizeof(JOB_STATUS_MESSAGE));
    #else
    #ifdef CSIM
    return !incoming_job_status_queue.empty();
    #else
    return true;//incoming_job_status_queue.empty();
    #endif
    #endif
}

bool JobDispatcher::ReadJobStatusMessage(JOB_STATUS_MESSAGE &read_message)
{
    bool status = false;
    #ifdef ZYNQ_COMPILE
    #ifndef LOOPBACK_TEST
    status = driver.ReadResponseQueuePacket((uint8_t*)&read_message, sizeof(JOB_STATUS_MESSAGE));
    #else
    int response_message_int;
    status = driver.ReadResponseQueuePacket((uint8_t*)&response_message_int, sizeof(JOB_STATUS_MESSAGE));
    std::cout << "Response packet: " << response_message_int << std::endl;
    read_message.packet_message_type = (response_message_int & 0xFF);
    read_message.job_ID = ((response_message_int >> 8)& 0xFF);
    #endif
    #else
    while (incoming_job_status_queue.empty());
    ASSERT(incoming_job_status_queue.size(), "Incoming job queue empty");
    read_message = incoming_job_status_queue.front();
    incoming_job_status_queue.erase(incoming_job_status_queue.begin());
    status = true;
    #endif
    return status;
}



void JobDispatcher::MainDispatcherThreadLoop()
{
    bool did_something_this_iteration = false;

    #ifdef CSIM
    {
        const int scratchpad_size = 6000000;
        const int scratchpad_size_in_bytes = scratchpad_size * sizeof(PIXEL4_T);
        uintptr_t scratchpad_start_addr = (uintptr_t)new BYTE_T[scratchpad_size_in_bytes];
        uintptr_t scratchpad_end_addr = (uintptr_t)((char*)scratchpad_start_addr + scratchpad_size_in_bytes);
        JobPackage initialization_message;
        initialization_message.job_ID = JobPackage::INITIALIZATION_PACKET_ID(); //
        initialization_message.job_descriptor.INPUT_IMAGES[0] = scratchpad_start_addr;
        initialization_message.job_descriptor.INPUT_IMAGES[1] = scratchpad_end_addr;
        outgoing_job_queue.push_back(initialization_message);
    }
    #endif

    bool jobs_dispatched = false;
    do {
        did_something_this_iteration = false;

        if (!incoming_job_queue.empty()) {
            // Process the job and dispatch it
            JOB_ID_T new_job_ID = GenerateNewJobID();
            std::cout << "Submitting job from dispatcher: " << new_job_ID << std::endl;
            pending_jobs.push({incoming_job_queue.front(), new_job_ID});
            incoming_job_queue.erase(incoming_job_queue.begin());
            active_jobs.insert(new_job_ID);
            job_start_times[new_job_ID] = std::chrono::high_resolution_clock::now();
            did_something_this_iteration = true;
            jobs_dispatched = true;
        }

        did_something_this_iteration = TryDispatchJob() || did_something_this_iteration;

        JOB_STATUS_MESSAGE job_status;
        bool read_successful = (jobs_dispatched) ? ReadJobStatusMessage(job_status) : false;

        if (read_successful) {
            // Remove the job from the set of active jobs
            std::cout << "Got Successful Read" << std::endl; 
            #ifdef LOOPBACK_TEST
            ASSERT((uint8_t)job_status.packet_message_type == (uint8_t)(pending_jobs.front().job_ID & 0xFF), "LOOPBACK RESULT ERROR packet_message_type " << (unsigned int)job_status.packet_message_type << "!=" << (unsigned int)(pending_jobs.front().job_ID & 0xFF));
            ASSERT((uint8_t)job_status.job_ID == (uint8_t)((pending_jobs.front().job_ID >> 8) & 0xFF), "LOOPBACK RESULT ERROR job_ID " << (unsigned int)job_status.job_ID << "!=" << (unsigned int)((pending_jobs.front().job_ID >> 8) & 0xFF));
            pending_jobs.pop();
            active_jobs.erase((JOB_ID_T)job_status.packet_message_type);
            dispatch_request_in_flight = false;
            #else
            switch (job_status.packet_message_type) {
                case JOB_STATUS_MESSAGE::JOB_ACCEPT_PACKET: {
                    ASSERT(job_status.job_ID == pending_jobs.front().job_ID, "received accept message for job that isn't at head of queue");
                    executing_jobs.push(pending_jobs.front());
                    pending_jobs.pop();
                    dispatch_request_in_flight = false;
                    ASSERT(accelerator_full == false, "WEIRD: got a job accept when accelerator already full - doesn't make sense");
                    std::cout<<"Job Accept"<<(int)job_status.job_ID<<std::endl;
                } break;

                case JOB_STATUS_MESSAGE::JOB_REJECT_PACKET: {
                    dispatch_request_in_flight = false;
                    accelerator_full = true;
                    job_start_times.erase(job_status.job_ID);
                    std::cout << "Job Reject " << (int)job_status.job_ID << std::endl;
                } break;

                case JOB_STATUS_MESSAGE::JOB_DONE_PACKET: {
                    if (accelerator_full) 
                        ASSERT(dispatch_request_in_flight == false, "Got job completion package from full accelerator but showing job in flight - doesn't make sense")

                    std::cout<<"Job Complete"<<(int)job_status.job_ID<<std::endl;

//                    ASSERT(executing_jobs.front().job_ID == job_status.job_ID, "Got out of order job completion - currently unexpected in the design");
                    auto job_time = std::chrono::system_clock::now() - job_start_times[job_status.job_ID];
                    auto num_microseconds = std::chrono::duration_cast<std::chrono::microseconds> (job_time);

                    JobDescriptor &completed_job_descriptor = executing_jobs.front().job_descriptor;
                    void *const pl_addr = (void*)completed_job_descriptor.OUTPUT_IMAGE_LOCATION;
                    const int image_size_in_bytes = completed_job_descriptor.IMAGE_SIZE() * sizeof(PIXEL4_T);
                    void *const ps_addr = (void*)pl_to_ps_output_addr_map.at((uintptr_t)pl_addr);
                    const bool pl_to_ps_copy_success = driver.PL_to_PS_DMA((void *const)axidma_output_image_transfer_buffer, ps_addr, pl_addr, image_size_in_bytes);
                    completed_job_descriptor.OUTPUT_IMAGE_LOCATION = (uintptr_t)ps_addr;

                    // Restore the PS input image DDR addresses
                    const int ldr_image_count = completed_job_descriptor.LDR_IMAGE_COUNT;
                    for (int i = 0; i < ldr_image_count; i++) {
                        void *const input_image_pl_addr = (void*)completed_job_descriptor.INPUT_IMAGES[i];
                        completed_job_descriptor.INPUT_IMAGES[i] = pl_to_ps_output_addr_map.at((uintptr_t)input_image_pl_addr);
                    }

                    pl_to_ps_output_addr_map.erase((uintptr_t)pl_addr);

                    finished_jobs.push_back({num_microseconds, job_status.job_ID, 
                                             executing_jobs.front().job_descriptor.IMAGE_SIZE(), 
                                             executing_jobs.front().job_descriptor.LDR_IMAGE_COUNT});
                    if (executing_jobs.front().job_ID == job_status.job_ID) {
                        executing_jobs.pop();
                    }
                    JOB_COMPLETION_PACKET job_completion_packet({(uintptr_t)nullptr, job_status.job_ID, -1});
                    outgoing_finished_job_queue.push_back(job_completion_packet);
                    active_jobs.erase(job_status.job_ID);
                    accelerator_full = false;
                    
                } break;
            };
            #endif

            did_something_this_iteration = true;
        }

        did_something_this_iteration = did_something_this_iteration || TryDispatchJob();

        if (!did_something_this_iteration) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

    } while (run_main_loop);

    while(active_jobs.size()) active_jobs.clear();
    while(executing_jobs.size()) executing_jobs.pop();
}

void JobDispatcher::StartDispatcher()
{
    driver_thread = std::thread(&JobDispatcher::MainDispatcherThreadLoop, this);
    #ifndef ZYNQ_COMPILE
    driver.Start();
    #endif
}

void JobDispatcher::StopDispatcher()
{
    #ifndef ZYNQ_COMPILE
    driver_thread.join();
    #endif
}

void JobDispatcher::SynchronizeWait()
{
    while (!incoming_job_queue.empty() || active_jobs.size() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void JobDispatcher::DispatchJobAsync(const JobDescriptor *const job_descriptor) 
{
    incoming_job_queue.push_back(*job_descriptor);
}

void JobDispatcher::DispatchJob(const JobDescriptor *const job_descriptor) 
{
    DispatchJobAsync(job_descriptor);
    SynchronizeWait();
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

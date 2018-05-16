#ifndef SOFTWARE_DRIVER_H
#define SOFTWARE_DRIVER_H

#include "driver.hpp"
#include "job_package.hpp"
#include "types.hpp"
#include "ccrf.hpp"
#include "software_test_ccrf.hpp"
#include "ccrf_scheduler.hpp"

#include <hls_stream.h>
#include <thread>

const int default_ccrf_unit_count = CCRF_COMPUTE_UNIT_COUNT;

class SoftwareTestDriver : public Driver
{
  public:
    SoftwareTestDriver(hls::stream<JobPackage> &incoming_queue, hls::stream<JOB_STATUS_MESSAGE> &output_queue) : 
        Driver(incoming_queue, output_queue), already_started(false)
    {
        InitializeCcrfUnitsAndQueues(default_ccrf_unit_count);
    }

    SoftwareTestDriver(hls::stream<JobPackage> &incoming_queue, 
                       hls::stream<JOB_STATUS_MESSAGE> &output_queue, 
                       const int default_ccrf_count) : 
        Driver(incoming_queue, output_queue), already_started(false)
    {
        InitializeCcrfUnitsAndQueues(default_ccrf_count);
    }

    ~SoftwareTestDriver();

    void SendJobLaunchRequest(JobPackage job_request);

    void Start();

  protected:
    void InitializeCcrfUnitsAndQueues(const int unit_count)
    {
        // ccrf_output_queues.resize(unit_count);//, hls::stream<PIXEL_T*>());
        // ccrf_input_queues.resize(unit_count);
        for (int i = 0; i < CCRF_COMPUTE_UNIT_COUNT; i++) {
            ccrf_unit_status_signals[i].is_processing = false;
            ccrf_unit_status_signals[i].running = false;
            ccrf_unit_status_signals[i].job_info.image_size = 0;
            ccrf_unit_status_signals[i].job_info.input1 = (uintptr_t)nullptr;
            ccrf_unit_status_signals[i].job_info.input2 = (uintptr_t)nullptr;
            ccrf_unit_status_signals[i].job_info.output = (uintptr_t)nullptr;
            ccrf_unit_status_signals[i].job_info.job_ID = 0;
        }
    }

    std::thread ccrf_top_level_scheduler_thread;
    std::thread job_result_notifier_thread;
    std::thread ccrf_task_scheduler_thread;
    std::thread ccrf_dispatcher_thread;

    std::thread ccrf_threads[CCRF_COMPUTE_UNIT_COUNT];


    // CCRF Engine Queues
    //hls::stream<JobPackage> incoming_job_request_queue; // 1  incoming_job_request_queue
    //hls::stream<JOB_STATUS_MESSAGE> job_status_response_queue; // 2 outgoing_job_response_queue
    hls::stream<JobPackage> jobs_to_schedule_queue; // 3
    hls::stream<JOB_SUBTASK> scheduler_to_dispatcher_subtask_queue; // 4
    hls::stream<JOB_COMPLETION_PACKET> jobs_in_progress_queue; // 5
    hls::stream<JOB_COMPLETION_PACKET> completed_job_from_completion_module_queue; // 6

    //std::vector<SoftwareDummyCCRF> ccrf_compute_units;
    #ifdef HW_COMPILE
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
    #else
    hls::stream<uintptr_t> ccrf_output_queues[CCRF_COMPUTE_UNIT_COUNT];
    hls::stream<JOB_SUBTASK> ccrf_input_queues[CCRF_COMPUTE_UNIT_COUNT];
    #endif
    CCRF_UNIT_STATUS_SIGNALS ccrf_unit_status_signals[CCRF_COMPUTE_UNIT_COUNT];
    //SoftwareDummyCCRF ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT];


    bool already_started;

};

#endif
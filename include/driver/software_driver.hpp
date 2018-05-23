#ifndef SOFTWARE_DRIVER_H
#define SOFTWARE_DRIVER_H

#include "driver.hpp"
#include "job_package.hpp"
#include "types.hpp"
#include "ccrf.hpp"
#include "software_test_ccrf.hpp"
#include "ccrf_scheduler.hpp"

#include <vector>
#include <thread>

const int default_ccrf_unit_count = CCRF_COMPUTE_UNIT_COUNT;

class SoftwareTestDriver : public Driver
{
  public:
    SoftwareTestDriver(std::vector<JobPackage> &incoming_queue, std::vector<JOB_STATUS_MESSAGE> &output_queue) : 
        Driver(incoming_queue, output_queue), already_started(false)
    {
        InitializeCcrfUnitsAndQueues(default_ccrf_unit_count);
    }

    SoftwareTestDriver(std::vector<JobPackage> &incoming_queue, 
                       std::vector<JOB_STATUS_MESSAGE> &output_queue, 
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
        // ccrf_output_queues.resize(unit_count);
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


    std::thread ccrf_wrapper_csim_thread;
    
    void RunCcrfCsim();

    // CCRF Engine Queues
    std::vector<JobPackage> jobs_to_schedule_queue; // 3
    std::vector<JOB_SUBTASK> scheduler_to_dispatcher_subtask_queue; // 4
    std::vector<JOB_COMPLETION_PACKET> jobs_in_progress_queue; // 5
    std::vector<JOB_COMPLETION_PACKET> completed_job_from_completion_module_queue; // 6

    std::vector<uintptr_t> ccrf_output_queues[CCRF_COMPUTE_UNIT_COUNT];
    std::vector<JOB_SUBTASK> ccrf_input_queues[CCRF_COMPUTE_UNIT_COUNT];
    CCRF_UNIT_STATUS_SIGNALS ccrf_unit_status_signals[CCRF_COMPUTE_UNIT_COUNT];

    bool already_started;
};

#endif
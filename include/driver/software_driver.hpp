#ifndef SOFTWARE_DRIVER_H
#define SOFTWARE_DRIVER_H

#include "driver.hpp"
#include "job_package.hpp"
#include "types.hpp"
#include "ccrf.hpp"
#include "ccrf_scheduler.hpp"

#include <hls_stream.h>
#include <thread>

const int default_ccrf_unit_count = CCRF_COMPUTE_UNIT_COUNT;

class SoftwareTestDriver : public Driver
{
  public:
    SoftwareTestDriver(hls::stream<JobPackage> &incoming_queue, hls::stream<JOB_STATUS_MESSAGE> &output_queue) : 
        Driver(incoming_job_request_queue, outgoing_job_response_queue), already_started(false)
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

    // void SetJobLaunchQueue(hls::stream<JobPackage> &job_request_queue)
    // {
    //     if (!already_started) {
    //         Start();
    //     }
    // }

    // void SetJobStatusQueue(hls::stream<JOB_STATUS_MESSAGE> &job_status_queue)
    // {
    //     if (!already_started) {
    //         Start();
    //     }
    // }

    void Start();

  protected:
    void InitializeCcrfUnitsAndQueues(const int unit_count)
    {
        // ccrf_output_queues.resize(unit_count);//, hls::stream<PIXEL_T*>());
        // ccrf_input_queues.resize(unit_count);
        
        for (int i = 0; i < unit_count; i++) {
            //ccrf_compute_units.push_back(SoftwareDummyCCRF());//ccrf_input_queues[i], ccrf_output_queues[i]));
            ccrf_output_queues[i] = &ccrf_compute_units[i].output_subtask_queue;
            ccrf_input_queues[i] = &ccrf_compute_units[i].input_subtask_queue;
        }
    }

    std::thread ccrf_top_level_scheduler_thread;
    std::thread job_result_notifier_thread;
    std::thread ccrf_task_scheduler_thread;
    std::thread ccrf_dispatcher_thread;

    hls::stream<JobPackage> *job_request_queue;
    hls::stream<JOB_STATUS_MESSAGE> *job_status_queue;

    // CCRF Engine Queues
    //hls::stream<JobPackage> incoming_job_request_queue; // 1  incoming_job_request_queue
    //hls::stream<JOB_STATUS_MESSAGE> job_status_response_queue; // 2 outgoing_job_response_queue
    hls::stream<JobPackage> jobs_to_schedule_queue; // 3
    hls::stream<JOB_SUBTASK> scheduler_to_dispatcher_subtask_queue; // 4
    hls::stream<JOB_COMPLETION_PACKET> jobs_in_progress_queue; // 5
    hls::stream<JOB_COMPLETION_PACKET> completed_job_from_completion_module_queue; // 6
    hls::stream<JOB_SUBTASK> *ccrf_input_queues[CCRF_COMPUTE_UNIT_COUNT];
    hls::stream<PIXEL_T*> *ccrf_output_queues[CCRF_COMPUTE_UNIT_COUNT]; // 7 +

    //std::vector<SoftwareDummyCCRF> ccrf_compute_units;
    SoftwareDummyCCRF ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT];


    bool already_started;

};

#endif
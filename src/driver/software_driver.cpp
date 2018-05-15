#include "software_driver.hpp"
#include "ccrf_scheduler.hpp"
#include "helper.hpp"
#include "types.hpp"
#include "ccrf.hpp"
#include "software_test_ccrf.hpp"

#include <iostream>
void SoftwareTestDriver::SendJobLaunchRequest(JobPackage job_request)
{
    if (!already_started) {
        Start();
    }
}

void SoftwareTestDriver::Start()
{


    ccrf_top_level_scheduler_thread = std::thread(CcrfSchedulerTopLevel, 
                                                  std::ref(incoming_job_request_queue), 
                                                  std::ref(outgoing_job_response_queue),
                                                  std::ref(jobs_to_schedule_queue),
                                                  std::ref(completed_job_from_completion_module_queue));
    
    
     ccrf_task_scheduler_thread = std::thread(CcrfSubtaskScheduler, 
                                              std::ref(jobs_to_schedule_queue), 
                                              std::ref(scheduler_to_dispatcher_subtask_queue), 
                                              std::ref(jobs_in_progress_queue));
    
    
    ccrf_dispatcher_thread = std::thread(CcrfSubtaskDispatcher, 
                                         std::ref(scheduler_to_dispatcher_subtask_queue), 
                                         std::ref(ccrf_unit_status_signals),
                                         #ifdef HW_COMPILE
                                         std::ref(ccrf_input_queues_1),
                                         std::ref(ccrf_input_queues_2),
                                         std::ref(ccrf_input_queues_3),
                                         std::ref(ccrf_input_queues_4),
                                         std::ref(ccrf_input_queues_5),
                                         std::ref(ccrf_input_queues_6)
                                         #else
                                         std::ref(ccrf_input_queues)
                                         #endif
                                         );
    
    
    job_result_notifier_thread = std::thread(JobResultNotifier,
                                             std::ref(completed_job_from_completion_module_queue), 
                                             std::ref(jobs_in_progress_queue),
                                             std::ref(ccrf_unit_status_signals),
                                             #ifdef HW_COMPILE
                                             std::ref(ccrf_output_queues_1),
                                             std::ref(ccrf_output_queues_2),
                                             std::ref(ccrf_output_queues_3),
                                             std::ref(ccrf_output_queues_4),
                                             std::ref(ccrf_output_queues_5),
                                             std::ref(ccrf_output_queues_6)
                                             #else
                                             std::ref(ccrf_output_queues)
                                             #endif
                                             );
    
    #ifdef HW_COMPILE
    ccrf_threads[0] = std::thread(Run_SoftwareDummyCCRF,
                                  std::ref(ccrf_unit_status_signals[0]),
                                  std::ref(ccrf_input_queues_1),
                                  std::ref(ccrf_output_queues_1));
    ccrf_threads[1] = std::thread(Run_SoftwareDummyCCRF,
                                  std::ref(ccrf_unit_status_signals[1]),
                                  std::ref(ccrf_input_queues_2),
                                  std::ref(ccrf_output_queues_2));
    ccrf_threads[2] = std::thread(Run_SoftwareDummyCCRF,
                                  std::ref(ccrf_unit_status_signals[2]),
                                  std::ref(ccrf_input_queues_3),
                                  std::ref(ccrf_output_queues_3));
    ccrf_threads[3] = std::thread(Run_SoftwareDummyCCRF,
                                  std::ref(ccrf_unit_status_signals[3]),
                                  std::ref(ccrf_input_queues_4),
                                  std::ref(ccrf_output_queues_4));
    ccrf_threads[4] = std::thread(Run_SoftwareDummyCCRF,
                                  std::ref(ccrf_unit_status_signals[4]),
                                  std::ref(ccrf_input_queues_5),
                                  std::ref(ccrf_output_queues_5));
    ccrf_threads[5] = std::thread(Run_SoftwareDummyCCRF,
                                  std::ref(ccrf_unit_status_signals[5]),
                                  std::ref(ccrf_input_queues_6),
                                  std::ref(ccrf_output_queues_6));
    #else
    for (int i = 0; i < default_ccrf_unit_count; i++) {
        ccrf_threads[i] = std::thread(Run_SoftwareDummyCCRF,
                                      std::ref(ccrf_unit_status_signals[i]),
                                      std::ref(ccrf_input_queues[i]),
                                      std::ref(ccrf_output_queues[i]));
    }
    #endif

}


SoftwareTestDriver::~SoftwareTestDriver()
{
    try {
        ccrf_top_level_scheduler_thread.join();
        job_result_notifier_thread.join();
        ccrf_task_scheduler_thread.join();
        ccrf_dispatcher_thread.join();

        for (int i = 0; i < default_ccrf_unit_count; i++) {
            ccrf_threads[i].join();
        }

    } catch (...) {
        ASSERT(false, "couldn't stop the software test driver threads...");
    }
}
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

template <typename T>
void dummy_function(T &num) {
    while(!num.empty()) {
        std::cout << "not_empty" << std::endl;
        num.read();
    }
     std::cout << "done dummy_function" << std::endl;
    // for (; *num >= 0; (*num)--) {
    //     std::cout << *num << std::endl;
    // }
}

void SoftwareTestDriver::Start()
{
    int x = 5;
    //ccrf_top_level_scheduler_thread = std::thread(dummy_function<hls::stream<JobPackage>>,
    //                                              std::ref(incoming_job_request_queue));
    
    
    ccrf_top_level_scheduler_thread = std::thread(CcrfSchedulerTopLevel, 
                                                   std::ref(incoming_job_request_queue), 
                                                   std::ref(outgoing_job_response_queue),
                                                   std::ref(jobs_to_schedule_queue),
                                                   std::ref(completed_job_from_completion_module_queue));
    
    
     ccrf_task_scheduler_thread = std::thread(CcrfSubtaskScheduler, 
                                                 std::ref(jobs_to_schedule_queue), 
                                                 std::ref(scheduler_to_dispatcher_subtask_queue), 
                                                 std::ref(jobs_in_progress_queue));
    
    
    ccrf_dispatcher_thread = std::thread(CcrfSubtaskDispatcher<SoftwareDummyCCRF>, 
                                            std::ref(scheduler_to_dispatcher_subtask_queue), 
                                            ccrf_compute_units);
    
    
    job_result_notifier_thread = std::thread(JobResultNotifier<SoftwareDummyCCRF>,
                                             std::ref(completed_job_from_completion_module_queue), 
                                             std::ref(jobs_in_progress_queue), 
                                             ccrf_compute_units);
    
    
    for (int i = 0; i < default_ccrf_unit_count; i++) {
        ccrf_compute_units[i].Initialize();
    }

}


SoftwareTestDriver::~SoftwareTestDriver()
{
    try {
        ccrf_top_level_scheduler_thread.join();
        job_result_notifier_thread.join();
        ccrf_task_scheduler_thread.join();
        ccrf_dispatcher_thread.join();
    } catch (...) {
        ASSERT(false, "couldn't stop the software test driver threads...");
    }
}
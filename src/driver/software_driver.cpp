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

void SoftwareTestDriver::RunCcrfCsim() 
{
    while(1) {
        CcrfWrapper(std::ref(incoming_job_request_queue), std::ref(outgoing_job_response_queue));
        /*,
        (BYTE_T *const)nullptr);// Treat uintptrs as raw pointers (not address offsets) May need to change this in the future
        */
    }
}

void SoftwareTestDriver::Start()
{
    ccrf_wrapper_csim_thread = std::thread(&SoftwareTestDriver::RunCcrfCsim, this);
}


SoftwareTestDriver::~SoftwareTestDriver()
{
}
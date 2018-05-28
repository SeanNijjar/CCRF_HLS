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
    hls::stream<JobPackage> to_ccrf_wrapper;
    hls::stream<JOB_STATUS_MESSAGE_AXI> from_ccrf_wrapper;
    while(1) {
        if (incoming_job_request_queue.size()) {
            to_ccrf_wrapper.write(incoming_job_request_queue.front());
            incoming_job_request_queue.erase(incoming_job_request_queue.begin());
        }

        CcrfWrapper(std::ref(to_ccrf_wrapper), std::ref(from_ccrf_wrapper));

        if (!from_ccrf_wrapper.empty()) {
            JOB_STATUS_MESSAGE_AXI response_message;
            response_message = from_ccrf_wrapper.read();
            JOB_STATUS_MESSAGE status_message;
            status_message.packet_message_type = response_message.data & 0xFF;
            status_message.job_ID = (response_message.data >> 8) & 0xFF;
            outgoing_job_response_queue.push_back(status_message);
        }
    }
}

void SoftwareTestDriver::Start()
{
    ccrf_wrapper_csim_thread = std::thread(&SoftwareTestDriver::RunCcrfCsim, this);
}


SoftwareTestDriver::~SoftwareTestDriver()
{
}
#ifndef DRIVER_H
#define DRIVER_H

#include "job_package.hpp"
#include "types.hpp"
#include <hls_stream.h>

class Driver
{
  public:
    Driver(hls::stream<JobPackage> &incoming_queue, hls::stream<JOB_STATUS_MESSAGE> &outgoing_queue) :
        incoming_job_request_queue(incoming_queue), outgoing_job_response_queue(outgoing_queue)
    {
    }

  public:
    hls::stream<JobPackage> &incoming_job_request_queue;
    hls::stream<JOB_STATUS_MESSAGE> &outgoing_job_response_queue;
};


class ZynqHardwareDriver : public Driver
{

};

#endif 
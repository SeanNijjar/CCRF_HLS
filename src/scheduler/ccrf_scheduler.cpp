#include "ccrf_scheduler.hpp"
#include "job_descriptor.hpp"
#include "job_package.hpp"
#include "ccrf.hpp"
#include "utils.hpp"
#include <hls_stream.h>

using namespace hls;


PIXEL_T *CCRF_SCRATCHPAD_START_ADDR = (PIXEL_T*)(0x10000000);




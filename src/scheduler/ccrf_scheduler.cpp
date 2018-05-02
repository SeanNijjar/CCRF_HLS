#include "ccrf_scheduler.hpp"
#include "job_descriptor.hpp"
#include "ccrf.hpp"
#include "utils.hpp"
#include <hls_stream.h>

using namespace hls;


PIXEL_T *CCRF_SCRATCHPAD_START_ADDR = (PIXEL_T*)(0x10000000);

bool DoesTaskWaitForDependencies(JOB_SUBTASK task_to_check, CCRF ccrf_compute_units[CCRF_COMPUTE_UNIT_COUNT]) {
    const int dependency_count = 3;
    typedef void* dependency_type;
    const PIXEL_T *dependencies[3] = {0,};
    dependencies[0] = task_to_check.input1;
    dependencies[1] = task_to_check.input2;
    dependencies[2] = task_to_check.output;
    for (int i = 0; i < dependency_count; i++) {
        const PIXEL_T *task_dependence = dependencies[i];
        for (int ccrf_unit = 0; ccrf_unit < CCRF_COMPUTE_UNIT_COUNT; ccrf_unit) {    
            if (ccrf_compute_units[ccrf_unit].GetTaskDependence(i) == task_dependence) {
                return true;
            }
        }
    }
    return false;
}



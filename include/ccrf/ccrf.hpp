#ifndef CCRF_H
#define CCRF_H

#include "types.hpp"
#include <assert.h>

class CCRF 
{
  public:
    const bool is_idle() const
    {
        UNIMPLEMENTED(); // Won't complain but just for reading
    }

    const PIXEL_T *const GetTaskDependence(const int i) const
    {
        assert(i > 0 && i < 3);
        
        switch(i) {
            case 0:
                return ccrf_task_details.input1;
            case 1:
                return ccrf_task_details.input2;
            case 2:
                return ccrf_task_details.output;
        };
    }

    const void run(JOB_SUBTASK task_to_run)
    {
        UNIMPLEMENTED();
    }

  private:
    JOB_SUBTASK ccrf_task_details;
};

#endif
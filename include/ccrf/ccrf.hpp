#ifndef CCRF_H
#define CCRF_H

#include "types.hpp"
#include <assert.h>

class CCRF 
{
  public:
    const PIXEL_T *const GetTaskDependence(const int i) {
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

    

  private:
    JOB_SUBTASK ccrf_task_details;
};

#endif
#ifndef CCRF_H
#define CCRF_H

#include "types.hpp"
#include "helper.hpp"

#include <hls_stream.h>


struct CCRF_UNIT_STATUS_SIGNALS
{
    JOB_SUBTASK job_info;
    // uintptr_t task_dep_ptr1 = (uintptr_t)nullptr;
    // uintptr_t task_dep_ptr2 = (uintptr_t)nullptr;
    // uintptr_t task_dep_ptr3 = (uintptr_t)nullptr;
    // JOB_ID_T associated_job = 0;
    // int image_size;
    bool is_processing = false;
    bool running = false;

};

typedef struct {
    BYTE_T data[16];
} WIDE_DATA_T;

const int MEMORY_SIZE = 2000000000;
const int CCRF_LUT_SIZE = 65536;

void Run_CCRF(CCRF_UNIT_STATUS_SIGNALS &status_signals,
              hls::stream<JOB_SUBTASK> &input_subtask_queue,
              hls::stream<uintptr_t> &output_subtask_queue,


			  const BYTE_T CCRF_LUT_BLUE0[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_GREEN0[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_RED0[CCRF_LUT_SIZE],

			  const BYTE_T CCRF_LUT_BLUE1[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_GREEN1[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_RED1[CCRF_LUT_SIZE],

			  const BYTE_T CCRF_LUT_BLUE2[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_GREEN2[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_RED2[CCRF_LUT_SIZE],

			  const BYTE_T CCRF_LUT_BLUE3[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_GREEN3[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_RED3[CCRF_LUT_SIZE],

			  const BYTE_T CCRF_LUT_BLUE4[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_GREEN4[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_RED4[CCRF_LUT_SIZE],

			  const BYTE_T CCRF_LUT_BLUE5[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_GREEN5[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_RED5[CCRF_LUT_SIZE],

			  const BYTE_T CCRF_LUT_BLUE6[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_GREEN6[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_RED6[CCRF_LUT_SIZE],

			  const BYTE_T CCRF_LUT_BLUE7[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_GREEN7[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_RED7[CCRF_LUT_SIZE],

			  const BYTE_T CCRF_LUT_BLUE8[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_GREEN8[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_RED8[CCRF_LUT_SIZE],

			  const BYTE_T CCRF_LUT_BLUE9[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_GREEN9[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_RED9[CCRF_LUT_SIZE],

			  const BYTE_T CCRF_LUT_BLUE10[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_GREEN10[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_RED10[CCRF_LUT_SIZE],

			  const BYTE_T CCRF_LUT_BLUE11[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_GREEN11[CCRF_LUT_SIZE],
			  const BYTE_T CCRF_LUT_RED11[CCRF_LUT_SIZE],
			  BYTE_T memory[MEMORY_SIZE],

              bool &ccrf_got_data,
              bool &ccrf_sent_data,
              const WIDE_DATA_T * memory_bus
              );





#endif

#ifndef CCRF_H
#define CCRF_H

#include "types.hpp"
#include "helper.hpp"

#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>

#define BURST_LENGTH 64
typedef ap_uint<8> PIXEL;
typedef ap_uint<16> LUT_entry;
typedef ap_uint<32> hlsuint32;
typedef uintptr_t BUS_addr;

typedef ap_uint<512> PLDDR_BUS_FLAT;

typedef struct {
	PIXEL pixel[64];
} PLDDR_BUS;
typedef struct {
	PIXEL ROM[256*256];
} CCRF_LUT;


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

typedef ap_axis<sizeof(JOB_SUBTASK)*8,1,1,1> JOB_SUBTASK_AXI;

typedef ap_uint</*sizeof(JOB_COMPLETION_PACKET)*/(8+4+4+1) *8> JOB_COMPLETION_PACKET_FLAT;

typedef struct {
    BYTE_T data[64];
} WIDE_DATA_T;

typedef ap_uint<sizeof(WIDE_DATA_T)*8> WIDE_DATA_FLAT_T;

static_assert(sizeof(WIDE_DATA_T) == sizeof(WIDE_DATA_FLAT_T), "type size mismatch");

typedef struct {
	uint32_t data[16];
} WIDE_PIXEL_WIDE_DATA_T;

const int MEMORY_SIZE = 2000000000;
const int CCRF_LUT_SIZE = 65536;
const int INVERSE_CRF_LUT_SIZE = 256;

#define CCRF_LUT_BUNDLE(N) \
	const BYTE_T CCRF_LUT_BLUE##N [CCRF_LUT_SIZE],\
	const BYTE_T CCRF_LUT_GREEN##N [CCRF_LUT_SIZE],\
    const BYTE_T CCRF_LUT_RED##N [CCRF_LUT_SIZE]

#define INVERSE_CRF_LUT_BUNDLE(N) \
		const uint32_t INVERSE_CRF_LUT_BLUE##N [INVERSE_CRF_LUT_SIZE],\
		const uint32_t INVERSE_CRF_LUT_GREEN##N [INVERSE_CRF_LUT_SIZE],\
	    const uint32_t INVERSE_CRF_LUT_RED##N [INVERSE_CRF_LUT_SIZE]


void InverseCRF(
		hls::stream<JOB_COMPLETION_PACKET_FLAT> &CCRF_completed_job,
		hls::stream<JOB_COMPLETION_PACKET_FLAT> &completed_packet_out,
		WIDE_DATA_FLAT_T *memory_bus,
		INVERSE_CRF_LUT_BUNDLE(0),
		INVERSE_CRF_LUT_BUNDLE(1),
		INVERSE_CRF_LUT_BUNDLE(2),
		INVERSE_CRF_LUT_BUNDLE(3)
);

void CCRF_Compute(
		PLDDR_BUS_FLAT * plmem,

	    hls::stream<JOB_SUBTASK_AXI> &job_info_in_queue,
		//hls::stream<JOB_SUBTASK> &job_info_in_queue,
		hls::stream<bool> &job_done_stream,

		const CCRF_LUT CCRF_LUT_BLUE[16],
		const CCRF_LUT CCRF_LUT_GREEN[16],
		const CCRF_LUT CCRF_LUT_RED[16]);
/*
void CCRF_Compute(
	CCRF_LUT_BUNDLE(0),
	CCRF_LUT_BUNDLE(1),
	CCRF_LUT_BUNDLE(2),
	CCRF_LUT_BUNDLE(3),
	CCRF_LUT_BUNDLE(4),
	CCRF_LUT_BUNDLE(5),
	CCRF_LUT_BUNDLE(6),
	CCRF_LUT_BUNDLE(7),
	CCRF_LUT_BUNDLE(8),
	CCRF_LUT_BUNDLE(9),
	CCRF_LUT_BUNDLE(10),
	CCRF_LUT_BUNDLE(11),
	CCRF_LUT_BUNDLE(12),
	CCRF_LUT_BUNDLE(13),
	CCRF_LUT_BUNDLE(14),
	CCRF_LUT_BUNDLE(15),

	WIDE_DATA_FLAT_T *memory_bus,

    hls::stream<JOB_SUBTASK_AXI> &job_info_in_queue,
	hls::stream<bool> &job_done_stream
) ;
*/

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

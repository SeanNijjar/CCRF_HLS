#include "ccrf.hpp"

#include <ap_utils.h>


void CCRF_Compute(const BYTE_T CCRF_LUT_BLUE0[CCRF_LUT_SIZE],
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

			      WIDE_DATA_T *memory_bus,//[MEMORY_SIZE],
				  WIDE_DATA_T *memory_bus2,

    			  JOB_SUBTASK &job_info_in,
	    		  bool &job_valid_in,
		    	  bool &job_done_out
) {
    #pragma HLS INTERFACE bram port=CCRF_LUT_BLUE0
    #pragma HLS INTERFACE bram port=CCRF_LUT_GREEN0
    #pragma HLS INTERFACE bram port=CCRF_LUT_RED0
    #pragma HLS INTERFACE bram port=CCRF_LUT_BLUE1
    #pragma HLS INTERFACE bram port=CCRF_LUT_GREEN1
    #pragma HLS INTERFACE bram port=CCRF_LUT_RED1
    #pragma HLS INTERFACE bram port=CCRF_LUT_BLUE2
    #pragma HLS INTERFACE bram port=CCRF_LUT_GREEN2
    #pragma HLS INTERFACE bram port=CCRF_LUT_RED2
    #pragma HLS INTERFACE bram port=CCRF_LUT_BLUE3
    #pragma HLS INTERFACE bram port=CCRF_LUT_GREEN3
    #pragma HLS INTERFACE bram port=CCRF_LUT_RED3

    //#pragma HLS INTERFACE m_axi port=memory_bus depth=2147483648
	//#pragma HLS RESOURCE core=m_axi variable=memory_bus
    #pragma HLS INTERFACE m_axi port=memory_bus depth=2147483648
    #pragma HLS DATA_PACK variable=memory_bus
    #pragma HLS INTERFACE m_axi port=memory_bus2 depth=2147483648
    #pragma HLS DATA_PACK variable=memory_bus2

    #pragma HLS INTERFACE ap_none port=job_info_in
    #pragma HLS INTERFACE ap_none port=job_valid_in
    #pragma HLS INTERFACE ap_none port=job_done_out
    #pragma HLS DATA_PACK variable=job_info_in

	static bool job_done_internal = false;

    job_done_out = job_done_internal;

    if (job_done_internal) {
    	job_done_internal = false;
    } else if (job_valid_in){
        ///////////////////////////////////////////////////////////////////
        //                  INSERT YOUR CCRF LOGIC HERE                  //
        ///////////////////////////////////////////////////////////////////
	int in1_offset = job_info_in.input1;
	int in2_offset = job_info_in.input2;
	int out_offset = job_info_in.output;
	int iteration_count = job_info_in.image_size/4;
	for (int i = 0; i < iteration_count; i++) {
	    #pragma HLS pipeline
        const int input1_pixel_offset = in1_offset+i;
        const int input2_pixel_offset = in2_offset+i;
        const int output_pixel_offset = out_offset+i;

        WIDE_DATA_T input1_wide_pixel = memory_bus[input1_pixel_offset];
        #pragma HLS DATA_PACK variable=input1_wide_pixel
        WIDE_DATA_T input2_wide_pixel = memory_bus2[input2_pixel_offset];
        #pragma HLS DATA_PACK variable=input2_wide_pixel
        WIDE_DATA_T output_wide_pixel;
        #pragma HLS DATA_PACK variable=output_wide_pixel

	    BYTE_T B1_0 = input1_wide_pixel.data[0];
	    BYTE_T G1_0 = input1_wide_pixel.data[1];
	    BYTE_T R1_0 = input1_wide_pixel.data[2];
        BYTE_T B1_1 = input1_wide_pixel.data[4];
        BYTE_T G1_1 = input1_wide_pixel.data[5];
        BYTE_T R1_1 = input1_wide_pixel.data[6];
        BYTE_T B1_2 = input1_wide_pixel.data[8];
        BYTE_T G1_2 = input1_wide_pixel.data[9];
        BYTE_T R1_2 = input1_wide_pixel.data[10];
        BYTE_T B1_3 = input1_wide_pixel.data[12];
        BYTE_T G1_3 = input1_wide_pixel.data[13];
        BYTE_T R1_3 = input1_wide_pixel.data[14];

        BYTE_T B2_0 = input2_wide_pixel.data[0];
        BYTE_T G2_0 = input2_wide_pixel.data[1];
        BYTE_T R2_0 = input2_wide_pixel.data[2];
        BYTE_T B2_1 = input2_wide_pixel.data[4];
        BYTE_T G2_1 = input2_wide_pixel.data[5];
        BYTE_T R2_1 = input2_wide_pixel.data[6];
        BYTE_T B2_2 = input2_wide_pixel.data[8];
        BYTE_T G2_2 = input2_wide_pixel.data[9];
        BYTE_T R2_2 = input2_wide_pixel.data[10];
        BYTE_T B2_3 = input2_wide_pixel.data[12];
        BYTE_T G2_3 = input2_wide_pixel.data[13];
        BYTE_T R2_3 = input2_wide_pixel.data[14];

	    uint16_t LUT_entry_b0 = (uint16_t)B1_0 + (uint16_t)B2_0 * 256;
        uint16_t LUT_entry_g0 = (uint16_t)G1_0 + (uint16_t)G2_0 * 256;
        uint16_t LUT_entry_r0 = (uint16_t)R1_0 + (uint16_t)R2_0 * 256;
        uint16_t LUT_entry_b1 = (uint16_t)B1_1 + (uint16_t)B2_1 * 256;
        uint16_t LUT_entry_g1 = (uint16_t)G1_1 + (uint16_t)G2_1 * 256;
        uint16_t LUT_entry_r1 = (uint16_t)R1_1 + (uint16_t)R2_1 * 256;
        uint16_t LUT_entry_b2 = (uint16_t)B1_2 + (uint16_t)B2_2 * 256;
        uint16_t LUT_entry_g2 = (uint16_t)G1_2 + (uint16_t)G2_2 * 256;
        uint16_t LUT_entry_r2 = (uint16_t)R1_2 + (uint16_t)R2_2 * 256;
        uint16_t LUT_entry_b3 = (uint16_t)B1_3 + (uint16_t)B2_3 * 256;
        uint16_t LUT_entry_g3 = (uint16_t)G1_3 + (uint16_t)G2_3 * 256;
        uint16_t LUT_entry_r3 = (uint16_t)R1_3 + (uint16_t)R2_3 * 256;

        output_wide_pixel.data[0] = CCRF_LUT_BLUE0[LUT_entry_b0];
        output_wide_pixel.data[1] = CCRF_LUT_BLUE1[LUT_entry_b1];
        output_wide_pixel.data[2] = CCRF_LUT_BLUE2[LUT_entry_b2];
        output_wide_pixel.data[4] = CCRF_LUT_BLUE3[LUT_entry_b3];
        output_wide_pixel.data[5] = CCRF_LUT_GREEN0[LUT_entry_g0];
        output_wide_pixel.data[6] = CCRF_LUT_GREEN1[LUT_entry_g1];
        output_wide_pixel.data[8] = CCRF_LUT_GREEN2[LUT_entry_g2];
        output_wide_pixel.data[9] = CCRF_LUT_GREEN3[LUT_entry_g3];
        output_wide_pixel.data[10] = CCRF_LUT_RED0[LUT_entry_r0];
        output_wide_pixel.data[12] = CCRF_LUT_RED1[LUT_entry_r1];
        output_wide_pixel.data[13] = CCRF_LUT_RED2[LUT_entry_r2];
        output_wide_pixel.data[14] = CCRF_LUT_RED3[LUT_entry_r3];

	    memory_bus[output_pixel_offset] = output_wide_pixel;

        }
        ///////////////////////////////////////////////////////////////////
	    job_done_internal = true;
    }
    job_done_out = job_done_internal;

}


void Run_CCRF(CCRF_UNIT_STATUS_SIGNALS &status_signals,
              hls::stream<JOB_SUBTASK> &input_subtask_queue,
              hls::stream<uintptr_t> &output_subtask_queue,

			  JOB_SUBTASK &job_info_out,
			  bool &job_valid,
			  bool &job_done,

              bool &ccrf_got_data,
              bool &ccrf_sent_data
                          )
{
    #pragma HLS STREAM variable=input_subtask_queue depth=1
    #pragma HLS STREAM variable=output_subtask_queue depth=1
    #pragma HLS INTERFACE ap_none port=status_signals
    #pragma HLS DATA_PACK variable=input_subtask_queue
    #pragma HLS DATA_PACK variable=status_signals struct_level
    //#pragma HLS DATA_PACK variable=output_subtask_queue struct_level
    //#pragma HLS INTERFACE axis port=input_subtask_queue
    //#pragma HLS INTERFACE axis port=output_subtask_queue
    #pragma HLS RESOURCE core=axis variable=input_subtask_queue
    #pragma HLS RESOURCE core=axis variable=output_subtask_queue

    #pragma HLS INTERFACE ap_ctrl_none port=return
	#pragma HLS INTERFACE ap_none port=job_valid
    #pragma HLS INTERFACE ap_none port=job_done
    #pragma HLS INTERFACE ap_none port=job_info_out
    #pragma HLS DATA_PACK variable=job_info_out
    #pragma HLS INTERFACE ap_none port=ccrf_got_data
    #pragma HLS INTERFACE ap_none port=ccrf_sent_data

	enum CTRL_STATE {
		START,
		WAIT_INPUT,
		SET_JOB_VALID,
		WAIT_JOB_DONE,
		WRITE_OUTPUT
	};

	static CTRL_STATE ctrl_state = START;
	CTRL_STATE ctrl_state_next = START;

	static CCRF_UNIT_STATUS_SIGNALS status_signals_internal;
    static bool started = false;
    ccrf_got_data = !input_subtask_queue.empty();
    ccrf_sent_data = false;

    static bool job_valid_internal = false;
    static JOB_SUBTASK job_info_internal;

	switch (ctrl_state) {
	case START:
		job_valid_internal = false;
    	status_signals_internal.is_processing = false;
    	status_signals_internal.running = true;
    	status_signals_internal.job_info.input1 = (uintptr_t)nullptr;
    	status_signals_internal.job_info.input2 = (uintptr_t)nullptr;
    	status_signals_internal.job_info.output = (uintptr_t)nullptr;
    	status_signals_internal.job_info.image_size = 0;
        status_signals_internal.job_info.job_ID = 0;
		ctrl_state_next = WAIT_INPUT;
		break;

	case WAIT_INPUT:
		job_valid_internal = false;
		if (!input_subtask_queue.empty()) {
			ctrl_state_next = SET_JOB_VALID;
		} else {
			ctrl_state_next = WAIT_INPUT;
		}
		break;


	case SET_JOB_VALID:
		status_signals_internal.is_processing = true;
		job_info_internal = input_subtask_queue.read();
		status_signals_internal.job_info = job_info_internal;
		job_valid_internal = true;
		ctrl_state_next = WAIT_JOB_DONE;
		break;

	case WAIT_JOB_DONE:
		job_valid_internal = true;
		if (job_done) {
			ctrl_state_next = WRITE_OUTPUT;
		} else {
			ctrl_state_next = WAIT_JOB_DONE;
		}
		break;

	case WRITE_OUTPUT:
		output_subtask_queue.write(job_info_internal.output);
		ctrl_state_next = START;
        break;
	};

	job_info_out = job_info_internal;
    status_signals = status_signals_internal;

    ctrl_state = ctrl_state_next;

    /*
    if (!started) {
        #pragma HLS UNROLL
    	status_signals_internal.is_processing = false;
    	status_signals_internal.running = true;
    	status_signals_internal.job_info.input1 = (uintptr_t)nullptr;
    	status_signals_internal.job_info.input2 = (uintptr_t)nullptr;
    	status_signals_internal.job_info.output = (uintptr_t)nullptr;
    	status_signals_internal.job_info.image_size = 0;
        status_signals_internal.job_info.job_ID = 0;
        started = true;
        job_valid_internal = false;
    }

    if (job_valid_internal) {
        if (job_done) {
        	status_signals_internal.job_info.input1 = (uintptr_t)nullptr;
        	status_signals_internal.job_info.input2 = (uintptr_t)nullptr;
        	status_signals_internal.job_info.output = (uintptr_t)nullptr;
        	status_signals_internal.job_info.image_size = 0;
        	status_signals_internal.job_info.job_ID = 0;
        	status_signals_internal.is_processing = false;
            ccrf_sent_data = true;
            output_subtask_queue.write(job_info_internal.output);
            job_valid_internal = false;
        }
        job_valid_internal = !job_done;
    } else if (!input_subtask_queue.empty()) {
        ASSERT(!status_signals.is_processing, "Tried to start a new job on an already busy CCRF unit");
        status_signals_internal.is_processing = true;
        job_info_internal = input_subtask_queue.read();
        status_signals_internal.job_info = job_info_internal;
        job_valid_internal = true;
    }
    */

    status_signals = status_signals_internal;

    job_info_out = job_info_internal;
    job_valid = job_valid_internal;
}

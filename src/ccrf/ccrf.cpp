#include "ccrf.hpp"


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
              bool &ccrf_sent_data
                          )
//,              BYTE_T *const memory_bus)
{
    //#pragma HLS INTERFACE m_axi port=memory_bus depth=2147483648 offset=slave
    #pragma HLS STREAM variable=input_subtask_queue depth=1
    #pragma HLS STREAM variable=output_subtask_queue depth=1
    #pragma HLS INTERFACE ap_none port=status_signals
    #pragma HLS DATA_PACK variable=input_subtask_queue struct_level
    #pragma HLS DATA_PACK variable=status_signals struct_level
    #pragma HLS DATA_PACK variable=output_subtask_queue struct_level
    #pragma HLS RESOURCE core=axis variable=input_subtask_queue
    #pragma HLS RESOURCE core=axis variable=output_subtask_queue
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_BLUE0
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_GREEN0
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_RED0
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_BLUE1
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_GREEN1
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_RED1
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_BLUE2
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_GREEN2
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_RED2
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_BLUE3
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_GREEN3
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_RED3
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_BLUE4
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_GREEN4
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_RED4
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_BLUE5
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_GREEN5
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_RED5
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_BLUE6
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_GREEN6
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_RED6
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_BLUE7
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_GREEN7
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_RED7
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_BLUE8
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_GREEN8
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_RED8
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_BLUE9
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_GREEN9
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_RED9
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_BLUE10
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_GREEN10
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_RED10
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_BLUE11
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_GREEN11
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_RED11
    #pragma HLS INTERFACE ap_memory port=memory

    #pragma HLS INTERFACE ap_none port=ccrf_got_data
    #pragma HLS INTERFACE ap_none port=ccrf_sent_data
    bool started = false;
    ccrf_got_data = !input_subtask_queue.empty();
    ccrf_sent_data = false;

    const int LUT_PORT_COUNT = 12;

    const BYTE_T* blue_ports[LUT_PORT_COUNT] = {CCRF_LUT_BLUE0, CCRF_LUT_BLUE1,
    		                      CCRF_LUT_BLUE2, CCRF_LUT_BLUE3,
								  CCRF_LUT_BLUE4, CCRF_LUT_BLUE5,
								  CCRF_LUT_BLUE6, CCRF_LUT_BLUE7,
								  CCRF_LUT_BLUE8, CCRF_LUT_BLUE9,
								  CCRF_LUT_BLUE10, CCRF_LUT_BLUE11};

    const BYTE_T*green_ports[LUT_PORT_COUNT] = {CCRF_LUT_GREEN0, CCRF_LUT_GREEN1,
        		                  CCRF_LUT_GREEN2, CCRF_LUT_GREEN3,
    							  CCRF_LUT_GREEN4, CCRF_LUT_GREEN5,
    							  CCRF_LUT_GREEN6, CCRF_LUT_GREEN7,
    							  CCRF_LUT_GREEN8, CCRF_LUT_GREEN9,
    							  CCRF_LUT_GREEN10, CCRF_LUT_GREEN11};

    const BYTE_T* red_ports[LUT_PORT_COUNT] = {CCRF_LUT_RED0, CCRF_LUT_RED1,
        		                 CCRF_LUT_RED2, CCRF_LUT_RED3,
    							 CCRF_LUT_RED4, CCRF_LUT_RED5,
    							 CCRF_LUT_RED6, CCRF_LUT_RED7,
    							 CCRF_LUT_RED8, CCRF_LUT_RED9,
    							 CCRF_LUT_RED10, CCRF_LUT_RED11};

    if (!started) {
        #pragma HLS UNROLL
        status_signals.is_processing = false;
        status_signals.running = true;
        status_signals.job_info.input1 = (uintptr_t)nullptr;
        status_signals.job_info.input2 = (uintptr_t)nullptr;
        status_signals.job_info.output = (uintptr_t)nullptr;
        status_signals.job_info.image_size = 0;
        status_signals.job_info.job_ID = 0;
        started = true;
    }

        if (!input_subtask_queue.empty()) {
            ASSERT(!status_signals.is_processing, "Tried to start a new job on an already busy CCRF unit");
            status_signals.is_processing = true;
            status_signals.job_info = input_subtask_queue.read();


            ///////////////////////////////////////////////////////////////////
            //                  INSERT YOUR CCRF LOGIC HERE                  //
            ///////////////////////////////////////////////////////////////////
            memory[status_signals.job_info.output] = memory[status_signals.job_info.input1] + memory[status_signals.job_info.input2] / 2;

            BYTE_T * input1 = &memory[status_signals.job_info.input1];
            BYTE_T * input2 = &memory[status_signals.job_info.input2];
            BYTE_T * output = &memory[status_signals.job_info.output];
            #pragma HLS INTERFACE ap_memory port=input1
            #pragma HLS INTERFACE ap_memory port=input2
            #pragma HLS INTERFACE ap_memory port=output
            for (int i = 0; i < status_signals.job_info.image_size; i++) {

            	BYTE_T blue_result = ((uint32_t)input1[i * 3 + 0] + (uint32_t)input2[i * 3 + 0]);
            	BYTE_T green_result = ((uint32_t)input1[i * 3 + 1] + (uint32_t)input2[i * 3 + 1]);
            	BYTE_T red_result = ((uint32_t)input1[i * 3 + 2] + (uint32_t)input2[i * 3 + 2]);
            	for (int port = 0; port < LUT_PORT_COUNT; port++) {
            		blue_result += blue_ports[port][i];
            		green_result += green_ports[port][i];
            		red_result += red_ports[port][i];
            	}
                output[i * 3 + 0] = blue_result / 2;
                output[i* 3 + 1] = green_result / 2;
                output[i* 3 + 2] = red_result / 2;
            }


            ///////////////////////////////////////////////////////////////////

            output_subtask_queue.write(status_signals.job_info.output);

            status_signals.job_info.input1 = (uintptr_t)nullptr;
            status_signals.job_info.input2 = (uintptr_t)nullptr;
            status_signals.job_info.output = (uintptr_t)nullptr;
            status_signals.job_info.image_size = 0;
            status_signals.job_info.job_ID = 0;
            status_signals.is_processing = false;

            ccrf_sent_data = true;
        }
}

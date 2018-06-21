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

              bool &ccrf_got_data,
              bool &ccrf_sent_data,
              WIDE_DATA_T * memory_bus)
{
    #pragma HLS INTERFACE m_axi port=memory_bus depth=2147483648
    #pragma HLS data_pack variable=memory_bus struct_level
    //#pragma HLS STREAM variable=input_subtask_queue depth=1
    //#pragma HLS STREAM variable=output_subtask_queue depth=1
    #pragma HLS INTERFACE ap_none port=status_signals
    #pragma HLS DATA_PACK variable=input_subtask_queue struct_level
//    #pragma HLS DATA_PACK variable=status_signals struct_level
 //   #pragma HLS DATA_PACK variable=output_subtask_queue struct_level
    #pragma HLS INTERFACE axis port=output_subtask_queue
    #pragma HLS INTERFACE axis port=input_subtask_queue
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

    #pragma HLS INTERFACE ap_none port=ccrf_got_data
    #pragma HLS INTERFACE ap_none port=ccrf_sent_data
    bool started = false;
    ccrf_got_data = !input_subtask_queue.empty();
    ccrf_sent_data = false;


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
//            memory[status_signals.job_info.output] = memory[status_signals.job_info.input1] + memory[status_signals.job_info.input2] / 2;

//            BYTE_T * input1 = &memory[status_signals.job_info.input1];
//            BYTE_T * input2 = &memory[status_signals.job_info.input2];
//            BYTE_T * output = &memory[status_signals.job_info.output];
//            #pragma HLS INTERFACE ap_memory port=input1
//            #pragma HLS INTERFACE ap_memory port=input2
//            #pragma HLS INTERFACE ap_memory port=output
/*            for (int i = 0; i < status_signals.job_info.image_size; i++) {

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
            }*/
	    int in1_offset = status_signals.job_info.input1;
	    int in2_offset = status_signals.job_info.input2;
	    int out_offset = status_signals.job_info.output;
	    for (int i = 0; i < status_signals.job_info.image_size/4; i++) {
	    #pragma HLS pipeline
		BYTE_T B1_0 = memory_bus[in1_offset+i].data[0];
		BYTE_T G1_0 = memory_bus[in1_offset+i].data[1];
		BYTE_T R1_0 = memory_bus[in1_offset+i].data[2];
                BYTE_T B1_1 = memory_bus[in1_offset+i].data[4];
                BYTE_T G1_1 = memory_bus[in1_offset+i].data[5];
                BYTE_T R1_1 = memory_bus[in1_offset+i].data[6];
                BYTE_T B1_2 = memory_bus[in1_offset+i].data[8];
                BYTE_T G1_2 = memory_bus[in1_offset+i].data[9];
                BYTE_T R1_2 = memory_bus[in1_offset+i].data[10];
                BYTE_T B1_3 = memory_bus[in1_offset+i].data[12];
                BYTE_T G1_3 = memory_bus[in1_offset+i].data[13];
                BYTE_T R1_3 = memory_bus[in1_offset+i].data[14];
                BYTE_T B2_0 = memory_bus[in2_offset+i].data[0];
                BYTE_T G2_0 = memory_bus[in2_offset+i].data[1];
                BYTE_T R2_0 = memory_bus[in2_offset+i].data[2];
                BYTE_T B2_1 = memory_bus[in2_offset+i].data[4];
                BYTE_T G2_1 = memory_bus[in2_offset+i].data[5];
                BYTE_T R2_1 = memory_bus[in2_offset+i].data[6];
                BYTE_T B2_2 = memory_bus[in2_offset+i].data[8];
                BYTE_T G2_2 = memory_bus[in2_offset+i].data[9];
                BYTE_T R2_2 = memory_bus[in2_offset+i].data[10];
                BYTE_T B2_3 = memory_bus[in2_offset+i].data[12];
                BYTE_T G2_3 = memory_bus[in2_offset+i].data[13];
                BYTE_T R2_3 = memory_bus[in2_offset+i].data[14];
		uint16_t LUT_entry_b0 = (uint16_t)B1_0 + (uint16_t)B2_0 * 256;
                uint16_t LUT_entry_b1 = (uint16_t)B1_1 + (uint16_t)B2_1 * 256;
                uint16_t LUT_entry_b2 = (uint16_t)B1_2 + (uint16_t)B2_2 * 256;
                uint16_t LUT_entry_b3 = (uint16_t)B1_3 + (uint16_t)B2_3 * 256;
                uint16_t LUT_entry_g0 = (uint16_t)G1_0 + (uint16_t)G2_0 * 256;
                uint16_t LUT_entry_g1 = (uint16_t)G1_1 + (uint16_t)G2_1 * 256;
                uint16_t LUT_entry_g2 = (uint16_t)G1_2 + (uint16_t)G2_2 * 256;
                uint16_t LUT_entry_g3 = (uint16_t)G1_3 + (uint16_t)G2_3 * 256;
                uint16_t LUT_entry_r0 = (uint16_t)R1_0 + (uint16_t)R2_0 * 256;
                uint16_t LUT_entry_r1 = (uint16_t)R1_1 + (uint16_t)R2_1 * 256;
                uint16_t LUT_entry_r2 = (uint16_t)R1_2 + (uint16_t)R2_2 * 256;
                uint16_t LUT_entry_r3 = (uint16_t)R1_3 + (uint16_t)R2_3 * 256;
		memory_bus[out_offset+i].data[0] = CCRF_LUT_BLUE0[LUT_entry_b0];
                memory_bus[out_offset+i].data[1] = CCRF_LUT_BLUE1[LUT_entry_b1];
                memory_bus[out_offset+i].data[2] = CCRF_LUT_BLUE2[LUT_entry_b2];
                memory_bus[out_offset+i].data[4] = CCRF_LUT_BLUE3[LUT_entry_b3];
                memory_bus[out_offset+i].data[5] = CCRF_LUT_GREEN0[LUT_entry_g0];
                memory_bus[out_offset+i].data[6] = CCRF_LUT_GREEN1[LUT_entry_g1];
                memory_bus[out_offset+i].data[8] = CCRF_LUT_GREEN2[LUT_entry_g2];
                memory_bus[out_offset+i].data[9] = CCRF_LUT_GREEN3[LUT_entry_g3];
                memory_bus[out_offset+i].data[10] = CCRF_LUT_RED0[LUT_entry_r0];
		memory_bus[out_offset+i].data[12] = CCRF_LUT_RED1[LUT_entry_r1];
		memory_bus[out_offset+i].data[13] = CCRF_LUT_RED2[LUT_entry_r2];
		memory_bus[out_offset+i].data[14] = CCRF_LUT_RED3[LUT_entry_r3];
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

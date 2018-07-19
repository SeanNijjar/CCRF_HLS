#include "ccrf.hpp"

#include <ap_utils.h>
#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <string.h>

#define CCRF_LUT_BUNDLE(N) \
	const BYTE_T CCRF_LUT_BLUE##N [CCRF_LUT_SIZE],\
	const BYTE_T CCRF_LUT_GREEN##N [CCRF_LUT_SIZE],\
    const BYTE_T CCRF_LUT_RED##N [CCRF_LUT_SIZE]

typedef ap_axis<sizeof(JOB_SUBTASK)*8,1,1,1> JOB_SUBTASK_AXI;

#define INVERSE_CRF_LUT_BUNDLE(N) \
		const uint32_t INVERSE_CRF_LUT_BLUE##N [INVERSE_CRF_LUT_SIZE],\
		const uint32_t INVERSE_CRF_LUT_GREEN##N [INVERSE_CRF_LUT_SIZE],\
	    const uint32_t INVERSE_CRF_LUT_RED##N [INVERSE_CRF_LUT_SIZE]

typedef ap_uint</*sizeof(JOB_COMPLETION_PACKET)*/(8+4+4+1) *8> JOB_COMPLETION_PACKET_FLAT;

void InverseCRF(
		hls::stream<JOB_COMPLETION_PACKET_FLAT> &CCRF_completed_job,
		hls::stream<JOB_COMPLETION_PACKET_FLAT> &completed_packet_out,
		WIDE_DATA_FLAT_T *memory_bus,
		INVERSE_CRF_LUT_BUNDLE(0),
		INVERSE_CRF_LUT_BUNDLE(1),
		INVERSE_CRF_LUT_BUNDLE(2),
		INVERSE_CRF_LUT_BUNDLE(3)
) {
    #pragma HLS INTERFACE bram port=INVERSE_CRF_LUT_BLUE0
    #pragma HLS INTERFACE bram port=INVERSE_CRF_LUT_GREEN0
    #pragma HLS INTERFACE bram port=INVERSE_CRF_LUT_RED0
    #pragma HLS INTERFACE bram port=INVERSE_CRF_LUT_BLUE1
    #pragma HLS INTERFACE bram port=INVERSE_CRF_LUT_GREEN1
    #pragma HLS INTERFACE bram port=INVERSE_CRF_LUT_RED1
    #pragma HLS INTERFACE bram port=INVERSE_CRF_LUT_BLUE2
    #pragma HLS INTERFACE bram port=INVERSE_CRF_LUT_GREEN2
    #pragma HLS INTERFACE bram port=INVERSE_CRF_LUT_RED2
    #pragma HLS INTERFACE bram port=INVERSE_CRF_LUT_BLUE3
    #pragma HLS INTERFACE bram port=INVERSE_CRF_LUT_GREEN3
    #pragma HLS INTERFACE bram port=INVERSE_CRF_LUT_RED3

    #pragma HLS INTERFACE m_axi port=memory_bus
    #pragma HLS STREAM variable=CCRF_completed_job depth=4
    #pragma HLS DATA_PACK variable=CCRF_completed_job
    #pragma HLS INTERFACE axis port=CCRF_completed_job
    #pragma HLS STREAM variable=completed_packet_out
    #pragma HLS INTERFACE axis port=completed_packet_out
    #pragma HLS DATA_PACK variable=completed_packet_out


	static WIDE_DATA_T MEM_READ_BUFFER[64];
	//static WIDE_PIXEL_WIDE_DATA_T MEM_WRITE_BUFFER[8];

	const int alignment = 4096;
	const int pixels_per_memory_read = 16;
	JOB_COMPLETION_PACKET_FLAT completed_ccrf_flat = CCRF_completed_job.read();
	JOB_COMPLETION_PACKET completed_ccrf = *(JOB_COMPLETION_PACKET*)&completed_ccrf_flat;

	uintptr_t ccrf_image_address = completed_ccrf.output_address;
	uintptr_t image_pixel_count = completed_ccrf.image_size;

	const uintptr_t ccrf_image_offset = ccrf_image_address / (pixels_per_memory_read * sizeof(PIXEL4_T));
	const uintptr_t iteration_count = image_pixel_count/pixels_per_memory_read;

	// Align the iteration to the next lowest 4096B alignment
	const int wide_reads_per_page = alignment / (pixels_per_memory_read*sizeof(PIXEL4_T));
	const int starting_i = iteration_count & ~(1-wide_reads_per_page);
	const int MAX_AXI_BURST_SIZE = 4096;
	//for (int i = starting_i; i >= 0; i -= 64) { // BURSTED LOOP
	for (int i = iteration_count - 1; 0 <= i; i--) {
      #pragma HLS PIPELINE II=4
      const uintptr_t input_pixel_offset = ccrf_image_offset + i;
      const uintptr_t output1_pixel_offset = ccrf_image_offset + (4 * i);
      const uintptr_t output2_pixel_offset = ccrf_image_offset + (4 * i) + 1;
      const uintptr_t output3_pixel_offset = ccrf_image_offset + (4 * i) + 2;
      const uintptr_t output4_pixel_offset = ccrf_image_offset + (4 * i) + 3;
      WIDE_DATA_FLAT_T input_data_flattened = memory_bus[input_pixel_offset];
	  WIDE_DATA_T input_data = *(WIDE_DATA_T*)&input_data_flattened;

	  /* BURSTED CODE COMMENTED OUT */
      //memcpy(MEM_READ_BUFFER, memory_bus[input_pixel_offset], MAX_AXI_BURST_SIZE);
      //for (int j = 0; j < 8; j++) {
        //#pragma HLS pipeline II=4
	    //WIDE_DATA_T input_data = MEM_READ_BUFFER[j];//memory_bus[input_pixel_offset];

	    WIDE_PIXEL_WIDE_DATA_T output_wide_pixel_1;
	    WIDE_PIXEL_WIDE_DATA_T output_wide_pixel_2;
	    WIDE_PIXEL_WIDE_DATA_T output_wide_pixel_3;
	    WIDE_PIXEL_WIDE_DATA_T output_wide_pixel_4;

        output_wide_pixel_1.data[0] = INVERSE_CRF_LUT_BLUE0[input_data.data[0]];
        output_wide_pixel_1.data[1] = INVERSE_CRF_LUT_GREEN0[input_data.data[1]];
        output_wide_pixel_1.data[2] = INVERSE_CRF_LUT_RED0[input_data.data[2]];
        output_wide_pixel_1.data[3] = -1;

        output_wide_pixel_1.data[4] = INVERSE_CRF_LUT_BLUE1[input_data.data[4]];
        output_wide_pixel_1.data[5] = INVERSE_CRF_LUT_GREEN1[input_data.data[5]];
        output_wide_pixel_1.data[6] = INVERSE_CRF_LUT_RED1[input_data.data[6]];
        output_wide_pixel_1.data[7] = -1;

        output_wide_pixel_1.data[8] = INVERSE_CRF_LUT_BLUE2[input_data.data[8]];
        output_wide_pixel_1.data[9] = INVERSE_CRF_LUT_GREEN2[input_data.data[9]];
        output_wide_pixel_1.data[10] = INVERSE_CRF_LUT_RED2[input_data.data[10]];
        output_wide_pixel_1.data[11] = -1;

        output_wide_pixel_1.data[12] = INVERSE_CRF_LUT_BLUE3[input_data.data[12]];
        output_wide_pixel_1.data[13] = INVERSE_CRF_LUT_GREEN3[input_data.data[13]];
        output_wide_pixel_1.data[14] = INVERSE_CRF_LUT_RED3[input_data.data[14]];
        output_wide_pixel_1.data[15] = -1;
        memory_bus[output1_pixel_offset] = *(WIDE_DATA_FLAT_T*)&output_wide_pixel_1;


        output_wide_pixel_2.data[0] = INVERSE_CRF_LUT_BLUE0[input_data.data[16]];
        output_wide_pixel_2.data[1] = INVERSE_CRF_LUT_GREEN0[input_data.data[17]];
        output_wide_pixel_2.data[2] = INVERSE_CRF_LUT_RED0[input_data.data[18]];
        output_wide_pixel_2.data[3] = -1;

        output_wide_pixel_2.data[4] = INVERSE_CRF_LUT_BLUE1[input_data.data[20]];
        output_wide_pixel_2.data[5] = INVERSE_CRF_LUT_GREEN1[input_data.data[21]];
        output_wide_pixel_2.data[6] = INVERSE_CRF_LUT_RED1[input_data.data[22]];
        output_wide_pixel_2.data[7] = -1;

        output_wide_pixel_2.data[8] = INVERSE_CRF_LUT_BLUE2[input_data.data[24]];
        output_wide_pixel_2.data[9] = INVERSE_CRF_LUT_GREEN2[input_data.data[25]];
        output_wide_pixel_2.data[10] = INVERSE_CRF_LUT_RED2[input_data.data[26]];
        output_wide_pixel_2.data[11] = -1;

        output_wide_pixel_2.data[12] = INVERSE_CRF_LUT_BLUE3[input_data.data[28]];
        output_wide_pixel_2.data[13] = INVERSE_CRF_LUT_GREEN3[input_data.data[29]];
        output_wide_pixel_2.data[14] = INVERSE_CRF_LUT_RED3[input_data.data[30]];
        output_wide_pixel_2.data[15] = -1;
        memory_bus[output2_pixel_offset] = *(WIDE_DATA_FLAT_T*)&output_wide_pixel_2;


        output_wide_pixel_3.data[0] = INVERSE_CRF_LUT_BLUE0[input_data.data[32]];
        output_wide_pixel_3.data[1] = INVERSE_CRF_LUT_GREEN0[input_data.data[33]];
        output_wide_pixel_3.data[2] = INVERSE_CRF_LUT_RED0[input_data.data[34]];
        output_wide_pixel_3.data[3] = -1;

        output_wide_pixel_3.data[4] = INVERSE_CRF_LUT_BLUE1[input_data.data[36]];
        output_wide_pixel_3.data[5] = INVERSE_CRF_LUT_GREEN1[input_data.data[37]];
        output_wide_pixel_3.data[6] = INVERSE_CRF_LUT_RED1[input_data.data[38]];
        output_wide_pixel_3.data[7] = -1;

        output_wide_pixel_3.data[8] = INVERSE_CRF_LUT_BLUE2[input_data.data[40]];
        output_wide_pixel_3.data[9] = INVERSE_CRF_LUT_GREEN2[input_data.data[41]];
        output_wide_pixel_3.data[10] = INVERSE_CRF_LUT_RED2[input_data.data[42]];
        output_wide_pixel_3.data[11] = -1;

        output_wide_pixel_3.data[12] = INVERSE_CRF_LUT_BLUE3[input_data.data[44]];
        output_wide_pixel_3.data[13] = INVERSE_CRF_LUT_GREEN3[input_data.data[45]];
        output_wide_pixel_3.data[14] = INVERSE_CRF_LUT_RED3[input_data.data[46]];
        output_wide_pixel_3.data[15] = -1;
        memory_bus[output3_pixel_offset] = *(WIDE_DATA_FLAT_T*)&output_wide_pixel_3;


        output_wide_pixel_4.data[0] = INVERSE_CRF_LUT_BLUE0[input_data.data[48]];
        output_wide_pixel_4.data[1] = INVERSE_CRF_LUT_GREEN0[input_data.data[49]];
        output_wide_pixel_4.data[2] = INVERSE_CRF_LUT_RED0[input_data.data[50]];
        output_wide_pixel_4.data[3] = -1;

        output_wide_pixel_4.data[4] = INVERSE_CRF_LUT_BLUE1[input_data.data[52]];
        output_wide_pixel_4.data[5] = INVERSE_CRF_LUT_GREEN1[input_data.data[53]];
        output_wide_pixel_4.data[6] = INVERSE_CRF_LUT_RED1[input_data.data[54]];
        output_wide_pixel_4.data[7] = -1;

        output_wide_pixel_4.data[8] = INVERSE_CRF_LUT_BLUE2[input_data.data[56]];
        output_wide_pixel_4.data[9] = INVERSE_CRF_LUT_GREEN2[input_data.data[57]];
        output_wide_pixel_4.data[10] = INVERSE_CRF_LUT_RED2[input_data.data[58]];
        output_wide_pixel_4.data[11] = -1;

        output_wide_pixel_4.data[12] = INVERSE_CRF_LUT_BLUE3[input_data.data[60]];
        output_wide_pixel_4.data[13] = INVERSE_CRF_LUT_GREEN3[input_data.data[61]];
        output_wide_pixel_4.data[14] = INVERSE_CRF_LUT_RED3[input_data.data[62]];
        output_wide_pixel_4.data[15] = -1;
        memory_bus[output4_pixel_offset] = *(WIDE_DATA_FLAT_T*)&output_wide_pixel_4;
	  //}
	}
	completed_packet_out.write(*(JOB_COMPLETION_PACKET_FLAT*)&completed_ccrf);
}


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

	/*
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

	*/

	WIDE_DATA_FLAT_T *memory_bus,

    hls::stream<JOB_SUBTASK_AXI> &job_info_in_queue,
	hls::stream<bool> &job_done_stream
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

    #pragma HLS INTERFACE bram port=CCRF_LUT_BLUE4
    #pragma HLS INTERFACE bram port=CCRF_LUT_GREEN4
    #pragma HLS INTERFACE bram port=CCRF_LUT_RED4
    #pragma HLS INTERFACE bram port=CCRF_LUT_BLUE5
    #pragma HLS INTERFACE bram port=CCRF_LUT_GREEN5
    #pragma HLS INTERFACE bram port=CCRF_LUT_RED5
    #pragma HLS INTERFACE bram port=CCRF_LUT_BLUE6
    #pragma HLS INTERFACE bram port=CCRF_LUT_GREEN6
    #pragma HLS INTERFACE bram port=CCRF_LUT_RED6
    #pragma HLS INTERFACE bram port=CCRF_LUT_BLUE7
    #pragma HLS INTERFACE bram port=CCRF_LUT_GREEN7
    #pragma HLS INTERFACE bram port=CCRF_LUT_RED7

    #pragma HLS INTERFACE bram port=CCRF_LUT_BLUE8
    #pragma HLS INTERFACE bram port=CCRF_LUT_GREEN8
    #pragma HLS INTERFACE bram port=CCRF_LUT_RED8
    #pragma HLS INTERFACE bram port=CCRF_LUT_BLUE9
    #pragma HLS INTERFACE bram port=CCRF_LUT_GREEN9
    #pragma HLS INTERFACE bram port=CCRF_LUT_RED9
    #pragma HLS INTERFACE bram port=CCRF_LUT_BLUE10
    #pragma HLS INTERFACE bram port=CCRF_LUT_GREEN10
    #pragma HLS INTERFACE bram port=CCRF_LUT_RED10
    #pragma HLS INTERFACE bram port=CCRF_LUT_BLUE11
    #pragma HLS INTERFACE bram port=CCRF_LUT_GREEN11
    #pragma HLS INTERFACE bram port=CCRF_LUT_RED11

    #pragma HLS INTERFACE bram port=CCRF_LUT_BLUE12
    #pragma HLS INTERFACE bram port=CCRF_LUT_GREEN12
    #pragma HLS INTERFACE bram port=CCRF_LUT_RED12
    #pragma HLS INTERFACE bram port=CCRF_LUT_BLUE13
    #pragma HLS INTERFACE bram port=CCRF_LUT_GREEN13
    #pragma HLS INTERFACE bram port=CCRF_LUT_RED13
    #pragma HLS INTERFACE bram port=CCRF_LUT_BLUE14
    #pragma HLS INTERFACE bram port=CCRF_LUT_GREEN14
    #pragma HLS INTERFACE bram port=CCRF_LUT_RED14
    #pragma HLS INTERFACE bram port=CCRF_LUT_BLUE15
    #pragma HLS INTERFACE bram port=CCRF_LUT_GREEN15
    #pragma HLS INTERFACE bram port=CCRF_LUT_RED15

    //#pragma HLS INTERFACE m_axi port=memory_bus depth=2147483648
	//#pragma HLS RESOURCE core=m_axi variable=memory_bus
    #pragma HLS INTERFACE m_axi port=memory_bus
    #pragma HLS DATA_PACK variable=memory_bus

    #pragma HLS STREAM variable=job_info_in_queue depth=1
    //#pragma HLS DATA_PACK variable=job_info_in_queue
    #pragma HLS INTERFACE axis port=job_info_in_queue
    #pragma HLS STREAM variable=job_done_stream
    #pragma HLS INTERFACE axis port=job_done_stream

    #pragma HLS INTERFACE ap_none port=return

    JOB_SUBTASK_AXI job_info_in_tmp_axi = job_info_in_queue.read();
    ap_int<sizeof(JOB_SUBTASK)*8> job_info_in_raw_data = job_info_in_tmp_axi.data;
	JOB_SUBTASK job_info_in = *(JOB_SUBTASK*)&(job_info_in_raw_data);
    #pragma HLS DATA_PACK variable=job_info_in

	const int BURST_LENGTH=64;
    static WIDE_DATA_FLAT_T BRAM_input0[BURST_LENGTH];
    static WIDE_DATA_FLAT_T BRAM_input1[BURST_LENGTH];
    static WIDE_DATA_FLAT_T BRAM_output[BURST_LENGTH];
    #pragma HLS DATA_PACK variable=BRAM_input0 struct_level
    #pragma HLS DATA_PACK variable=BRAM_input1 struct_level
    #pragma HLS DATA_PACK variable=BRAM_output struct_level

    const int pixels_per_memory_read = 16;

    const int pixels_per_memory_burst = pixels_per_memory_read*BURST_LENGTH;


	const uintptr_t in1_wide_data_offset = job_info_in.input1 / (pixels_per_memory_read * sizeof(PIXEL4_T));
	const uintptr_t in2_wide_data_offset = job_info_in.input2 / (pixels_per_memory_read * sizeof(PIXEL4_T));
	const uintptr_t out_wide_data_offset = job_info_in.output / (pixels_per_memory_read * sizeof(PIXEL4_T));
	const uintptr_t iteration_count = job_info_in.image_size/pixels_per_memory_read;

	const uintptr_t memburst_count = iteration_count/BURST_LENGTH;


	for (int i = 0; i < memburst_count; i++) {
        #pragma HLS PIPELINE
    	const uintptr_t input1_pixel_offset = in1_wide_data_offset+i*BURST_LENGTH;
	    const uintptr_t input2_pixel_offset = in2_wide_data_offset+i*BURST_LENGTH;
	    const uintptr_t output_pixel_offset = out_wide_data_offset+i*BURST_LENGTH;
	    memcpy(BRAM_input0, memory_bus+input1_pixel_offset, BURST_LENGTH*sizeof(WIDE_DATA_T));
	    memcpy(BRAM_input1, memory_bus+input2_pixel_offset, BURST_LENGTH*sizeof(WIDE_DATA_T));

		for (int j = 0; j < BURST_LENGTH; j++) {
            #pragma HLS pipeline
			WIDE_DATA_T output_wide_pixel;

			WIDE_DATA_FLAT_T input1_wide_pixel_flat = BRAM_input0[j];
            WIDE_DATA_FLAT_T input2_wide_pixel_flat = BRAM_input1[j];
			WIDE_DATA_T input1_wide_pixel = *(WIDE_DATA_T*)&input1_wide_pixel_flat;
			WIDE_DATA_T input2_wide_pixel = *(WIDE_DATA_T*)&input2_wide_pixel_flat;

            // INPUT IMAGE 1
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

			BYTE_T B1_4 = input1_wide_pixel.data[16];
			BYTE_T G1_4 = input1_wide_pixel.data[17];
			BYTE_T R1_4 = input1_wide_pixel.data[18];
        	BYTE_T B1_5 = input1_wide_pixel.data[20];
        	BYTE_T G1_5 = input1_wide_pixel.data[21];
        	BYTE_T R1_5 = input1_wide_pixel.data[22];
       		BYTE_T B1_6 = input1_wide_pixel.data[24];
       		BYTE_T G1_6 = input1_wide_pixel.data[25];
        	BYTE_T R1_6 = input1_wide_pixel.data[26];
        	BYTE_T B1_7 = input1_wide_pixel.data[28];
        	BYTE_T G1_7 = input1_wide_pixel.data[29];
        	BYTE_T R1_7 = input1_wide_pixel.data[30];

			    BYTE_T B1_8 = input1_wide_pixel.data[32];
			    BYTE_T G1_8 = input1_wide_pixel.data[33];
			    BYTE_T R1_8 = input1_wide_pixel.data[34];
       	 		BYTE_T B1_9 = input1_wide_pixel.data[36];
       	 		BYTE_T G1_9 = input1_wide_pixel.data[37];
       	 		BYTE_T R1_9 = input1_wide_pixel.data[38];
       	 		BYTE_T B1_10 = input1_wide_pixel.data[40];
        		BYTE_T G1_10 = input1_wide_pixel.data[41];
        		BYTE_T R1_10 = input1_wide_pixel.data[42];
        		BYTE_T B1_11 = input1_wide_pixel.data[44];
        		BYTE_T G1_11 = input1_wide_pixel.data[45];
        		BYTE_T R1_11 = input1_wide_pixel.data[46];

			    BYTE_T B1_12 = input1_wide_pixel.data[48];
			    BYTE_T G1_12 = input1_wide_pixel.data[49];
			    BYTE_T R1_12 = input1_wide_pixel.data[50];
        		BYTE_T B1_13 = input1_wide_pixel.data[52];
        		BYTE_T G1_13 = input1_wide_pixel.data[53];
        		BYTE_T R1_13 = input1_wide_pixel.data[54];
        		BYTE_T B1_14 = input1_wide_pixel.data[56];
        		BYTE_T G1_14 = input1_wide_pixel.data[57];
        		BYTE_T R1_14 = input1_wide_pixel.data[58];
        		BYTE_T B1_15 = input1_wide_pixel.data[60];
        		BYTE_T G1_15 = input1_wide_pixel.data[61];
        		BYTE_T R1_15 = input1_wide_pixel.data[62];

        		// INPUT IMAGE 2
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

			    BYTE_T B2_4 = input2_wide_pixel.data[16];
			    BYTE_T G2_4 = input2_wide_pixel.data[17];
			    BYTE_T R2_4 = input2_wide_pixel.data[18];
        		BYTE_T B2_5 = input2_wide_pixel.data[20];
        		BYTE_T G2_5 = input2_wide_pixel.data[21];
        		BYTE_T R2_5 = input2_wide_pixel.data[22];
        		BYTE_T B2_6 = input2_wide_pixel.data[24];
        		BYTE_T G2_6 = input2_wide_pixel.data[25];
        		BYTE_T R2_6 = input2_wide_pixel.data[26];
        		BYTE_T B2_7 = input2_wide_pixel.data[28];
        		BYTE_T G2_7 = input2_wide_pixel.data[29];
        		BYTE_T R2_7 = input2_wide_pixel.data[30];

			    BYTE_T B2_8 = input2_wide_pixel.data[32];
			    BYTE_T G2_8 = input2_wide_pixel.data[33];
			    BYTE_T R2_8 = input2_wide_pixel.data[34];
        		BYTE_T B2_9 = input2_wide_pixel.data[36];
        		BYTE_T G2_9 = input2_wide_pixel.data[37];
        		BYTE_T R2_9 = input2_wide_pixel.data[38];
        		BYTE_T B2_10 = input2_wide_pixel.data[40];
        		BYTE_T G2_10 = input2_wide_pixel.data[41];
        		BYTE_T R2_10 = input2_wide_pixel.data[42];
        		BYTE_T B2_11 = input2_wide_pixel.data[44];
        		BYTE_T G2_11 = input2_wide_pixel.data[45];
        		BYTE_T R2_11 = input2_wide_pixel.data[46];

			    BYTE_T B2_12 = input2_wide_pixel.data[48];
			    BYTE_T G2_12 = input2_wide_pixel.data[49];
			    BYTE_T R2_12 = input2_wide_pixel.data[50];
        		BYTE_T B2_13 = input2_wide_pixel.data[52];
        		BYTE_T G2_13 = input2_wide_pixel.data[53];
        		BYTE_T R2_13 = input2_wide_pixel.data[54];
        		BYTE_T B2_14 = input2_wide_pixel.data[56];
        		BYTE_T G2_14 = input2_wide_pixel.data[57];
        		BYTE_T R2_14 = input2_wide_pixel.data[58];
        		BYTE_T B2_15 = input2_wide_pixel.data[60];
        		BYTE_T G2_15 = input2_wide_pixel.data[61];
        		BYTE_T R2_15 = input2_wide_pixel.data[62];

			    uint16_t LUT_entry_b0 = (uint16_t)B1_0 * 256 + (uint16_t)B2_0;
        		uint16_t LUT_entry_g0 = (uint16_t)G1_0 * 256 + (uint16_t)G2_0;
        		uint16_t LUT_entry_r0 = (uint16_t)R1_0 * 256 + (uint16_t)R2_0;
        		uint16_t LUT_entry_b1 = (uint16_t)B1_1 * 256 + (uint16_t)B2_1;
        		uint16_t LUT_entry_g1 = (uint16_t)G1_1 * 256 + (uint16_t)G2_1;
        		uint16_t LUT_entry_r1 = (uint16_t)R1_1 * 256 + (uint16_t)R2_1;
        		uint16_t LUT_entry_b2 = (uint16_t)B1_2 * 256 + (uint16_t)B2_2;
        		uint16_t LUT_entry_g2 = (uint16_t)G1_2 * 256 + (uint16_t)G2_2;
        		uint16_t LUT_entry_r2 = (uint16_t)R1_2 * 256 + (uint16_t)R2_2;
        		uint16_t LUT_entry_b3 = (uint16_t)B1_3 * 256 + (uint16_t)B2_3;
        		uint16_t LUT_entry_g3 = (uint16_t)G1_3 * 256 + (uint16_t)G2_3;
        		uint16_t LUT_entry_r3 = (uint16_t)R1_3 * 256 + (uint16_t)R2_3;
	
			    uint16_t LUT_entry_b4 = (uint16_t)B1_4 * 256 + (uint16_t)B2_4;
        		uint16_t LUT_entry_g4 = (uint16_t)G1_4 * 256 + (uint16_t)G2_4;
        		uint16_t LUT_entry_r4 = (uint16_t)R1_4 * 256 + (uint16_t)R2_4;
        		uint16_t LUT_entry_b5 = (uint16_t)B1_5 * 256 + (uint16_t)B2_5;
       	 		uint16_t LUT_entry_g5 = (uint16_t)G1_5 * 256 + (uint16_t)G2_5;
        		uint16_t LUT_entry_r5 = (uint16_t)R1_5 * 256 + (uint16_t)R2_5;
       	 		uint16_t LUT_entry_b6 = (uint16_t)B1_6 * 256 + (uint16_t)B2_6;
        		uint16_t LUT_entry_g6 = (uint16_t)G1_6 * 256 + (uint16_t)G2_6;
        		uint16_t LUT_entry_r6 = (uint16_t)R1_6 * 256 + (uint16_t)R2_6;
        		uint16_t LUT_entry_b7 = (uint16_t)B1_7 * 256 + (uint16_t)B2_7;
        		uint16_t LUT_entry_g7 = (uint16_t)G1_7 * 256 + (uint16_t)G2_7;
        		uint16_t LUT_entry_r7 = (uint16_t)R1_7 * 256 + (uint16_t)R2_7;
	
			    uint16_t LUT_entry_b8 = (uint16_t)B1_8 * 256 + (uint16_t)B2_8;
        		uint16_t LUT_entry_g8 = (uint16_t)G1_8 * 256 + (uint16_t)G2_8;
        		uint16_t LUT_entry_r8 = (uint16_t)R1_8 * 256 + (uint16_t)R2_8;
        		uint16_t LUT_entry_b9 = (uint16_t)B1_9 * 256 + (uint16_t)B2_9;
        		uint16_t LUT_entry_g9 = (uint16_t)G1_9 * 256 + (uint16_t)G2_9;
        		uint16_t LUT_entry_r9 = (uint16_t)R1_9 * 256 + (uint16_t)R2_9;
        		uint16_t LUT_entry_b10 = (uint16_t)B1_10 * 256 + (uint16_t)B2_10;
        		uint16_t LUT_entry_g10 = (uint16_t)G1_10 * 256 + (uint16_t)G2_10;
        		uint16_t LUT_entry_r10 = (uint16_t)R1_10 * 256 + (uint16_t)R2_10;
        		uint16_t LUT_entry_b11 = (uint16_t)B1_11 * 256 + (uint16_t)B2_11;
        		uint16_t LUT_entry_g11 = (uint16_t)G1_11 * 256 + (uint16_t)G2_11;
        		uint16_t LUT_entry_r11 = (uint16_t)R1_11 * 256 + (uint16_t)R2_11;

        		uint16_t LUT_entry_b12 = (uint16_t)B1_12 * 256 + (uint16_t)B2_12;
        		uint16_t LUT_entry_g12 = (uint16_t)G1_12 * 256 + (uint16_t)G2_12;
        		uint16_t LUT_entry_r12 = (uint16_t)R1_12 * 256 + (uint16_t)R2_12;
        		uint16_t LUT_entry_b13 = (uint16_t)B1_13 * 256 + (uint16_t)B2_13;
        		uint16_t LUT_entry_g13 = (uint16_t)G1_13 * 256 + (uint16_t)G2_13;
        		uint16_t LUT_entry_r13 = (uint16_t)R1_13 * 256 + (uint16_t)R2_13;
       	 		uint16_t LUT_entry_b14 = (uint16_t)B1_14 * 256 + (uint16_t)B2_14;
        		uint16_t LUT_entry_g14 = (uint16_t)G1_14 * 256 + (uint16_t)G2_14;
        		uint16_t LUT_entry_r14 = (uint16_t)R1_14 * 256 + (uint16_t)R2_14;
        		uint16_t LUT_entry_b15 = (uint16_t)B1_15 * 256 + (uint16_t)B2_15;
        		uint16_t LUT_entry_g15 = (uint16_t)G1_15 * 256 + (uint16_t)G2_15;
        		uint16_t LUT_entry_r15 = (uint16_t)R1_15 * 256 + (uint16_t)R2_15;


	        	output_wide_pixel.data[0] = CCRF_LUT_BLUE0[LUT_entry_b0];
	        	output_wide_pixel.data[1] = CCRF_LUT_GREEN0[LUT_entry_g0];
	        	output_wide_pixel.data[2] = CCRF_LUT_RED0[LUT_entry_r0];
	        	output_wide_pixel.data[3] = 255;
	        	output_wide_pixel.data[4] = CCRF_LUT_BLUE1[LUT_entry_b1];
	        	output_wide_pixel.data[5] = CCRF_LUT_GREEN1[LUT_entry_g1];
	        	output_wide_pixel.data[6] = CCRF_LUT_RED1[LUT_entry_r1];
	        	output_wide_pixel.data[7] = 255;
	        	output_wide_pixel.data[8] = CCRF_LUT_BLUE2[LUT_entry_b2];
	        	output_wide_pixel.data[9] = CCRF_LUT_GREEN2[LUT_entry_g2];
	        	output_wide_pixel.data[10] = CCRF_LUT_RED2[LUT_entry_r2];
	        	output_wide_pixel.data[11] = 255;
	        	output_wide_pixel.data[12] = CCRF_LUT_BLUE3[LUT_entry_b3];
	        	output_wide_pixel.data[13] = CCRF_LUT_GREEN3[LUT_entry_g3];
	        	output_wide_pixel.data[14] = CCRF_LUT_RED3[LUT_entry_r3];
	        	output_wide_pixel.data[15] = 255;
	
        		output_wide_pixel.data[16] = CCRF_LUT_BLUE4[LUT_entry_b4];
        		output_wide_pixel.data[17] = CCRF_LUT_GREEN4[LUT_entry_g4];
        		output_wide_pixel.data[18] = CCRF_LUT_RED4[LUT_entry_r4];
        		output_wide_pixel.data[19] = 255;
        		output_wide_pixel.data[20] = CCRF_LUT_BLUE5[LUT_entry_b5];
        		output_wide_pixel.data[21] = CCRF_LUT_GREEN5[LUT_entry_g5];
        		output_wide_pixel.data[22] = CCRF_LUT_RED5[LUT_entry_r5];
        		output_wide_pixel.data[23] = 255;
        		output_wide_pixel.data[24] = CCRF_LUT_BLUE6[LUT_entry_b6];
        		output_wide_pixel.data[25] = CCRF_LUT_GREEN6[LUT_entry_g6];
        		output_wide_pixel.data[26] = CCRF_LUT_RED6[LUT_entry_r6];
        		output_wide_pixel.data[27] = 255;
        		output_wide_pixel.data[28] = CCRF_LUT_BLUE7[LUT_entry_b7];
        		output_wide_pixel.data[29] = CCRF_LUT_GREEN7[LUT_entry_g7];
        		output_wide_pixel.data[30] = CCRF_LUT_RED7[LUT_entry_r7];
        		output_wide_pixel.data[31] = 255;
		
        		output_wide_pixel.data[32] = CCRF_LUT_BLUE8[LUT_entry_b8];
        		output_wide_pixel.data[33] = CCRF_LUT_GREEN8[LUT_entry_g8];
        		output_wide_pixel.data[34] = CCRF_LUT_RED8[LUT_entry_r8];
        		output_wide_pixel.data[35] = 255;
        		output_wide_pixel.data[36] = CCRF_LUT_BLUE9[LUT_entry_b9];
        		output_wide_pixel.data[37] = CCRF_LUT_GREEN9[LUT_entry_g9];
        		output_wide_pixel.data[38] = CCRF_LUT_RED9[LUT_entry_r9];
        		output_wide_pixel.data[39] = 255;
        		output_wide_pixel.data[40] = CCRF_LUT_BLUE10[LUT_entry_b10];
        		output_wide_pixel.data[41] = CCRF_LUT_GREEN10[LUT_entry_g10];
        		output_wide_pixel.data[42] = CCRF_LUT_RED10[LUT_entry_r10];
        		output_wide_pixel.data[43] = 255;
        		output_wide_pixel.data[44] = CCRF_LUT_BLUE11[LUT_entry_b11];
        		output_wide_pixel.data[45] = CCRF_LUT_GREEN11[LUT_entry_g11];
        		output_wide_pixel.data[46] = CCRF_LUT_RED11[LUT_entry_r11];
        		output_wide_pixel.data[47] = 255;

        		output_wide_pixel.data[48] = CCRF_LUT_BLUE12[LUT_entry_b12];
        		output_wide_pixel.data[49] = CCRF_LUT_GREEN12[LUT_entry_g12];
        		output_wide_pixel.data[50] = CCRF_LUT_RED12[LUT_entry_r12];
        		output_wide_pixel.data[51] = 255;
        		output_wide_pixel.data[52] = CCRF_LUT_BLUE13[LUT_entry_b13];
        		output_wide_pixel.data[53] = CCRF_LUT_GREEN13[LUT_entry_g13];
        		output_wide_pixel.data[54] = CCRF_LUT_RED13[LUT_entry_r13];
        		output_wide_pixel.data[55] = 255;
        		output_wide_pixel.data[56] = CCRF_LUT_BLUE14[LUT_entry_b14];
        		output_wide_pixel.data[57] = CCRF_LUT_GREEN14[LUT_entry_g14];
        		output_wide_pixel.data[58] = CCRF_LUT_RED14[LUT_entry_r14];
        		output_wide_pixel.data[59] = 255;
        		output_wide_pixel.data[60] = CCRF_LUT_BLUE15[LUT_entry_b15];
        		output_wide_pixel.data[61] = CCRF_LUT_GREEN15[LUT_entry_g15];
        		output_wide_pixel.data[62] = CCRF_LUT_RED15[LUT_entry_r15];
        		output_wide_pixel.data[63] = 255;

     			//memory_bus[output_pixel_offset] = output_wide_pixel;
			    BRAM_output[j] = *(WIDE_DATA_FLAT_T*)&output_wide_pixel;
    		}
		    memcpy(memory_bus+output_pixel_offset, BRAM_output, BURST_LENGTH*sizeof(WIDE_DATA_T));
    	}

	bool done = true;
    	job_done_stream.write(done);

}


void Run_CCRF(CCRF_UNIT_STATUS_SIGNALS &status_signals,
              hls::stream<JOB_SUBTASK> &input_subtask_queue,
              hls::stream<uintptr_t> &output_subtask_queue,

			  hls::stream<JOB_SUBTASK> &job_info_out_queue,
			  hls::stream<bool> &job_done_in_queue,

              bool &ccrf_got_data,
              bool &ccrf_sent_data,
			  ap_uint<4> &current_ccrf_task_num
              )
{
    #pragma HLS STREAM variable=input_subtask_queue depth=1
    #pragma HLS STREAM variable=output_subtask_queue depth=1
    #pragma HLS DATA_PACK variable=job_info_out_queue struct_level
    #pragma HLS RESOURCE core=axis variable=job_info_out_queue
    #pragma HLS STREAM variable=job_done_in_queue depth=1
    #pragma HLS RESOURCE core=axis variable=job_done_in_queue
    #pragma HLS INTERFACE ap_none port=status_signals
    #pragma HLS DATA_PACK variable=input_subtask_queue
    #pragma HLS DATA_PACK variable=status_signals struct_level
    //#pragma HLS DATA_PACK variable=output_subtask_queue struct_level
    //#pragma HLS INTERFACE axis port=input_subtask_queue
    //#pragma HLS INTERFACE axis port=output_subtask_queue
    #pragma HLS RESOURCE core=axis variable=input_subtask_queue
    #pragma HLS RESOURCE core=axis variable=output_subtask_queue

    #pragma HLS INTERFACE ap_ctrl_none port=return
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

    static ap_uint<4> current_ccrf_task_num_internal = 16;
    static bool job_valid_internal = false;
    static JOB_SUBTASK job_info_internal;
    #pragma HLS DATA_PACK variable=job_info_internal

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
		job_info_out_queue.write(job_info_internal);
		current_ccrf_task_num_internal += 1;
		ctrl_state_next = WAIT_JOB_DONE;
		break;

	case WAIT_JOB_DONE:
		if (!job_done_in_queue.empty()) {
			bool job_done = job_done_in_queue.read();
			if (job_done) {
			    ctrl_state_next = WRITE_OUTPUT;
			} else {
				ctrl_state_next = WAIT_JOB_DONE;
			}
		} else {
			ctrl_state_next = WAIT_JOB_DONE;
		}
		break;

	case WRITE_OUTPUT:
		output_subtask_queue.write(job_info_internal.output);
		ctrl_state_next = START;
        break;
	};

    status_signals = status_signals_internal;
    current_ccrf_task_num = current_ccrf_task_num_internal;
    status_signals = status_signals_internal;

    ctrl_state = ctrl_state_next;
}

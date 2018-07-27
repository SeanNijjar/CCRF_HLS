#include "ccrf.hpp"

#include <ap_utils.h>
#include <ap_int.h>
#include <ap_axi_sdata.h>
//#include <string.h>




void InverseCRF(
		hls::stream<JOB_COMPLETION_PACKET_FLAT> &CCRF_completed_job,
		hls::stream<JOB_COMPLETION_PACKET_FLAT> &completed_packet_out,
		PIXEL4_FLAT_T *memory_bus,
		//WIDE_DATA_FLAT_T *memory_bus,
		INVERSE_CRF_LUT_BUNDLE(0),
		INVERSE_CRF_LUT_BUNDLE(1),
		INVERSE_CRF_LUT_BUNDLE(2),
		INVERSE_CRF_LUT_BUNDLE(3)
) {
    #pragma HLS INTERFACE m_axi port=memory_bus depth=2147483648
	//4294967295
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
    
    const uintptr_t ccrf_image_offset = ccrf_image_address / sizeof(PIXEL4_T);//(pixels_per_memory_read * sizeof(PIXEL4_T));
    const uintptr_t iteration_count = image_pixel_count;///pixels_per_memory_read;
    
    // Align the iteration to the next lowest 4096B alignment
    const int wide_reads_per_page = alignment / (pixels_per_memory_read*sizeof(PIXEL4_T));
    const int starting_i = iteration_count & ~(1-wide_reads_per_page);
    const int MAX_AXI_BURST_SIZE = 4096;
    //for (int i = starting_i; i >= 0; i -= 64) { // BURSTED LOOP


    for (int i = iteration_count; 0 <= i; i--) {
        //#pragma HLS PIPELINE II=4
        const uintptr_t input_pixel_offset = ccrf_image_offset + i;
        const uintptr_t output1_pixel_offset = ccrf_image_offset + (4 * i);
        const uintptr_t output2_pixel_offset = ccrf_image_offset + (4 * i) + 1;
        const uintptr_t output3_pixel_offset = ccrf_image_offset + (4 * i) + 2;
        const uintptr_t output4_pixel_offset = ccrf_image_offset + (4 * i) + 3;
        /*
        WIDE_DATA_FLAT_T input_data_flattened;
        WIDE_PIXEL_WIDE_DATA_T output_wide_pixel1;
        WIDE_PIXEL_WIDE_DATA_T output_wide_pixel2;
        WIDE_PIXEL_WIDE_DATA_T output_wide_pixel3;
        WIDE_PIXEL_WIDE_DATA_T output_wide_pixel4;
        */
        PIXEL4_FLAT_T input_data_flattened;
        uint32_t output_wide_pixel1;
        uint32_t output_wide_pixel2;
        uint32_t output_wide_pixel3;
        uint32_t output_wide_pixel4;
        //memcpy((WIDE_DATA_FLAT_T*)&input_data_flattened, &memory_bus[input_pixel_offset], sizeof(WIDE_DATA_FLAT_T));
        input_data_flattened = memory_bus[input_pixel_offset];
        //WIDE_DATA_T input_data = *(WIDE_DATA_T*)&input_data_flattened;
        //WIDE_DATA_T input_data;
        PIXEL4_T input_data;
        memcpy(&input_data, &input_data_flattened, sizeof(PIXEL4_T));//sizeof(WIDE_DATA_T));
        
        /* BURSTED CODE COMMENTED OUT */
        //memcpy(MEM_READ_BUFFER, memory_bus[input_pixel_offset], MAX_AXI_BURST_SIZE);
        //for (int j = 0; j < 8; j++) {
        //#pragma HLS pipeline II=4
        //WIDE_DATA_T input_data = MEM_READ_BUFFER[j];//memory_bus[input_pixel_offset];
        
        BYTE_T B0_0IDX = input_data[0];
        BYTE_T G0_0IDX = input_data[1];
        BYTE_T R0_0IDX = input_data[2];
        
        output_wide_pixel1 = INVERSE_CRF_LUT_BLUE0[B0_0IDX];
        output_wide_pixel2 = INVERSE_CRF_LUT_GREEN0[G0_0IDX];
        output_wide_pixel3 = INVERSE_CRF_LUT_RED0[R0_0IDX];
        output_wide_pixel4 = 1;
        /*
        output_wide_pixel1.data[4] = INVERSE_CRF_LUT_BLUE1[B1_0IDX];
        output_wide_pixel1.data[5] = INVERSE_CRF_LUT_GREEN1[G1_0IDX];
        output_wide_pixel1.data[6] = INVERSE_CRF_LUT_RED1[R1_0IDX];
        output_wide_pixel1.data[7] = 1;
        output_wide_pixel1.data[8] = INVERSE_CRF_LUT_BLUE2[B2_0IDX];
        output_wide_pixel1.data[9] = INVERSE_CRF_LUT_GREEN2[G2_0IDX];
        output_wide_pixel1.data[10] = INVERSE_CRF_LUT_RED2[R2_0IDX];
        output_wide_pixel1.data[11] = 1;
        output_wide_pixel1.data[12] = INVERSE_CRF_LUT_BLUE3[B3_0IDX];
        output_wide_pixel1.data[13] = INVERSE_CRF_LUT_GREEN3[G3_0IDX];
        output_wide_pixel1.data[14] = INVERSE_CRF_LUT_RED3[R3_0IDX];
        output_wide_pixel1.data[15] = 1;
        */
        //memory_bus[output1_pixel_offset] = *(WIDE_DATA_FLAT_T*)&output_wide_pixel1;
        memory_bus[output1_pixel_offset] = *(uint32_t*)&output_wide_pixel1;
        memory_bus[output2_pixel_offset] = *(uint32_t*)&output_wide_pixel2;
        memory_bus[output3_pixel_offset] = *(uint32_t*)&output_wide_pixel3;
        memory_bus[output4_pixel_offset] = *(uint32_t*)&output_wide_pixel4;

        /*
        BYTE_T B0_1IDX = input_data.data[16];
        BYTE_T G0_1IDX = input_data.data[17];
        BYTE_T R0_1IDX = input_data.data[18];
        
        BYTE_T B1_1IDX = input_data.data[20];
        BYTE_T G1_1IDX = input_data.data[21];
        BYTE_T R1_1IDX = input_data.data[22];
        
        BYTE_T B2_1IDX = input_data.data[24];
        BYTE_T G2_1IDX = input_data.data[25];
        BYTE_T R2_1IDX = input_data.data[26];
        
        BYTE_T B3_1IDX = input_data.data[28];
        BYTE_T G3_1IDX = input_data.data[29];
        BYTE_T R3_1IDX = input_data.data[30];

        output_wide_pixel2.data[0] = INVERSE_CRF_LUT_BLUE0[B1_0IDX];//B0_1IDX];
        output_wide_pixel2.data[1] = INVERSE_CRF_LUT_GREEN0[G1_0IDX];//G0_1IDX];
        output_wide_pixel2.data[2] = INVERSE_CRF_LUT_RED0[R1_0IDX];//R0_1IDX];
        output_wide_pixel2.data[3] = 1;
        output_wide_pixel2.data[4] = INVERSE_CRF_LUT_BLUE1[B1_1IDX];
        output_wide_pixel2.data[5] = INVERSE_CRF_LUT_GREEN1[G1_1IDX];
        output_wide_pixel2.data[6] = INVERSE_CRF_LUT_RED1[R1_1IDX];
        output_wide_pixel2.data[7] = 1;
        output_wide_pixel2.data[8] = INVERSE_CRF_LUT_BLUE2[B2_1IDX];
        output_wide_pixel2.data[9] = INVERSE_CRF_LUT_GREEN2[G2_1IDX];
        output_wide_pixel2.data[10] = INVERSE_CRF_LUT_RED2[R2_1IDX];
        output_wide_pixel2.data[11] = 1;
        output_wide_pixel2.data[12] = INVERSE_CRF_LUT_BLUE3[B3_1IDX];
        output_wide_pixel2.data[13] = INVERSE_CRF_LUT_GREEN3[G3_1IDX];
        output_wide_pixel2.data[14] = INVERSE_CRF_LUT_RED3[R3_1IDX];
        output_wide_pixel2.data[15] = 1;

        memory_bus[output2_pixel_offset] = *(WIDE_DATA_FLAT_T*)&output_wide_pixel2;
        
        BYTE_T B0_2IDX = input_data.data[32];
        BYTE_T G0_2IDX = input_data.data[33];
        BYTE_T R0_2IDX = input_data.data[34];
        
        BYTE_T B1_2IDX = input_data.data[36];
        BYTE_T G1_2IDX = input_data.data[37];
        BYTE_T R1_2IDX = input_data.data[38];
        
        BYTE_T B2_2IDX = input_data.data[40];
        BYTE_T G2_2IDX = input_data.data[41];
        BYTE_T R2_2IDX = input_data.data[42];
        
        BYTE_T B3_2IDX = input_data.data[44];
        BYTE_T G3_2IDX = input_data.data[45];
        BYTE_T R3_2IDX = input_data.data[46];
        
        output_wide_pixel3.data[0] = INVERSE_CRF_LUT_BLUE0[B0_2IDX];
        output_wide_pixel3.data[1] = INVERSE_CRF_LUT_GREEN0[G0_2IDX];
        output_wide_pixel3.data[2] = INVERSE_CRF_LUT_RED0[R0_2IDX];
        output_wide_pixel3.data[3] = 1;
        output_wide_pixel3.data[4] = INVERSE_CRF_LUT_BLUE1[B1_2IDX];
        output_wide_pixel3.data[5] = INVERSE_CRF_LUT_GREEN1[G1_2IDX];
        output_wide_pixel3.data[6] = INVERSE_CRF_LUT_RED1[R1_2IDX];
        output_wide_pixel3.data[7] = 1;
        output_wide_pixel3.data[8] = INVERSE_CRF_LUT_BLUE2[B2_2IDX];
        output_wide_pixel3.data[9] = INVERSE_CRF_LUT_GREEN2[G2_2IDX];
        output_wide_pixel3.data[10] = INVERSE_CRF_LUT_RED2[R2_2IDX];
        output_wide_pixel3.data[11] = 1;
        output_wide_pixel3.data[12] = INVERSE_CRF_LUT_BLUE3[B3_2IDX];
        output_wide_pixel3.data[13] = INVERSE_CRF_LUT_GREEN3[G3_2IDX];
        output_wide_pixel3.data[14] = INVERSE_CRF_LUT_RED3[R3_2IDX];
        output_wide_pixel3.data[15] = 1;
        

        memory_bus[output3_pixel_offset] = *(WIDE_DATA_FLAT_T*)&output_wide_pixel3;

        BYTE_T B0_3IDX = input_data.data[48];
        BYTE_T G0_3IDX = input_data.data[49];
        BYTE_T R0_3IDX = input_data.data[50];
        
        BYTE_T B1_3IDX = input_data.data[52];
        BYTE_T G1_3IDX = input_data.data[53];
        BYTE_T R1_3IDX = input_data.data[54];
        
        BYTE_T B2_3IDX = input_data.data[56];
        BYTE_T G2_3IDX = input_data.data[57];
        BYTE_T R2_3IDX = input_data.data[58];
        
        BYTE_T B3_3IDX = input_data.data[60];
        BYTE_T G3_3IDX = input_data.data[61];
        BYTE_T R3_3IDX = input_data.data[62];
        
        output_wide_pixel4.data[0] = INVERSE_CRF_LUT_BLUE0[B0_3IDX];
        output_wide_pixel4.data[1] = INVERSE_CRF_LUT_GREEN0[G0_3IDX];
        output_wide_pixel4.data[2] = INVERSE_CRF_LUT_RED0[R0_3IDX];
        output_wide_pixel4.data[3] = 1;
        output_wide_pixel4.data[4] = INVERSE_CRF_LUT_BLUE1[B1_3IDX];
        output_wide_pixel4.data[5] = INVERSE_CRF_LUT_GREEN1[G1_3IDX];
        output_wide_pixel4.data[6] = INVERSE_CRF_LUT_RED1[R1_3IDX];
        output_wide_pixel4.data[7] = 1;
        output_wide_pixel4.data[8] = INVERSE_CRF_LUT_BLUE2[B2_3IDX];
        output_wide_pixel4.data[9] = INVERSE_CRF_LUT_GREEN2[G2_3IDX];
        output_wide_pixel4.data[10] = INVERSE_CRF_LUT_RED2[R2_3IDX];
        output_wide_pixel4.data[11] = 1;
        output_wide_pixel4.data[12] = INVERSE_CRF_LUT_BLUE3[B3_3IDX];
        output_wide_pixel4.data[13] = INVERSE_CRF_LUT_GREEN3[G3_3IDX];
        output_wide_pixel4.data[14] = INVERSE_CRF_LUT_RED3[R3_3IDX];
        output_wide_pixel4.data[15] = 1;
        
        memory_bus[output4_pixel_offset] = *(WIDE_DATA_FLAT_T*)&output_wide_pixel4;
        */
        //}
    }
    completed_packet_out.write(*(JOB_COMPLETION_PACKET_FLAT*)&completed_ccrf);
}



/*
void CCRF_Compute(
		PLDDR_BUS_FLAT * plmem,

	    hls::stream<JOB_SUBTASK_AXI> &job_info_in_queue,
		//hls::stream<JOB_SUBTASK> &job_info_in_queue,
		hls::stream<bool> &job_done_stream,


		//const int input1_offset,
		//const int input2_offset,
		//const int output_offset,
		//const int image_size,


		const CCRF_LUT CCRF_LUT_BLUE[16],
		const CCRF_LUT CCRF_LUT_GREEN[16],
		const CCRF_LUT CCRF_LUT_RED[16])
{
	// Global memory interface
	#pragma HLS INTERFACE m_axi port=plmem depth=2147483648 max_read_burst_length=64 max_write_burst_length=64 bundle="plmem"
	#pragma HLS INTERFACE ap_memory port=CCRF_LUT_BLUE
	#pragma HLS INTERFACE ap_memory port=CCRF_LUT_GREEN
	#pragma HLS INTERFACE ap_memory port=CCRF_LUT_RED
	#pragma HLS array_partition variable=CCRF_LUT_BLUE complete
	#pragma HLS array_partition variable=CCRF_LUT_GREEN complete
	#pragma HLS array_partition variable=CCRF_LUT_RED complete

	// Bind all control ports to a single bundle
	//pragma HLS INTERFACE s_axilite port=input1_offset bundle=CTRL_BUS
	//#pragma HLS INTERFACE s_axilite port=input2_offset bundle=CTRL_BUS
	//#pragma HLS INTERFACE s_axilite port=output_offset bundle=CTRL_BUS
	//#pragma HLS INTERFACE s_axilite port=image_size bundle=CTRL_BUS
	//#pragma HLS INTERFACE s_axilite port=return bundle=CTRL_BUS

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
	const uintptr_t input1_offset = job_info_in.input1;
	const uintptr_t input2_offset = job_info_in.input2;
	const uintptr_t output_offset = job_info_in.output;
	const int image_size = job_info_in.image_size;

	const int CHANNELS_PER_PIXEL = 4;

	BUS_addr input1_offset_64 = input1_offset;
	BUS_addr input2_offset_64 = input2_offset;
	BUS_addr output_offset_64 = output_offset;

	PLDDR_BUS_FLAT input_buffer1[BURST_LENGTH*CHANNELS_PER_PIXEL];
	PLDDR_BUS_FLAT input_buffer2[BURST_LENGTH*CHANNELS_PER_PIXEL];
	PLDDR_BUS_FLAT output_buffer[BURST_LENGTH*CHANNELS_PER_PIXEL];
	
	PIXEL input1[64*CHANNELS_PER_PIXEL];
	PIXEL input2[64*CHANNELS_PER_PIXEL];
	PIXEL output[64*CHANNELS_PER_PIXEL];
	#pragma HLS array_partition variable=input1 cyclic factor=64 dim=0
	#pragma HLS array_partition variable=input2 cyclic factor=64 dim=0
	#pragma HLS array_partition variable=output cyclic factor=64 dim=0

	int byte_per_burst = 64 * BURST_LENGTH;
	int mem_burst_num = (image_size -1) / (CHANNELS_PER_PIXEL*byte_per_burst) + 1;
//	Every Burst : 64*64 BYTES
//	3 Burst Per time
//	Image Size : 1920*1080*4 BYTES
	for (int i = 0; i < mem_burst_num; i++) {
	#pragma HLS loop_tripcount min=507 max=507
		for (int copy_in = 0; copy_in < CHANNELS_PER_PIXEL; copy_in++) {
		  #pragma HLS UNROLL
		  memcpy(input_buffer1+copy_in*BURST_LENGTH, plmem+input1_offset_64/64+(CHANNELS_PER_PIXEL*i+copy_in)*BURST_LENGTH, BURST_LENGTH*sizeof(PLDDR_BUS_FLAT));
		  memcpy(input_buffer2+copy_in*BURST_LENGTH, plmem+input2_offset_64/64+(CHANNELS_PER_PIXEL*i+copy_in)*BURST_LENGTH, BURST_LENGTH*sizeof(PLDDR_BUS_FLAT));
		}

		for (int j = 0; j < BURST_LENGTH; j++) {
			for (int k = 0; k < CHANNELS_PER_PIXEL; k ++) {
			#pragma HLS pipeline
				for (int p = 0; p < 64; p++) {
				#pragma HLS unroll
					input1[64*k+p] = input_buffer1[CHANNELS_PER_PIXEL*j+k].range(p*8+7,p*8);
					input2[64*k+p] = input_buffer2[CHANNELS_PER_PIXEL*j+k].range(p*8+7,p*8);
				}
			}
			for (int k = 0; k < CHANNELS_PER_PIXEL; k ++) {
			#pragma HLS pipeline II=1
				for (int p = 0; p < 16; p++) {
				#pragma HLS unroll
					output[k*64+CHANNELS_PER_PIXEL*p] = CCRF_LUT_BLUE[p].ROM[(input1[k*64+CHANNELS_PER_PIXEL*p],input2[k*64+CHANNELS_PER_PIXEL*p])];
					output[k*64+CHANNELS_PER_PIXEL*p+1] = CCRF_LUT_GREEN[p].ROM[(input1[k*64+CHANNELS_PER_PIXEL*p+1],input2[k*64+CHANNELS_PER_PIXEL*p+1])];
					output[k*64+CHANNELS_PER_PIXEL*p+2] = CCRF_LUT_RED[p].ROM[(input1[k*64+CHANNELS_PER_PIXEL*p+2],input2[k*64+CHANNELS_PER_PIXEL*p+2])];
					output[k*64+CHANNELS_PER_PIXEL*p+3] = 255; // ALPHA CHANNEL
				}
			}
			for (int k = 0; k < CHANNELS_PER_PIXEL; k ++) {
			#pragma HLS pipeline
				for (int p = 0; p < 64; p++) {
				#pragma HLS unroll
					output_buffer[CHANNELS_PER_PIXEL*j+k].range(p*8+7,p*8) = output[64*k+p];
				}
			}
		}
		for (int copy_out = 0; copy_out < CHANNELS_PER_PIXEL; copy_out++) {
		  #pragma HLS UNROLL
		  void *source = &output_buffer[copy_out*BURST_LENGTH];
		  void *dest = &plmem[output_offset_64/64+(CHANNELS_PER_PIXEL*i+copy_out)*BURST_LENGTH];
		  memcpy(dest, source, BURST_LENGTH*sizeof(PLDDR_BUS_FLAT));
		}

	}

    bool done = true;
    job_done_stream.write(done);
}
*/

void CCRF_Compute(
	PIXEL4_FLAT_T *memory_bus,
	/*
    WIDE_DATA_FLAT_T *memory_bus,
    */

    hls::stream<JOB_SUBTASK_AXI> &job_info_in_queue,
    hls::stream<bool> &job_done_stream,

    const CCRF_LUT CCRF_LUT_BLUE[16],
    const CCRF_LUT CCRF_LUT_GREEN[16],
    const CCRF_LUT CCRF_LUT_RED[16]
) {
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_BLUE
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_GREEN
    #pragma HLS INTERFACE ap_memory port=CCRF_LUT_RED
    #pragma HLS array_partition variable=CCRF_LUT_BLUE complete
    #pragma HLS array_partition variable=CCRF_LUT_GREEN complete
    #pragma HLS array_partition variable=CCRF_LUT_RED complete

    //#pragma HLS INTERFACE m_axi port=memory_bus depth=2147483648 max_read_burst_length=64 max_write_burst_length=64 bundle="plmem"
    #pragma HLS INTERFACE m_axi port=memory_bus depth=2147483648
    #pragma HLS DATA_PACK variable=memory_bus

    #pragma HLS STREAM variable=job_info_in_queue depth=1
    #pragma HLS INTERFACE axis port=job_info_in_queue
    #pragma HLS STREAM variable=job_done_stream
    #pragma HLS INTERFACE axis port=job_done_stream

    #pragma HLS INTERFACE ap_none port=return

    JOB_SUBTASK_AXI job_info_in_tmp_axi = job_info_in_queue.read();
    ap_int<sizeof(JOB_SUBTASK)*8> job_info_in_raw_data = job_info_in_tmp_axi.data;
	JOB_SUBTASK job_info_in = *(JOB_SUBTASK*)&(job_info_in_raw_data);
    #pragma HLS DATA_PACK variable=job_info_in

    //WIDE_DATA_FLAT_T BRAM_input0[BURST_LENGTH];
    //WIDE_DATA_FLAT_T BRAM_input1[BURST_LENGTH];
    //WIDE_DATA_FLAT_T BRAM_output[BURST_LENGTH];
    //#pragma HLS DATA_PACK variable=BRAM_input0 struct_level
    //#pragma HLS DATA_PACK variable=BRAM_input1 struct_level
    //#pragma HLS DATA_PACK variable=BRAM_output struct_level

    const int pixels_per_memory_read = 16;

    const int pixels_per_memory_burst = pixels_per_memory_read*BURST_LENGTH;


    const uintptr_t in1_wide_data_offset = job_info_in.input1 / sizeof(PIXEL4_T);//(pixels_per_memory_read * sizeof(PIXEL4_T));
    const uintptr_t in2_wide_data_offset = job_info_in.input2 / sizeof(PIXEL4_T);//(pixels_per_memory_read * sizeof(PIXEL4_T));
    const uintptr_t out_wide_data_offset = job_info_in.output / sizeof(PIXEL4_T);//(pixels_per_memory_read * sizeof(PIXEL4_T));
    const uintptr_t iteration_count = job_info_in.image_size;//(job_info_in.image_size-1)/pixels_per_memory_read + 1;

    static_assert(BURST_LENGTH*sizeof(WIDE_DATA_T) == 4096, "burst size is wrong");

    for (int i = 0; i < iteration_count; i++) {
        #pragma HLS PIPELINE
        const uintptr_t input1_pixel_offset = in1_wide_data_offset+i;
        const uintptr_t input2_pixel_offset = in2_wide_data_offset+i;
        const uintptr_t output_pixel_offset = out_wide_data_offset+i;

        WIDE_DATA_T output_wide_pixel;

        WIDE_DATA_FLAT_T input1_wide_pixel_flat = memory_bus[input1_pixel_offset];
        WIDE_DATA_FLAT_T input2_wide_pixel_flat = memory_bus[input2_pixel_offset];

        WIDE_DATA_T input1_wide_pixel;
        WIDE_DATA_T input2_wide_pixel;
        for (int byte = 0; byte < sizeof(PIXEL4_T);byte++) {//pixels_per_memory_read * sizeof(PIXEL4_T); byte++) {
            #pragma HLS UNROLL
            input1_wide_pixel.data[byte] = input1_wide_pixel_flat.range(byte*8+7,byte*8);
            input2_wide_pixel.data[byte] = input2_wide_pixel_flat.range(byte*8+7,byte*8);
        }

        // INPUT IMAGE 1
        BYTE_T B1_0 = input1_wide_pixel.data[0];
        BYTE_T G1_0 = input1_wide_pixel.data[1];
        BYTE_T R1_0 = input1_wide_pixel.data[2];
        /*
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
        */

        // INPUT IMAGE 2
        BYTE_T B2_0 = input2_wide_pixel.data[0];
        BYTE_T G2_0 = input2_wide_pixel.data[1];
        BYTE_T R2_0 = input2_wide_pixel.data[2];
        /*
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
        */

        uint16_t LUT_entry_b0 = (uint16_t)B1_0 + 256 * (uint16_t)B2_0;
        uint16_t LUT_entry_g0 = (uint16_t)G1_0 + 256 * (uint16_t)G2_0;
        uint16_t LUT_entry_r0 = (uint16_t)R1_0 + 256 * (uint16_t)R2_0;
        /*
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
        */

        output_wide_pixel.data[0] = CCRF_LUT_BLUE[0].ROM[LUT_entry_b0];
        output_wide_pixel.data[1] = CCRF_LUT_GREEN[0].ROM[LUT_entry_g0];
        output_wide_pixel.data[2] = CCRF_LUT_RED[0].ROM[LUT_entry_r0];
        output_wide_pixel.data[3] = (BYTE_T)255;
        /*
        output_wide_pixel.data[4] = CCRF_LUT_BLUE[1].ROM[LUT_entry_b1];
        output_wide_pixel.data[5] = CCRF_LUT_GREEN[1].ROM[LUT_entry_g1];
        output_wide_pixel.data[6] = CCRF_LUT_RED[1].ROM[LUT_entry_r1];
        output_wide_pixel.data[7] = (BYTE_T)255;
        output_wide_pixel.data[8] = CCRF_LUT_BLUE[2].ROM[LUT_entry_b2];
        output_wide_pixel.data[9] = CCRF_LUT_GREEN[2].ROM[LUT_entry_g2];
        output_wide_pixel.data[10] = CCRF_LUT_RED[2].ROM[LUT_entry_r2];
        output_wide_pixel.data[11] = (BYTE_T)255;
        output_wide_pixel.data[12] = CCRF_LUT_BLUE[3].ROM[LUT_entry_b3];
        output_wide_pixel.data[13] = CCRF_LUT_GREEN[3].ROM[LUT_entry_g3];
        output_wide_pixel.data[14] = CCRF_LUT_RED[3].ROM[LUT_entry_r3];
        output_wide_pixel.data[15] = (BYTE_T)255;
            
        output_wide_pixel.data[16] = CCRF_LUT_BLUE[4].ROM[LUT_entry_b4];
        output_wide_pixel.data[17] = CCRF_LUT_GREEN[4].ROM[LUT_entry_g4];
        output_wide_pixel.data[18] = CCRF_LUT_RED[4].ROM[LUT_entry_r4];
        output_wide_pixel.data[19] = (BYTE_T)255;
        output_wide_pixel.data[20] = CCRF_LUT_BLUE[5].ROM[LUT_entry_b5];
        output_wide_pixel.data[21] = CCRF_LUT_GREEN[5].ROM[LUT_entry_g5];
        output_wide_pixel.data[22] = CCRF_LUT_RED[5].ROM[LUT_entry_r5];
        output_wide_pixel.data[23] = (BYTE_T)255;
        output_wide_pixel.data[24] = CCRF_LUT_BLUE[6].ROM[LUT_entry_b6];
        output_wide_pixel.data[25] = CCRF_LUT_GREEN[6].ROM[LUT_entry_g6];
        output_wide_pixel.data[26] = CCRF_LUT_RED[6].ROM[LUT_entry_r6];
        output_wide_pixel.data[27] = (BYTE_T)255;
        output_wide_pixel.data[28] = CCRF_LUT_BLUE[7].ROM[LUT_entry_b7];
        output_wide_pixel.data[29] = CCRF_LUT_GREEN[7].ROM[LUT_entry_g7];
        output_wide_pixel.data[30] = CCRF_LUT_RED[7].ROM[LUT_entry_r7];
        output_wide_pixel.data[31] = (BYTE_T)255;
            
        output_wide_pixel.data[32] = CCRF_LUT_BLUE[8].ROM[LUT_entry_b8];
        output_wide_pixel.data[33] = CCRF_LUT_GREEN[8].ROM[LUT_entry_g8];
        output_wide_pixel.data[34] = CCRF_LUT_RED[8].ROM[LUT_entry_r8];
        output_wide_pixel.data[35] = (BYTE_T)255;
        output_wide_pixel.data[36] = CCRF_LUT_BLUE[9].ROM[LUT_entry_b9];
        output_wide_pixel.data[37] = CCRF_LUT_GREEN[9].ROM[LUT_entry_g9];
        output_wide_pixel.data[38] = CCRF_LUT_RED[9].ROM[LUT_entry_r9];
        output_wide_pixel.data[39] = (BYTE_T)255;
        output_wide_pixel.data[40] = CCRF_LUT_BLUE[10].ROM[LUT_entry_b10];
        output_wide_pixel.data[41] = CCRF_LUT_GREEN[10].ROM[LUT_entry_g10];
        output_wide_pixel.data[42] = CCRF_LUT_RED[10].ROM[LUT_entry_r10];
        output_wide_pixel.data[43] = (BYTE_T)255;
        output_wide_pixel.data[44] = CCRF_LUT_BLUE[11].ROM[LUT_entry_b11];
        output_wide_pixel.data[45] = CCRF_LUT_GREEN[11].ROM[LUT_entry_g11];
        output_wide_pixel.data[46] = CCRF_LUT_RED[11].ROM[LUT_entry_r11];
        output_wide_pixel.data[47] = (BYTE_T)255;
            
        output_wide_pixel.data[48] = CCRF_LUT_BLUE[12].ROM[LUT_entry_b12];
        output_wide_pixel.data[49] = CCRF_LUT_GREEN[12].ROM[LUT_entry_g12];
        output_wide_pixel.data[50] = CCRF_LUT_RED[12].ROM[LUT_entry_r12];
        output_wide_pixel.data[51] = (BYTE_T)255;
        output_wide_pixel.data[52] = CCRF_LUT_BLUE[13].ROM[LUT_entry_b13];
        output_wide_pixel.data[53] = CCRF_LUT_GREEN[13].ROM[LUT_entry_g13];
        output_wide_pixel.data[54] = CCRF_LUT_RED[13].ROM[LUT_entry_r13];
        output_wide_pixel.data[55] = (BYTE_T)255;
        output_wide_pixel.data[56] = CCRF_LUT_BLUE[14].ROM[LUT_entry_b14];
        output_wide_pixel.data[57] = CCRF_LUT_GREEN[14].ROM[LUT_entry_g14];
        output_wide_pixel.data[58] = CCRF_LUT_RED[14].ROM[LUT_entry_r14];
        output_wide_pixel.data[59] = (BYTE_T)255;
        output_wide_pixel.data[60] = CCRF_LUT_BLUE[15].ROM[LUT_entry_b15];
        output_wide_pixel.data[61] = CCRF_LUT_GREEN[15].ROM[LUT_entry_g15];
        output_wide_pixel.data[62] = CCRF_LUT_RED[15].ROM[LUT_entry_r15];
        output_wide_pixel.data[63] = (BYTE_T)255;
        */
            
        //memory_bus[output_pixel_offset] = output_wide_pixel;
        WIDE_DATA_FLAT_T output_data_flat;

        for (int byte = 0; byte < sizeof(PIXEL4_T); byte++) {
            #pragma HLS UNROLL
            output_data_flat.range(byte*8+7, byte*8) = output_wide_pixel.data[byte];
        }

        memory_bus[output_pixel_offset] = output_data_flat;
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

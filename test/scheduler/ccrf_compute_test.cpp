#include "ccrf.hpp"
#include "types.hpp"
int main() {

  CCRF_LUT LUT_BLUE[16];
  CCRF_LUT LUT_GREEN[16];
  CCRF_LUT LUT_RED[16];

#pragma HLS array_partition variable=LUT_BLUE dim=1
#pragma HLS array_partition variable=LUT_GREEN dim=1
#pragma HLS array_partition variable=LUT_RED dim=1

  BYTE_T value_blue = 0;
  BYTE_T value_green = 64;
  BYTE_T value_red = 128;

  for (int lut = 0; lut < 16; lut++) {
    value_blue = 0;
	value_green = 64;
	value_red = 128;
    for (int i = 0; i < 256; i++) {
	  for (int j = 0; j < 256; j++) {
		  LUT_BLUE[lut].ROM[i * 256 + j] = value_blue++;
		  LUT_GREEN[lut].ROM[i * 256 + j] = value_green++;
		  LUT_RED[lut].ROM[i * 256 + j] = value_red++;
	  }
    }
  }

  const int image_size = 1920*1080;
  BYTE_T *image1_unaligned = new BYTE_T[image_size * sizeof(PIXEL4_T) + 64*4096];
  BYTE_T *image2_unaligned = new BYTE_T[image_size * sizeof(PIXEL4_T) + 64*4096];
  BYTE_T *output_unaligned = new BYTE_T[image_size * sizeof(PIXEL4_T) + 64*4096];
  BYTE_T *image1 = image1_unaligned + (4096 - ((uintptr_t)image1_unaligned % 4096));
  BYTE_T *image2 = image2_unaligned + (4096 - ((uintptr_t)image2_unaligned % 4096));
  BYTE_T *output = output_unaligned + (4096 - ((uintptr_t)output_unaligned % 4096));
  assert((uintptr_t)image1 % 4096 == 0);
  assert((uintptr_t)image2 % 4096 == 0);
  assert((uintptr_t)output % 4096 == 0);
  for (int i = 0; i < image_size; i++) {
	  image1[4*i] = rand();//value_blue++;
	  image1[4*i+1] = rand();//value_green++;
	  image1[4*i+2] = rand();//value_red++;
	  image1[4*i+3] = 255;
  }
  value_blue += 64;
  value_green += 64;
  value_red += 64;
  for (int i = 0; i < image_size; i++) {
	  image2[4*i] = rand();//value_blue++;
	  image2[4*i+1] = rand();//value_green++;
	  image2[4*i+2] = rand();//value_red++;
	  image2[4*i+3] = 255;
  }

  hls::stream<JOB_SUBTASK_AXI> job_info_in_queue;
  hls::stream<bool> job_done_stream;

  JOB_SUBTASK job_subtask;
  job_subtask.output = (uintptr_t)output;
  job_subtask.input1 = (uintptr_t)image1;
  job_subtask.input2 = (uintptr_t)image2;
  job_subtask.image_size = image_size;
  job_subtask.job_ID = 1;

  JOB_SUBTASK_AXI job_subtask_axi;
  job_subtask_axi.data = *(ap_int<sizeof(JOB_SUBTASK) * 8>*)&(job_subtask);
  job_info_in_queue.write(job_subtask_axi);
  while (job_done_stream.empty()) {
  	CCRF_Compute(
  		nullptr/*WIDE_DATA_FLAT_T *memory_bus*/,

  	    job_info_in_queue,
    	job_done_stream,

		LUT_BLUE, LUT_GREEN, LUT_RED
	);
  }

  assert(!job_done_stream.empty());

  for (int i = 0; i < image_size; i++) {
	  const uint16_t image1_blue = image1[i*4];
	  const uint16_t image1_green = image1[i*4+1];
	  const uint16_t image1_red = image1[i*4+2];
	  const uint16_t image2_blue = image2[i*4];
	  const uint16_t image2_green = image2[i*4+1];
	  const uint16_t image2_red = image2[i*4+2];
	  const uint16_t output_blue = output[i*4];
	  const uint16_t output_green = output[i*4+1];
	  const uint16_t output_red = output[i*4+2];
	  const uint16_t lut_blue_idx = (uint16_t)image1_blue * 256 + (uint16_t)image2_blue;
	  const uint16_t lut_green_idx = (uint16_t)image1_green * 256 + (uint16_t)image2_green;
	  const uint16_t lut_red_idx = (uint16_t)image1_red * 256 + (uint16_t)image2_red;
	  const uint16_t lut_blue_result = LUT_BLUE[0].ROM[lut_blue_idx];
	  const uint16_t lut_green_result = LUT_GREEN[0].ROM[lut_green_idx];
	  const uint16_t lut_red_result = LUT_RED[0].ROM[lut_red_idx];
	  assert(output_blue == lut_blue_result);
	  assert(output_green == lut_green_result);
	  assert(output_red == lut_red_result);
  }

  return 0;
}

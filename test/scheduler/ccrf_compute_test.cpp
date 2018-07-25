#include "ccrf.hpp"
#include "types.hpp"
int main() {

  BYTE_T LUT_BLUE[65535];
  BYTE_T LUT_GREEN[65535];
  BYTE_T LUT_RED[65535];

  BYTE_T value_blue = 0;
  BYTE_T value_green = 64;
  BYTE_T value_red = 128;
  for (int i = 0; i < 255; i++) {
	  for (int j = 0; j < 256; j++) {
		  LUT_BLUE[i * 256 + j] = value_blue++;
		  LUT_GREEN[i * 256 + j] = value_green++;
		  LUT_RED[i * 256 + j] = value_red++;
	  }
  }

  const int image_size = 1920*1080;
  BYTE_T *image1_unaligned = new BYTE_T[image_size * sizeof(PIXEL4_T) + 5000];
  BYTE_T *image2_unaligned = new BYTE_T[image_size * sizeof(PIXEL4_T) + 5000];
  BYTE_T *output_unaligned = new BYTE_T[image_size * sizeof(PIXEL4_T) + 5000];
  BYTE_T *image1 = image1_unaligned + (4096 - ((uintptr_t)image1_unaligned % 4096));
  BYTE_T *image2 = image2_unaligned + (4096 - ((uintptr_t)image2_unaligned % 4096));
  BYTE_T *output = output_unaligned + (4096 - ((uintptr_t)output_unaligned % 4096));
  for (int i = 0; i < image_size; i++) {
	  image1[4*i] = value_blue++;
	  image1[4*i+1] = value_green++;
	  image1[4*i+2] = value_red++;
	  image1[4*i+3] = 255;
  }
  value_blue += 64;
  value_green += 64;
  value_red += 64;
  for (int i = 0; i < image_size; i++) {
	  image2[4*i] = value_blue++;
	  image2[4*i+1] = value_green++;
	  image2[4*i+2] = value_red++;
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
		LUT_BLUE, LUT_GREEN, LUT_RED,
		LUT_BLUE, LUT_GREEN, LUT_RED,
		LUT_BLUE, LUT_GREEN, LUT_RED,
		LUT_BLUE, LUT_GREEN, LUT_RED,
		LUT_BLUE, LUT_GREEN, LUT_RED,
		LUT_BLUE, LUT_GREEN, LUT_RED,
		LUT_BLUE, LUT_GREEN, LUT_RED,
		LUT_BLUE, LUT_GREEN, LUT_RED,
		LUT_BLUE, LUT_GREEN, LUT_RED,
		LUT_BLUE, LUT_GREEN, LUT_RED,
		LUT_BLUE, LUT_GREEN, LUT_RED,
		LUT_BLUE, LUT_GREEN, LUT_RED,
		LUT_BLUE, LUT_GREEN, LUT_RED,
		LUT_BLUE, LUT_GREEN, LUT_RED,
		LUT_BLUE, LUT_GREEN, LUT_RED,
		LUT_BLUE, LUT_GREEN, LUT_RED,

		nullptr/*WIDE_DATA_FLAT_T *memory_bus*/,

	    job_info_in_queue,
		job_done_stream
	);
  }

  assert(!job_done_stream.empty());

  for (int i = 0; i < image_size; i++) {
	  assert(output[i*4] == LUT_BLUE[
									 (uint16_t)image1[i*4] * 256 +
									 (uint16_t)image2[i*4]
									]);
	  assert(output[i*4+1] == LUT_GREEN[
	  									 (uint16_t)image1[i*4+1] * 256 +
	  									 (uint16_t)image2[i*4+1]
	  									]);
	  assert(output[i*4+2] == LUT_RED[
	  									 (uint16_t)image1[i*4+2] * 256 +
	  									 (uint16_t)image2[i*4+2]
	  									]);
  }

}

#ifndef UTILS_H
#define UTILS_H

#include "types.hpp"
#include <string>
#include <vector>

class JobDescriptor;

void Convert4ChannelTo3Channel(void *const image, const int pixel_count);
void Convert32bitChannelTo8BitChannels(void *const image, const int pixel_count, const int scaler);

void TonemapHDRImage(JobDescriptor &job_descriptor);

std::vector<std::string> ReadFileContents(std::string file_path);

IMAGE_STACK_T ReadImageStackImages(std::string image_stack_file_list);

bool WriteImageToFile(IMAGE_T image, std::string image_file_path);

IMAGE_T ReadImageFile(std::string image_file_path);

#endif

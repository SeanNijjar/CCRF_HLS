#ifndef UTILS_H
#define UTILS_H

#include "types.hpp"

// for the .cpp
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

std::vector<std::string> ReadFileContents(std::string file_path);

IMAGE_STACK_T ReadImageStackImages(std::string image_stack_file_list);

bool WriteImageToFile(IMAGE_T image, std::string image_file_path);

IMAGE_T ReadImageFile(std::string image_file_path);

#endif
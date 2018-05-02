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

std::vector<std::string> ReadFileContents(std::string file_path) 
{
    std::vector<std::string> contents;
    std::ifstream file(file_path, std::ifstream::in);
    
    std::string file_line;
    std::getline(file, file_line);
    while (file.good()) {
        contents.push_back(file_line);
        std::getline(file, file_line);
    }

    return contents;
}

IMAGE_STACK_T ReadImageStackImages(std::string image_stack_file_list) 
{
    IMAGE_STACK_T image_stack;
    std::ifstream file_list(image_stack_file_list, std::ifstream::in);
    
    std::string LDR_image_file;
    std::getline(file_list, LDR_image_file);
    while (file_list.good()) {
        image_stack.push_back(LDR_image_file);
        std::getline(file_list, LDR_image_file);
    }

    return image_stack;
}

bool WriteImageToFile(IMAGE_T image, std::string image_file_path)
{
    cv::Mat image_data(cv::Size(image.height, image.width), CV_8UC3, image.data);
    bool write_result = cv::imwrite(image_file_path, CV_LOAD_IMAGE_COLOR);
    if (!write_result) {
        std::cout << "Couldn't write image file " << image_file_path << std::endl;
        return false;
    }

    return true;
}

IMAGE_T ReadImageFile(std::string image_file_path) 
{    
    cv::Mat image_matrix = cv::imread(image_file_path, CV_LOAD_IMAGE_COLOR);
    if (!image_matrix.data) {
        std::cout << "Couldn't open image file " << image_file_path << std::endl;
    }

    return {(PIXEL_T*)image_matrix.data, image_matrix.rows, image_matrix.cols};
}

#endif
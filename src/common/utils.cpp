#include "utils.hpp"

// for the .cpp
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <opencv2/opencv.hpp>


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
    std::ifstream file_list(image_stack_file_list);
    
    std::string LDR_image_file;
    while (std::getline(file_list, LDR_image_file)) {
        if (LDR_image_file.empty()) {
            continue;
        }
        image_stack.push_back(ReadImageFile(LDR_image_file));
    }

    return image_stack;
}

bool WriteImageToFile(IMAGE_T image, std::string image_file_path)
{
    cv::Mat image_data(cv::Size(image.width, image.height), CV_8UC3, image.data);
    bool write_result = cv::imwrite(image_file_path, image_data);//, CV_LOAD_IMAGE_COLOR);
    if (!write_result) {
        std::cout << "Couldn't write image file " << image_file_path << std::endl;
        return false;
    }

    return true;
}

IMAGE_T ReadImageFile(std::string image_file_path) 
{    
    // TODO: make it allocate new memory for each LDR image
    cv::Mat image_matrix = cv::imread(image_file_path);
    PIXEL_T* image_matrix_data = nullptr;
    if (!image_matrix.data) {
        std::cout << "Couldn't open image file " << image_file_path << std::endl;
    } else {    
        const int pixel_count = image_matrix.rows * image_matrix.cols;
        image_matrix_data = new PIXEL_T[pixel_count];
        memcpy(image_matrix_data, image_matrix.data, pixel_count * sizeof(PIXEL_T));
    }
    return {(PIXEL_T*)image_matrix_data, image_matrix.rows, image_matrix.cols};
}


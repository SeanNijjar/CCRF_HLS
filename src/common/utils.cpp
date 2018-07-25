#include "utils.hpp"
#include "helper.hpp"
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

void TonemapHDRImage(JobDescriptor &job_descriptor)
{
    cv::Mat tonemapped_ldr_image;
    cv::Mat hdr_image(cv::Size(job_descriptor.IMAGE_HEIGHT, job_descriptor.IMAGE_WIDTH),
                      CV_32UC3,
                      job_descriptor.OUTPUT_IMAGE_LOCATION);
    cv::Ptr<cv::TonemapDurand> tonemapper = cv::createTonemapDurand(2.2f);
    tonemapper->process(hdr_image, tonemapped_ldr_image);
    memcpy(job_decriptor.OUTPUT_IMAGE_LOCATION, tonemapped_ldr_image, job_descriptor.IMAGE_SIZE() * sizeof(HDR_PIXEL3_T));
}

bool WriteImageToFile(IMAGE_T image, std::string image_file_path)
{
    cv::Mat image_data(cv::Size(image.height, image.width), CV_8UC4, image.data);
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
    PIXEL4_T* image_matrix_data = nullptr;
    if (!image_matrix.data) {
        std::cout << "Couldn't open image file " << image_file_path << std::endl;
    } else {    
        const int pixel_count = image_matrix.rows * image_matrix.cols;
        image_matrix_data = new PIXEL4_T[pixel_count];
        for (int pixel = 0; pixel < pixel_count; pixel++) {
            PIXEL4_T pixel4_data;
            memcpy(&pixel4_data, &image_matrix.data[pixel*sizeof(PIXEL_T)], sizeof(PIXEL_T));
            pixel4_data[3] = -1;
            for (int channel = 0; channel < 4; channel++) {
                image_matrix_data[pixel][channel] = pixel4_data[channel];
            }
        }
        //memcpy(image_matrix_data, image_matrix.data, pixel_count * sizeof(PIXEL_T));

        ASSERT(image_matrix_data[0][0] == image_matrix.data[0], "pixel value mismatch after preprocessing");
        ASSERT(image_matrix_data[0][1] == image_matrix.data[1], "pixel value mismatch after preprocessing");
        ASSERT(image_matrix_data[0][2] == image_matrix.data[2], "pixel value mismatch after preprocessing");
        ASSERT(image_matrix_data[1][0] == image_matrix.data[3], "pixel value mismatch after preprocessing");
        ASSERT(image_matrix_data[1][1] == image_matrix.data[4], "pixel value mismatch after preprocessing");
        ASSERT(image_matrix_data[1][2] == image_matrix.data[5], "pixel value mismatch after preprocessing");

	std::cout << "Image pixel values: {B,G,R,A}: " << std::endl;
        for (int pixel = 0; pixel < 2; pixel++) {
            std::cout << " {";
            for (int channel = 0; channel < 4; channel++) {
		if (channel > 0) {
                    std::cout << ", ";
                } 
                std::cout << (int)image_matrix_data[pixel][channel];
            }
            std::cout << " }";
        }
    }

    

    return {(PIXEL_T*)image_matrix_data, image_matrix.rows, image_matrix.cols};
}




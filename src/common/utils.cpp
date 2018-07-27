#include "utils.hpp"
#include "helper.hpp"
// for the .cpp
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <opencv2/opencv.hpp>
#include <opencv2/photo.hpp>
#include "job_descriptor.hpp"

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

void Convert32bitChannelTo8BitChannels(void *const image, const int pixel_count, const int scaler)
{
    std::cout << "Converting from 32bit float to 8but uint" << std::endl;
    char *hdr_image = (char*)image;
    char *ldr_image = (char*)image;
    const int ldr_pixel_size = sizeof(PIXEL3_T);
    const int hdr_pixel_size = sizeof(HDR_PIXEL3_T);

    for(int i = 0; i < pixel_count; i++, hdr_image += hdr_pixel_size, ldr_image += ldr_pixel_size) {
        if (i < 32) {
          std::cout << "[" << i << "]: " << hdr_image[0] << ", " << hdr_image[1] << ", " << hdr_image[2] << std::endl;
        }
	HDR_PIXEL3_T hdr_pixel;
        memcpy(&hdr_pixel, hdr_image, hdr_pixel_size);
	PIXEL3_T ldr_pixel;
	ldr_pixel[0] = hdr_pixel[0] * scaler;
	ldr_pixel[1] = hdr_pixel[1] * scaler;
	ldr_pixel[2] = hdr_pixel[2] * scaler;

        memcpy(ldr_image, &ldr_pixel, ldr_pixel_size);
        if (i < 32) {
          std::cout << "[" << i << "]: " << (uint16_t)ldr_pixel[0] << ", " << (uint16_t)ldr_pixel[1] << ", " << (uint16_t)ldr_pixel[2] << std::endl;
        }
    }

}

void Convert4ChannelTo3Channel(void *const image, const int pixel_count)
{
    char * image_4_channel = (char*)image;
    char * image_3_channel = (char*)image;
    const int pixel4_size = sizeof(HDR_PIXEL4_T);
    const int pixel3_size = sizeof(HDR_PIXEL3_T);
    for(int i = 0; i < pixel_count; i++, image_4_channel += pixel4_size, image_3_channel += pixel3_size) {
        memcpy(image_3_channel, image_4_channel, pixel3_size);
        HDR_PIXEL3_T dest_pixel;
        memcpy(&dest_pixel, image_3_channel, pixel3_size);;
        if (i < 32) {
           std::cout << "[" << i << "]: " << dest_pixel[0] << ", " << dest_pixel[1] << ", " << dest_pixel[2] << std::endl;
        }
    }
}

void TonemapHDRImage(JobDescriptor &job_descriptor)
{
    HDR_PIXEL3_T *output_image_location = (HDR_PIXEL3_T*)job_descriptor.OUTPUT_IMAGE_LOCATION;;
//    HDR_PIXEL3_T *tmp_image = new HDR_PIXEL3_T[job_descriptor.IMAGE_SIZE()];
    cv::Mat tonemapped_ldr_image;//(cv::Size(job_descriptor.IMAGE_HEIGHT, job_descriptor.IMAGE_WIDTH),
//                      CV_32FC3,
//                      tmp_image);
    cv::Mat hdr_image(cv::Size(job_descriptor.IMAGE_HEIGHT, job_descriptor.IMAGE_WIDTH),
                      CV_32FC3,
                      output_image_location);
    cv::Ptr<cv::TonemapDurand> tonemapper = cv::createTonemapDurand(2.2f);
    tonemapper->process(hdr_image, tonemapped_ldr_image);
    memcpy((void*)output_image_location, tonemapped_ldr_image.data, (size_t)(job_descriptor.IMAGE_SIZE() * sizeof(HDR_PIXEL3_T)));

    float min_val = 1.0f;
    float max_val = 0.0f;
    for (int i = 0; i < job_descriptor.IMAGE_SIZE(); i++) {
      for (int j = 0; j < 3; j++) {
        if (output_image_location[i][j] < min_val) {
          min_val = output_image_location[i][j];
        }
        if (output_image_location[i][j] > max_val) {
          max_val = output_image_location[i][j];
        }
//        if (output_image_location[i][j] >= 1.) {
//          output_image_location[i][j] = 1.;
//        }
      }
    }


    std::cout << "MAX_VAL=" << max_val << " MIN_VAL=" << min_val << std::endl;

    for (int i = 0; i < 16; i++) {
      std::cout << "[" << i << "]: " << output_image_location[i][0] << ", " << output_image_location[i][1] << ", " << output_image_location[i][2] << std::endl;
    }

//    delete tmp_image;
}

bool WriteImageToFile(IMAGE_T image, std::string image_file_path)
{
    cv::Mat image_data(cv::Size(image.height, image.width), CV_8UC3, image.data);
    //cv::Mat image_data(cv::Size(image.height, image.width), CV_32FC3, image.data);
    //image_data = image_data * 255;
    bool write_result = cv::imwrite(image_file_path, image_data);//, CV_LOAD_IMAGE_COLOR);
    //bool write_result = cv::imwrite(image_file_path, image_data);//, CV_LOAD_IMAGE_COLOR);
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
            memcpy(&pixel4_data, &image_matrix.data[pixel*sizeof(PIXEL3_T)], sizeof(PIXEL3_T));
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
        for (int pixel = 0; pixel < 16; pixel++) {
            std::cout << " {";
            for (int channel = 0; channel < 4; channel++) {
		if (channel > 0) {
                    std::cout << ", ";
                } 
                std::cout << (int)image_matrix_data[pixel][channel];
            }
            std::cout << " }";
        }
        std::cout << std::endl;
    }

    

    return {(PIXEL_T*)image_matrix_data, image_matrix.rows, image_matrix.cols};
}




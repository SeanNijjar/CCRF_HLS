#ifndef JOB_DESCRIPTOR_H
#define JOB_DESCRIPTOR_H

#include "types.hpp"
#include "helper.hpp"

#include <cstdint>
#include <stddef.h>
#include <string.h>
#include <memory>

struct JobDescriptor 
{
    //PIXEL_T *OUTPUT_IMAGE_LOCATION;   // Where to dump the HDR image
    //PIXEL_T *INPUT_IMAGES[10];   // The memory locations of the input images // Harcode to 10 max for now to make transmission in FPGA easier
    uintptr_t OUTPUT_IMAGE_LOCATION;
    uintptr_t INPUT_IMAGES[6];
	uint16_t IMAGE_WIDTH;
	uint16_t IMAGE_HEIGHT;
    uint8_t LDR_IMAGE_COUNT;   // Number of LDR images that correspond to the single output HDR frame

    int IMAGE_SIZE() const
    {
        return IMAGE_WIDTH * IMAGE_HEIGHT;
    }

    static JobDescriptor *Create(IMAGE_STACK_T image_stack)
    {
        const int num_images = image_stack.size();
        ASSERT(num_images > 0, "Given an empty image stack");
        
        const int height = image_stack[0].height;
        const int width = image_stack[0].width;

        JobDescriptor *job_descriptor = new JobDescriptor();//InterpretRawBufferAsJobDescriptor(
                                                //new BYTE_T[JobDescriptor::BytesNeededForJobDescriptor(num_images)]
                                              //);
        job_descriptor->IMAGE_WIDTH = width;
        job_descriptor->IMAGE_HEIGHT = height;
        job_descriptor->LDR_IMAGE_COUNT = num_images;
        for (int i = 0; i < num_images; i++) {//auto image : image_stack) {
            auto &image = image_stack[i];
            ASSERT(image.width == width, "Image width dimensions for image " << i << " in stack don't match image 0 width");
            ASSERT(image.height == height, "Image width dimensions for image " << i << " in stack don't match image 0 width");
            job_descriptor->INPUT_IMAGES[i] = (uintptr_t)image.data;
        }
    
        return job_descriptor;
    }

    static const int BytesNeededForEntireJob(const JobDescriptor *const job_descriptor) {
        const int image_size = job_descriptor->IMAGE_SIZE();
        return BytesNeededForJobDescriptor(job_descriptor) + (image_size * sizeof(PIXEL_T) * job_descriptor->LDR_IMAGE_COUNT);
    }

    static const int BytesNeededForJobDescriptor(const JobDescriptor *const job_descriptor) {
        const int num_images = job_descriptor->LDR_IMAGE_COUNT;
        return BytesNeededForJobDescriptor(num_images);
    }

    static const int BytesNeededForJobDescriptor(const int num_images) {
        return offsetof(JobDescriptor, INPUT_IMAGES) + sizeof(PIXEL_T*) * num_images;
    }

    //static void SetImageSize(JobDescriptor *const job_descriptor_impl, const int image_size) {
    //    job_descriptor_impl->IMAGE_SIZE = image_size;
    //}

    static void SetImageCount(JobDescriptor *const job_descriptor_impl, const int image_count) {
        job_descriptor_impl->LDR_IMAGE_COUNT = image_count;
    }

    static void AddInputImageLocation(JobDescriptor *const job_descriptor_impl, PIXEL_T * const input_location, const int image_num) {
        job_descriptor_impl->INPUT_IMAGES[image_num] = (uintptr_t)input_location;
    }

    static void CopyToRawBuffer(BYTE_T *raw_buffer, const JobDescriptor *const job_descriptor_impl) {
        ASSERT(raw_buffer != nullptr, "raw_buffer is a nullptr");
        ASSERT(job_descriptor_impl->LDR_IMAGE_COUNT > 0, "Bad job descriptor copy. LDR_IMAGE_COUNT not initialized")
        ASSERT(job_descriptor_impl->IMAGE_SIZE() > 0, "Bad job descriptor copy. IMAGE_SIZE not initialized")
        ASSERT((PIXEL_T*)job_descriptor_impl->OUTPUT_IMAGE_LOCATION != nullptr, "Bad job descriptor copy. OUTPUT_IMAGE_LOCATION not initialized")
        for (int image = 0; image < job_descriptor_impl->LDR_IMAGE_COUNT; image++) {
            ASSERT((PIXEL_T*)job_descriptor_impl->INPUT_IMAGES[image] != nullptr, "Bad job descriptor copy. One of the input image addresses was not assigned")
        }

        memcpy(raw_buffer, job_descriptor_impl, BytesNeededForJobDescriptor(job_descriptor_impl->LDR_IMAGE_COUNT));
    }

    static JobDescriptor *InterpretRawBufferAsJobDescriptor(BYTE_T *raw_buffer) {
        ASSERT((unsigned long)raw_buffer % alignof(JobDescriptor) == 0, "misaligned raw buffer trying to be interpreted as JobDescriptor");
        ASSERT(raw_buffer != nullptr, "raw_buffer is a nullptr");

        return (JobDescriptor*)raw_buffer;
    }

    static const int NumberOfLDRImages(const JobDescriptor *const job_descriptor_impl) {
        ASSERT(job_descriptor_impl != nullptr, "job_descriptor_impl == nullptr");
        return job_descriptor_impl->LDR_IMAGE_COUNT;
    }

    static const int ImageSize(const JobDescriptor *const job_descriptor_impl) {
        ASSERT(job_descriptor_impl != nullptr, "job_descriptor_impl == nullptr");
        return job_descriptor_impl->IMAGE_SIZE();
    }

    static const PIXEL_T *OutputImageLocation(const JobDescriptor *const job_descriptor_impl) {
        ASSERT(job_descriptor_impl != nullptr, "job_descriptor_impl == nullptr");
        return (PIXEL_T*)job_descriptor_impl->OUTPUT_IMAGE_LOCATION;
    }

    static const PIXEL_T *InputImageLocation(const JobDescriptor *const job_descriptor_impl, const int image_num) {
        ASSERT(job_descriptor_impl != nullptr, "job_descriptor_impl == nullptr");
        ASSERT(image_num < job_descriptor_impl->LDR_IMAGE_COUNT, "Tried to get input image location for image number that was greater than total number of input images");
        return (PIXEL_T*)job_descriptor_impl->INPUT_IMAGES[image_num];
    }

    
};

#endif

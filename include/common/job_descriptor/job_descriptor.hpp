#ifndef JOB_DESCRIPTOR_H
#define JOB_DESCRIPTOR_H

#include "types.hpp"
#include "helper.hpp"

#include <stddef.h>
#include <string.h>
#include <memory>

class JobDescriptor 
{
  private:
    struct JOB_DESCRIPTOR_IMPL {
        int LDR_IMAGE_COUNT;   // Number of LDR images that correspond to the single output HDR frame
        int IMAGE_SIZE;   // The number of pixels per LDR/HDR image (= to width * height)
        PIXEL_T *OUTPUT_IMAGE_LOCATION;   // Where to dump the HDR image
        PIXEL_T *INPUT_IMAGES[];   // The memory locations of the input images
    };


  public:
    using JOB_DESCRIPTOR_T = JOB_DESCRIPTOR_IMPL;
    const JOB_DESCRIPTOR_T *GetHandle();

    

    static JOB_DESCRIPTOR_T *Create(IMAGE_STACK_T image_stack)
    {
        const int num_images = image_stack.size();
        ASSERT(num_images > 0, "Given an empty image stack");
        
        const int height = image_stack[0].height;
        const int width = image_stack[0].width;

        JOB_DESCRIPTOR_IMPL *job_descriptor = InterpretRawBufferAsJobDescriptor(
                                                new BYTE_T[JobDescriptor::BytesNeededForJobDescriptor(num_images)]
                                              );

        int i = 0;
        for (auto image : image_stack) {
            ASSERT(image.width == width, "Image width dimensions for image " << i << " in stack don't match image 0 width");
            ASSERT(image.height == height, "Image width dimensions for image " << i << " in stack don't match image 0 width");
            job_descriptor->INPUT_IMAGES[i] = image.data;
        }
    
    }

    static const int BytesNeededForEntireJob(const JOB_DESCRIPTOR_T *const job_descriptor) {
        return BytesNeededForJobDescriptor(job_descriptor) + job_descriptor->IMAGE_SIZE * job_descriptor->LDR_IMAGE_COUNT;
    }

    static const int BytesNeededForJobDescriptor(const JOB_DESCRIPTOR_T *const job_descriptor) {
        const int num_images = job_descriptor->LDR_IMAGE_COUNT;
        return BytesNeededForJobDescriptor(num_images);
    }

    static const int BytesNeededForJobDescriptor(const int num_images) {
        return offsetof(JOB_DESCRIPTOR_IMPL, INPUT_IMAGES) + sizeof(PIXEL_T*) * num_images;
    }

    static void SetImageSize(JOB_DESCRIPTOR_T *const job_descriptor_impl, const int image_size) {
        job_descriptor_impl->IMAGE_SIZE = image_size;
    }

    static void SetImageCount(JOB_DESCRIPTOR_T *const job_descriptor_impl, const int image_count) {
        job_descriptor_impl->LDR_IMAGE_COUNT = image_count;
    }

    static void AddInputImageLocation(JOB_DESCRIPTOR_T *const job_descriptor_impl, PIXEL_T * const input_location, const int image_num) {
        job_descriptor_impl->INPUT_IMAGES[image_num] = input_location;
    }

    static void CopyToRawBuffer(BYTE_T *raw_buffer, const JOB_DESCRIPTOR_T *const job_descriptor_impl) {
        ASSERT(raw_buffer != nullptr, "raw_buffer is a nullptr");
        ASSERT(job_descriptor_impl->LDR_IMAGE_COUNT > 0, "Bad job descriptor copy. LDR_IMAGE_COUNT not initialized")
        ASSERT(job_descriptor_impl->IMAGE_SIZE > 0, "Bad job descriptor copy. IMAGE_SIZE not initialized")
        ASSERT(job_descriptor_impl->OUTPUT_IMAGE_LOCATION != nullptr, "Bad job descriptor copy. OUTPUT_IMAGE_LOCATION not initialized")
        for (int image = 0; image < job_descriptor_impl->LDR_IMAGE_COUNT; image++) {
            ASSERT(job_descriptor_impl->INPUT_IMAGES[image] != nullptr, "Bad job descriptor copy. One of the input image addresses was not assigned")
        }

        memcpy(raw_buffer, job_descriptor_impl, BytesNeededForJobDescriptor(job_descriptor_impl->LDR_IMAGE_COUNT));
    }

    static JOB_DESCRIPTOR_T *InterpretRawBufferAsJobDescriptor(BYTE_T *raw_buffer) {
        ASSERT((unsigned long)raw_buffer % sizeof(JOB_DESCRIPTOR_IMPL) == 0, "misaligned raw buffer trying to be interpreted as JobDescriptor");
        ASSERT(raw_buffer != nullptr, "raw_buffer is a nullptr");

        return (JOB_DESCRIPTOR_IMPL*)raw_buffer;
    }

    static const int NumberOfLDRImages(const JOB_DESCRIPTOR_T *const job_descriptor_impl) {
        ASSERT(job_descriptor_impl != nullptr, "job_descriptor_impl == nullptr");
        return job_descriptor_impl->LDR_IMAGE_COUNT;
    }

    static const int ImageSize(const JOB_DESCRIPTOR_T *const job_descriptor_impl) {
        ASSERT(job_descriptor_impl != nullptr, "job_descriptor_impl == nullptr");
        return job_descriptor_impl->IMAGE_SIZE;
    }

    static const PIXEL_T *OutputImageLocation(const JOB_DESCRIPTOR_T *const job_descriptor_impl) {
        ASSERT(job_descriptor_impl != nullptr, "job_descriptor_impl == nullptr");
        return job_descriptor_impl->OUTPUT_IMAGE_LOCATION;
    }

    static const PIXEL_T *InputImageLocation(const JOB_DESCRIPTOR_T *const job_descriptor_impl, const int image_num) {
        ASSERT(job_descriptor_impl != nullptr, "job_descriptor_impl == nullptr");
        ASSERT(image_num < job_descriptor_impl->LDR_IMAGE_COUNT, "Tried to get input image location for image number that was greater than total number of input images");
        return job_descriptor_impl->INPUT_IMAGES[image_num];
    }

    
};

#endif
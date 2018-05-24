#ifndef JOB_PACKAGE_H
#define JOB_PACKAGE_H

#include "job_descriptor.hpp"
#include <iostream>

struct JobPackage
{
    // pragma pack?
    JobDescriptor job_descriptor;
    JOB_ID_T job_ID;

    static JOB_ID_T INITIALIZATION_PACKET_ID() { return 0; }

    static JobDescriptor *GetJobDescriptorFromPackage(BYTE_T *job_package) {
        return JobDescriptor::InterpretRawBufferAsJobDescriptor((BYTE_T*)((char*)job_package + sizeof(JOB_ID_T)));
    }

    static const int GetLDRImageCount(BYTE_T *job_package) {
        JobDescriptor *job_descriptor = GetJobDescriptorFromPackage(job_package);
        return JobDescriptor::NumberOfLDRImages(job_descriptor);
    }

    static const PIXEL_T* GetLDRImageAddress(BYTE_T *job_package, const int image_num) {
        JobDescriptor *job_descriptor = GetJobDescriptorFromPackage(job_package);
        const int num_images = JobDescriptor::NumberOfLDRImages(job_descriptor);
        ASSERT(image_num < num_images, "Image number too high");
        ASSERT((unsigned long)JobDescriptor::InputImageLocation(job_descriptor, image_num) < 
                (num_images * JobDescriptor::ImageSize(job_descriptor) + 
                (unsigned long)job_descriptor + JobDescriptor::BytesNeededForJobDescriptor(num_images)),
                "JobPackage not consolidated in memory");
        return (PIXEL_T*)job_descriptor->INPUT_IMAGES[image_num];
    }

    static void ConsolidateJob(BYTE_T *consolidated_job_buffer, const JobDescriptor *const job_descriptor) {
        const int num_images = JobDescriptor::NumberOfLDRImages(job_descriptor);
        const int bytes_needed_for_job_descriptor = JobDescriptor::BytesNeededForJobDescriptor(num_images);
        memcpy(consolidated_job_buffer, job_descriptor, bytes_needed_for_job_descriptor);
        JobDescriptor *copied_job_descriptor = JobDescriptor::InterpretRawBufferAsJobDescriptor((BYTE_T*)consolidated_job_buffer);

        const int image_size = JobDescriptor::ImageSize(job_descriptor);
        int current_byte_offset = /*(BYTE_T*)copied_job_descriptor + */ JobDescriptor::BytesNeededForJobDescriptor(num_images);

        copied_job_descriptor->OUTPUT_IMAGE_LOCATION = (uintptr_t)((BYTE_T*)copied_job_descriptor + current_byte_offset);
        static_assert(sizeof(unsigned long) >= sizeof(JobDescriptor::OUTPUT_IMAGE_LOCATION), "unsigned long not big enough to store output location pointer");
        //ASSERT(((unsigned long)copied_job_descriptor->OUTPUT_IMAGE_LOCATION % sizeof(PIXEL_T)) == 0, "Misaligned consolidated image data");
        current_byte_offset += image_size * sizeof(PIXEL_T);

        for (int i = 0; i < num_images; i++) {
            copied_job_descriptor->INPUT_IMAGES[i] = (uintptr_t)((BYTE_T*)copied_job_descriptor + current_byte_offset);
            //ASSERT(((unsigned long)copied_job_descriptor->INPUT_IMAGES[i] % alignof(job_descriptor)) == 0, "Misaligned consolidated image data");
            //for (unsigned long byte = 0; byte < image_size * sizeof(PIXEL_T); byte++) {
            //    BYTE_T *dest = (BYTE_T*)copied_job_descriptor->INPUT_IMAGES[i] + byte;
            //    BYTE_T *src = (BYTE_T*)job_descriptor->INPUT_IMAGES[i] + byte;
            //    *dest = *src;
            //}
            ((BYTE_T*)copied_job_descriptor->INPUT_IMAGES[i])[0] = ((BYTE_T*)job_descriptor->INPUT_IMAGES[i])[0];
            ((BYTE_T*)copied_job_descriptor->INPUT_IMAGES[i])[image_size * sizeof(PIXEL_T) - 1] = ((BYTE_T*)job_descriptor->INPUT_IMAGES[i])[image_size * sizeof(PIXEL_T) - 1];
            memcpy((BYTE_T*)(copied_job_descriptor->INPUT_IMAGES[i]), (BYTE_T*)(job_descriptor->INPUT_IMAGES[i]), image_size * sizeof(PIXEL_T));
            current_byte_offset += image_size * sizeof(PIXEL_T);
        }
        
    }
};

#endif

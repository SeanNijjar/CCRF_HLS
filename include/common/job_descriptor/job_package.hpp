#include "job_descriptor.hpp"

struct JobPackage
{
    // pragma pack?
    JOB_ID_T job_ID;
    JobDescriptor job_descriptor;

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
        return job_descriptor->INPUT_IMAGES[image_num];
    }

    static void ConsolidateJob(BYTE_T *consolidated_job_buffer, const JobDescriptor *const job_descriptor) {
        const int num_images = JobDescriptor::NumberOfLDRImages(job_descriptor);
        memcpy(consolidated_job_buffer, job_descriptor, JobDescriptor::BytesNeededForJobDescriptor(num_images));
        JobDescriptor *copied_job_descriptor = JobDescriptor::InterpretRawBufferAsJobDescriptor((BYTE_T*)job_descriptor);

        const int image_size = JobDescriptor::ImageSize(job_descriptor);
        int current_byte_offset = /*(BYTE_T*)copied_job_descriptor + */ JobDescriptor::BytesNeededForJobDescriptor(num_images);

        copied_job_descriptor->OUTPUT_IMAGE_LOCATION = (PIXEL_T*)((BYTE_T*)job_descriptor + current_byte_offset);
        static_assert(sizeof(unsigned long) >= sizeof(JobDescriptor::OUTPUT_IMAGE_LOCATION), "unsigned long not big enough to store output location pointer");
        ASSERT(((unsigned long)copied_job_descriptor->OUTPUT_IMAGE_LOCATION % sizeof(PIXEL_T)) == 0, "Misaligned consolidated image data");
        current_byte_offset += image_size * sizeof(PIXEL_T);

        for (int i = 0; i < num_images; i++) {
            copied_job_descriptor->INPUT_IMAGES[i] = (PIXEL_T*)((BYTE_T*)job_descriptor + current_byte_offset);
            ASSERT(((unsigned long)copied_job_descriptor->INPUT_IMAGES[i] % sizeof(PIXEL_T)) == 0, "Misaligned consolidated image data");
            memcpy(copied_job_descriptor->INPUT_IMAGES[i], job_descriptor->INPUT_IMAGES[i], image_size * sizeof(PIXEL_T));
            current_byte_offset += image_size * sizeof(PIXEL_T);
        }
        
    }
};
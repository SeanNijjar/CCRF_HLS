#include "common/include/job_descriptor/job_descriptor.h"

class JobPackage
{
  private:
    static JOB_DESCRIPTOR_IMPL *GetJobDescriptorFromPackage(BYTE_T *job_package) {
        return InterpretRawBufferAsJobDescriptor(job_package);
    }

  public:
    static const int GetLDRImageCount(BYTE_T *job_package) {
        JOB_DESCRIPTOR_IMPL *job_descriptor = GetJobDescriptorFromPackage(job_package);
        return JobDescriptor::GetLDRImageCount(job_descriptor);
    }

    static const int GetLDRImageAddress(BYTE_T *job_package, const int image_num) {
        JOB_DESCRIPTOR_IMPL *job_descriptor = GetJobDescriptorFromPackage(job_package);
        const int num_images = JobDescriptor::GetLDRImageCount(job_descriptor);
        ASSERT(image_num < num_images, "Image number too high");
        ASSERT(JobDescriptor::GetLDRImageAddress[image_num] < 
                num_images * JobDescriptor::ImageSize(job_descriptor) + 
                (unsigned long)job_descriptor) + JobDescriptor::BytesNeededForJobDescriptor(num_images),
                "JobPackage not consolidated in memory");
        return job_descriptor->INPUT_IMAGES[image_num];
    }

    static void ConsolidateJob(BYTE_T *consolidated_job_buffer, JOB_DESCRIPTOR_IMPL *job_descriptor) {
        const int num_images = JobDescriptor::GetLDRImageCount(job_descriptor);
        memcpy(consolidated_job_buffer, job_descriptor, JobDescriptor::BytesNeededForJobDescriptor(num_images));
        JOB_DESCRIPTOR_IMPL *copied_job_descriptor = JobDescriptor::InterpretRawBufferAsJobDescriptor(job_descriptor);

        const int image_size = JobDescriptor::ImageSize(job_descriptor);
        int current_byte_offset = (BYTE_T*)copied_job_descriptor + 
                                  JobDescriptor::BytesNeededForJobDescriptor(num_images);
        copied_job_descriptor->OUTPUT_IMAGE = (BYTE_T*)job_descriptor + current_byte_offset;
        current_byte_offset += image_size * sizeof(PIXEL_T);
        for (int i = 0; i < num_images; i++) {
            copied_job_descriptor->INPUT_IMAGES[i] = (BYTE_T*)job_descriptor + current_byte_offset;
            memcpy(copied_job_descriptor->INPUT_IMAGES[i], job_descriptor->INPUT_IMAGES[i], image_size * sizeof(PIXEL_T));
            current_byte_offset += image_size * sizeof(PIXEL_T);
        }
        
    }
};
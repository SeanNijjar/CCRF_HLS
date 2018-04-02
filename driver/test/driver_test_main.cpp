#include "common/include/utils.h"
#include "common/include/helper.h"
#include "driver/include/driver.h"

#include <string>

int main(int argc, char *argv[]) 
{
    ASSERT(argc > 1, "Expect at least one path to a set of file lists to test");

    Job

    // Read in the test image sets
    std::vector<IMAGE_STACK_T> image_stacks;
    for (int i = 1; i < argc; i++) {
        std::string file_list_path(argv[i]);
        auto file_contents = ReadFileContents(file_list_path);
        for (auto file : file_contents) {
            image_stack.push_back(ReadImageStackImages(file));
        }
    }

    // Create job descriptions for each LDR image set to HDR image
    // Note though that these jobs are not consolidated (meaning all)
    // of the input and output images are not necessarily in contiguous memory
    std::vector<JOB_DESCRIPTOR_IMPL*> job_descriptors;
    for(auto image_stack : image_stacks) {
        // TODO: Change the Create routine to be CreateConsolidated
        //       which will only emit job descriptors with job data
        //       that exists contiguously in memory
        job_descriptors.push_back(JobDescriptor::Create(image_stack, /*TODO: output_locations*/));
    }

    // Now we have to consolidate our job data so it is in a contiguous buffer
    // This is necessary to DMA the job to the FPGA/remote processor
    std::vector<BYTE_T*> consolidated_job_buffers;
    for (auto job_descriptor : job_descriptors) {
        BYTE_T *consolidated_job_buffer = new BYTE_T[JobDescriptor::BytesNeededForEntireJob(job_descriptor)];
        JobPackage::ConsolidateJob(consolidated_job_buffer, job_descriptor);
        consolidated_job_buffers.push_back(consolidated_job_buffer);
    }

    JobDispatcher job_dispatcher(JobDispatcher::DISPATCH_MODE_EXCLUSIVE);
    std::vector<JOB_ID_T> job_IDs;
    for (auto consolidated_job_buffer : consolidated_job_buffers) {
        JOB_ID_T job_ID = job_dispatcher.DispatchJob(JobDescriptor::InterpretRawBufferAsJobDescriptor(consolidated_job_buffer));
        job_IDs.push_back(job_ID);
    }
    
    // TODO: Figure out how to get back the outputs/results. We should be able to dump the results 
    //       into the output buffers. Changing it to have the user specify the output buffer above
    //       Internally, there needs to be a mapping between user output buffer and the memory mapped
    //       output buffer

    job_dispatcher.WaitForJobsToFinish();
    // Check Result
    for (auto result : results) {
        const int image_size = JobDescriptor::ImageSize(JobDescriptor::InterpretRawBufferAsJobDescriptor(result));
        auto input0 = JobDescriptor::InputImageLocation(JobDescriptor::InterpretRawBufferAsJobDescriptor(result), 0);
        auto output = JobDescriptor::OutputImageLocation(JobDescriptor::InterpretRawBufferAsJobDescriptor(result));
        for(int i = 0; i < image_size; i++) {
            if (input0[i] != output[i]) {
                std::cout << "ERROR in output at index " << i << " expected " << input0[i] << 
                             " from input0 but got " << output[i] << "." << std::endl;
                EXIT_FAILURE();
            }
        }
    }

    std::cout << "Test Successful" << std::endl;
    EXIT_SUCCESS();
}
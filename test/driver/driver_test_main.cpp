#include "utils.hpp"
#include "helper.hpp"
#include "job_dispatcher.hpp"
#include "job_descriptor.hpp"
#include "job_package.hpp"

#include <string>
#include <vector>
#include <unistd.h>
#include <memory>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>


int main(int argc, char *argv[]) 
{
    ASSERT(argc > 2, "Expect at least 2 arguments: 1) path to the folder of the file list 2) the file containing the list of files to test");
    std::string root_dir = argv[1];
    std::string file_with_file_lists_path = argv[2];

    const int scratchpad_pixel_count = 100000000;
    CCRF_SCRATCHPAD_START_ADDR = (uintptr_t)new PIXEL_T[scratchpad_pixel_count];
    CCRF_SCRATCHPAD_END_ADDR = (uintptr_t)CCRF_SCRATCHPAD_START_ADDR + scratchpad_pixel_count;

    // Read in the test image sets
    const char *starting_pwd = get_current_dir_name();
    int rc = chdir(root_dir.c_str());
    assert(rc == 0);
    std::vector<IMAGE_STACK_T> image_stacks;
    std::string file_list_path(argv[1]);
    file_list_path.append(argv[2]);
    auto file_contents = ReadFileContents(file_list_path);
    for (auto file : file_contents) {
        std::string image_dir = root_dir + file;
        rc = chdir(image_dir.c_str());
        assert(rc == 0);
        image_stacks.push_back(ReadImageStackImages("file_list"));
        rc = chdir(root_dir.c_str());
        assert(rc == 0);
    }
    
    chdir(starting_pwd);
    delete[] starting_pwd;
    starting_pwd = nullptr;

    // Create job descriptions for each LDR image set to HDR image
    // Note though that these jobs are not consolidated (meaning all)
    // of the input and output images are not necessarily in contiguous memory
    std::vector<JobDescriptor*> job_descriptors;
    for (auto img_stack_iter = image_stacks.begin(); img_stack_iter != image_stacks.end(); img_stack_iter++) {
        JobDescriptor *new_job_descriptor = JobDescriptor::Create(*img_stack_iter);

        job_descriptors.push_back(new_job_descriptor);
    }

    // Now we have to consolidate our job data so it is in a contiguous buffer
    // This is necessary to DMA the job to the FPGA/remote processor
    std::vector<BYTE_T*> consolidated_job_buffers;
    for (auto job_desc_iter = job_descriptors.begin(); job_desc_iter != job_descriptors.end(); job_desc_iter++) {
        //job_descriptor : job_descriptors) 
        for (int i = 0; i < (*job_desc_iter)->LDR_IMAGE_COUNT; i++) {
            std::string input_string = "y";
            input_string.append(std::to_string(i));
            strcpy((char*)(*job_desc_iter)->INPUT_IMAGES[i], input_string.c_str());
        }
        unsigned long bytes_needed_for_entire_job = JobDescriptor::BytesNeededForEntireJob(*job_desc_iter);
        BYTE_T *consolidated_job_buffer = (BYTE_T*)new BYTE_T*[bytes_needed_for_entire_job + 32];
        JobPackage::ConsolidateJob(consolidated_job_buffer, *job_desc_iter);
        consolidated_job_buffers.push_back(consolidated_job_buffer);
    }

    JobDispatcher job_dispatcher(JobDispatcher::DISPATCH_MODE_EXCLUSIVE_BLOCKING);
    job_dispatcher.StartDispatcher();
    std::vector<JOB_ID_T> job_IDs;
    for (auto consolidated_job_buffer : consolidated_job_buffers) {
        //job_dispatcher.DispatchJobAsync(JobDescriptor::InterpretRawBufferAsJobDescriptor(consolidated_job_buffer));
        job_dispatcher.DispatchJob(JobDescriptor::InterpretRawBufferAsJobDescriptor(consolidated_job_buffer));
    }

    job_dispatcher.SynchronizeWait();

    job_dispatcher.PrintJobResultStats();

    int i = 0;
    for (auto consolidated_job_buffer : consolidated_job_buffers) {
        JobDescriptor *processed_image_job_descriptor = JobDescriptor::InterpretRawBufferAsJobDescriptor(consolidated_job_buffer);
        IMAGE_T image_to_write_to_file((PIXEL_T*)processed_image_job_descriptor->OUTPUT_IMAGE_LOCATION, processed_image_job_descriptor->IMAGE_WIDTH, processed_image_job_descriptor->IMAGE_HEIGHT);
        WriteImageToFile(image_to_write_to_file, std::string("HDR_OUTPUT_").append(std::to_string(i)).append(".png"));
        i++;
    }

    //job_dispatcher.StopDispatcher();
    std::cout << "Test Done" << std::endl;
    exit(0);
}
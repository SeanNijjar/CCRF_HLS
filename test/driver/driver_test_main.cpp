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


    // Read in the test image sets
    std::cout << "Reading in test image sets" << std::endl;
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
    
    std::cout << "Creating job dispatcher" << std::endl; 
    JobDispatcher job_dispatcher(JobDispatcher::DISPATCH_MODE_EXCLUSIVE_BLOCKING);
    std::cout << "Starting job dispatcher" << std::endl;
    job_dispatcher.StartDispatcher();

    // Create job descriptions for each LDR image set to HDR image
    // Note though that these jobs are not consolidated (meaning all)
    // of the input and output images are not necessarily in contiguous memory
    std::cout << "Creating job descriptors" << std::endl;
    std::vector<JobDescriptor*> job_descriptors;
    for (auto img_stack_iter = image_stacks.begin(); img_stack_iter != image_stacks.end(); img_stack_iter++) {
        JobDescriptor *new_job_descriptor = JobDescriptor::Create(*img_stack_iter);

        size_t image_size = new_job_descriptor->IMAGE_SIZE() * sizeof(PIXEL4_T);
        int num_images = new_job_descriptor->LDR_IMAGE_COUNT;
        for (int i = 0; i < num_images; i++) {//auto image : image_stack) {
            PIXEL4_T *image = (PIXEL4_T*)job_dispatcher.AxidmaMalloc(image_size);
            memcpy(image, (PIXEL4_T*)new_job_descriptor->INPUT_IMAGES[i], image_size);
            delete (BYTE_T*)new_job_descriptor->INPUT_IMAGES[i];
            new_job_descriptor->INPUT_IMAGES[i] = (uintptr_t)image;
            std::cout << "Input image " << i << " address = " << image << std::endl;
        }

        new_job_descriptor->OUTPUT_IMAGE_LOCATION = (uintptr_t)malloc(image_size);//(uintptr_t)job_dispatcher.AxidmaMalloc(image_size);
        job_descriptors.push_back(new_job_descriptor);
    }

    // Now we have to consolidate our job data so it is in a contiguous buffer
    // This is necessary to DMA the job to the FPGA/remote processor
    std::cout << "Consolidating jobs" << std::endl;
    std::vector<BYTE_T*> consolidated_job_buffers;
    for (auto job_desc_iter = job_descriptors.begin(); job_desc_iter != job_descriptors.end(); job_desc_iter++) {
        size_t bytes_needed_for_descriptor = JobDescriptor::BytesNeededForJobDescriptor((*job_desc_iter)->LDR_IMAGE_COUNT);
        #ifdef CSIM
        BYTE_T *consolidated_job_buffer = (BYTE_T*)new BYTE_T[bytes_needed_for_entire_job + 32];//(BYTE_T*)new BYTE_T*[bytes_needed_for_entire_job + 32];
        #else
        BYTE_T *consolidated_job_buffer = (BYTE_T*)job_dispatcher.AxidmaMalloc(bytes_needed_for_descriptor);//(BYTE_T*)new BYTE_T*[bytes_needed_for_entire_job + 32];
        #endif
        memcpy(consolidated_job_buffer, (*job_desc_iter), bytes_needed_for_descriptor);
        consolidated_job_buffers.push_back(consolidated_job_buffer);
    }
    
    std::cout << "Dispatching jobs" << std::endl;

    std::vector<JOB_ID_T> job_IDs;
    int i = 0;
    for (auto consolidated_job_buffer : consolidated_job_buffers) {
        std::cout << "Dispatching job..." << std::endl;
        job_dispatcher.DispatchJob(JobDescriptor::InterpretRawBufferAsJobDescriptor(consolidated_job_buffer));

        //job_dispatcher.SynchronizeWait();
 
        job_dispatcher.PrintJobResultStats();


        JobDescriptor *processed_image_job_descriptor = JobDescriptor::InterpretRawBufferAsJobDescriptor(consolidated_job_buffer);
        IMAGE_T image_to_write_to_file((PIXEL_T*)processed_image_job_descriptor->OUTPUT_IMAGE_LOCATION, processed_image_job_descriptor->IMAGE_WIDTH, processed_image_job_descriptor->IMAGE_HEIGHT);
        std::cout << "Writing image to file" << std::endl;
        WriteImageToFile(image_to_write_to_file, std::string("HDR_OUTPUT_").append(std::to_string(i)).append(".jpg"));
        #ifdef CSIM
        delete consolidated_job_buffer;
        #else
        std::cout << "AxidmaMalloc" << std::endl;
        job_dispatcher.AxidmaFree(consolidated_job_buffer, JobDescriptor::BytesNeededForJobDescriptor((JobDescriptor*)consolidated_job_buffer));
        #endif
        i++;
        std::cout << "Done Image" << std::endl;
    }

/*    job_dispatcher.SynchronizeWait();

    job_dispatcher.PrintJobResultStats();

    i = 0;
    for (auto consolidated_job_buffer : consolidated_job_buffers) {
        JobDescriptor *processed_image_job_descriptor = JobDescriptor::InterpretRawBufferAsJobDescriptor(consolidated_job_buffer);
        IMAGE_T image_to_write_to_file((PIXEL_T*)processed_image_job_descriptor->OUTPUT_IMAGE_LOCATION, processed_image_job_descriptor->IMAGE_WIDTH, processed_image_job_descriptor->IMAGE_HEIGHT);
        WriteImageToFile(image_to_write_to_file, std::string("HDR_OUTPUT_").append(std::to_string(i)).append(".png"));
        #ifdef CSIM
        delete consolidated_job_buffer;
        #else
        job_dispatcher.AxidmaFree(consolidated_job_buffer, JobDescriptor::BytesNeededForEntireJob((JobDescriptor*)consolidated_job_buffer) + 32);
        #endif
        i++;
    }

*/

    //job_dispatcher.StopDispatcher();
    std::cout << "Test Done" << std::endl;
    exit(0);
}

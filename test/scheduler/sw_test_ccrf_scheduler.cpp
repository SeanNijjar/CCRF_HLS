#include "ccrf_scheduler.hpp"
#include "job_descriptor/job_descriptor.hpp"
#include "helper.hpp"
#include <hls_stream.h>
#include <vector>
#include <thread>
#include <chrono>
//#include "common/include/utils.hpp"

//#include <cstdlib.h>
template <typename T>
struct GraphNode
{
    T data;
    GraphNode *left;
    GraphNode *right;

    GraphNode(T _data) : left(nullptr), right(nullptr), data(_data)
    {}
};

int expected_subtask_count (int ldr_img_count)
{
    if (ldr_img_count == 1) {
        return 0;
    } else {
        return expected_subtask_count(ldr_img_count-1) + ldr_img_count-1;
    }
};

int main(int argc, char *argv[]) {
    int test_result = 0;
    
    // An argument, if present, tells us the LDR image count for the job descriptor
    // Otherwise, or target use case is 5 LDR images/HDR image
    const int ldr_img_count = (argc == 2) ? atoi(argv[1]) : 5;

    hls::stream<JobPackage> input_job_descriptor_queue;
    hls::stream<JOB_COMPLETION_PACKET> jobs_in_progress_queue;
    hls::stream<JOB_SUBTASK> output_subtask_queue;

    // Single Job Descriptor
    IMAGE_STACK_T ldr_images;

    PIXEL_T *image_addr = (PIXEL_T*)1000;
    const int image_w = 1024;
    const int image_h = 1024;
    for (int i = 0; i < ldr_img_count; i++) {
        ldr_images.push_back({image_addr, image_w, image_h});
        image_addr += (image_w * image_h);
    }

    // this might bite me in the butt
    JobDescriptor *job_descr = JobDescriptor::Create(ldr_images);
    job_descr->OUTPUT_IMAGE_LOCATION = 54000;
    JobPackage jobPackage;
    jobPackage.job_descriptor = *job_descr;
    jobPackage.job_ID = 1;
    input_job_descriptor_queue.write(jobPackage); // This will free the pointer? (maybe it does move under the hood)
    //delete job_descr;
    bool threaded = false;
    const uintptr_t CCRF_HARDWARE_SCRATCHPAD_START = 1000000;
    const uintptr_t CCRF_HARDWARE_SCRATCHPAD_END = 200000000;
    bool ccrf_subtask_scheduler_got_data;
    if (threaded) {
        //std::thread ccrf_scheduler_thread(CcrfScheduler<true>, input_job_descriptor_queue, output_subtask_queue, jobs_in_progress_queue);
        //ccrf_scheduler_thread.detach();
        //std::this_thread::sleep_for (std::chrono::seconds(1));
    } else {
    	for (int i = 0; i < 1000; i++) {
    	CcrfSubtaskScheduler(input_job_descriptor_queue,
    			output_subtask_queue,
				jobs_in_progress_queue,
				CCRF_HARDWARE_SCRATCHPAD_START,
    			CCRF_HARDWARE_SCRATCHPAD_END,
    			ccrf_subtask_scheduler_got_data);
    	}

        //CcrfSubtaskScheduler<false>(input_job_descriptor_queue, output_subtask_queue, jobs_in_progress_queue);
    }

    
    ASSERT(input_job_descriptor_queue.empty(), "input queue not emptied properly");
    ASSERT(!jobs_in_progress_queue.empty(), "jobs_in_progress not populated properly");
    ASSERT(!output_subtask_queue.empty(), "output_subtask_queue not populated properly");
    
	while (!jobs_in_progress_queue.empty()) {
		jobs_in_progress_queue.read();
	}

    std::vector<JOB_SUBTASK> subtasks;
    while (!output_subtask_queue.empty()) {
        JOB_SUBTASK subtask = output_subtask_queue.read();
        subtasks.push_back(subtask);
    }

    const int expected_num_subtasks = expected_subtask_count(job_descr->LDR_IMAGE_COUNT);
    std::cout << "Expected num subtasks = " << expected_num_subtasks << std::endl;
    const int actual_subtask_count = subtasks.size();
    ASSERT(expected_num_subtasks == actual_subtask_count, "wrong number of subtasks generated")

    std::vector<JOB_SUBTASK> parent_list;
    std::vector<JOB_SUBTASK> children_list;
    parent_list.push_back(subtasks.back());
    subtasks.pop_back();

    const int parent_size = parent_list.size();
    int layer = 1;
    while (subtasks.size()) {
        for (children_list.clear(); children_list.size() < parent_list.size() + 1; subtasks.pop_back()) {
            children_list.push_back(subtasks.back());
        }

        for (int parent = 0; parent < parent_list.size(); parent++) {
            ASSERT(parent_list[parent].input2 == children_list[parent].output, "subtask graph problem. Layer=" << layer << " parent=" << parent << "p[parent].input2 != c[parent].output");
            ASSERT(parent_list[parent].input1 == children_list[parent+1].output, "subtask graph problem. Layer=" << layer << " parent=" << parent << "p[parent].input2 != c[parent+1].output");
        }

        std::swap(parent_list, children_list);
        layer++;
    }
    

    return test_result;
}

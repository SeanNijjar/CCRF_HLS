#include "driver.hpp"
#include "job_package.hpp"
#ifdef ZYNQ_COMPILE
#include "libaxidma.hpp"
#endif
#include <vector>
#include <sys/stat.h>           // Open() system call
#include <sys/types.h> 
#include <unistd.h>
#include <fcntl.h>

uintptr_t CCRF_SCRATCHPAD_START_ADDR = (uintptr_t)0x0;
uintptr_t CCRF_SCRATCHPAD_END_ADDR = (uintptr_t)0x0;

#ifdef ZYNQ_COMPILE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

//#include "util.h"               // Miscellaneous utilities
//#include "conversion.h"         // Convert bytes to MiBs
#include "axidma_ctrl_func.hpp"

bool ZynqHardwareDriver::PlDmaWrite(void *const axidma_transfer_buf, const uintptr_t ps_addr, const uintptr_t pl_addr, const int transfer_size)
{
	std::cout << "PlDmaWrite" << std::endl;
	memcpy(axidma_transfer_buf, (char *const)ps_addr,transfer_size);
        int tx_channel = axidma_get_dma_tx(axidma_dev)->data[0];
        int size;
        int left_bytes = transfer_size;
        char *input_buf_dummy = (char*)axidma_transfer_buf;
	uintptr_t pl_addr_tmp = (uintptr_t)pl_addr;
        do {
                size = left_bytes > BUFFER_SIZE ? BUFFER_SIZE : left_bytes;
		std::cout << "left_bytes=" << left_bytes << " transfer_size=" << size << std::endl;
		assert ((uintptr_t)pl_addr_tmp % 4096 == 0);
		assert ((uintptr_t)input_buf_dummy % 4096 == 0);
                DMA_S2MM(dma_vptr, ADDR_LOWER(pl_addr_tmp), ADDR_UPPER(pl_addr_tmp), size);
                axidma_oneway_transfer(axidma_dev, tx_channel, input_buf_dummy, size, true);
                input_buf_dummy += size;
                pl_addr_tmp += size;
                left_bytes -= size;
        } while (left_bytes != 0);
	std::cout << "/PlDmaWrite" << std::endl;
	return true;
}

/*insert all the lines in main function to you code after where you got the final image addr and size. In the future
I may need your help to insert more code but right now it just like this*/
bool ZynqHardwareDriver::PL_to_PS_DMA(void *const axidma_transfer_buf, char *const ps_addr, void * const pl_addr, size_t transfer_size)
{	
	std::cout << "PL_to_PS_DMA" << std::endl;
        int rx_channel = axidma_get_dma_rx(axidma_dev)->data[0];
        int size;
        int left_bytes = transfer_size;
        char * output_buf_dummy = (char*)axidma_transfer_buf;
	uintptr_t pl_addr_tmp = (uintptr_t)pl_addr;
        do {
		assert ((uintptr_t)pl_addr_tmp % 4096 == 0);
		assert ((uintptr_t)output_buf_dummy % 4096 == 0);
                size = left_bytes > BUFFER_SIZE ? BUFFER_SIZE : left_bytes;
		std::cout << "left_bytes=" << left_bytes << " transfer_size=" << size << std::endl;
                DMA_MM2S(dma_vptr, ADDR_LOWER(pl_addr_tmp), ADDR_UPPER(pl_addr_tmp), size);
                axidma_oneway_transfer(axidma_dev, rx_channel, output_buf_dummy, size, true);
                output_buf_dummy += size;
                pl_addr_tmp += size;
                left_bytes -= size;
        } while (left_bytes != 0);
	memcpy(ps_addr, axidma_transfer_buf, transfer_size);
	std::cout << "/PL_to_PS_DMA" << std::endl;
	return true;
}

bool ZynqHardwareDriver::AxidmaSendStreamData(void *axidma_buffer_data, void *user_buffer, size_t transfer_size)
{
    memcpy(axidma_buffer_data, user_buffer, transfer_size);
    int rc = axidma_oneway_transfer(axidma_dev, tx_chans->data[1], 
                                    axidma_buffer_data, 
                                    transfer_size, 
                                    false);
    ASSERT(rc == 0, "return code is non-zero: " << rc);
    return (rc == 0) ? true : false;    
}

bool ZynqHardwareDriver::SendJobRequest(JobPackage &job)
{
    #ifdef LOOPBACK_TEST
    int loopback_data = job.job_ID;
    std::cout << "write job_request queue:" << loopback_data << std::endl;
    void *job_buffer = &loopback_data;
    #else
    void *job_buffer = &job;
    #endif

    bool successful = AxidmaSendStreamData(job_package_axidma_buffer, job_buffer, job_package_axidma_buffer_size);
    std::cout << "Done one-way transfer" << std::endl;
    return successful;
}

const uint64_t ZynqHardwareDriver::GetDMAFileSize(std::string dma_file_path)
{
    uint64_t file_size;
    auto dma_file_descriptor = open(dma_file_path.c_str(), O_RDONLY);

    if (dma_file_descriptor < 0) {
        file_size = 0;
        goto return_stmt;
    }
 
    struct stat file_stat;
    if (fstat(dma_file_descriptor, &file_stat) < 0) {
        std::cerr << "Unable to get file statistics" << std::endl;
        file_size = 0;
        goto return_stmt;
    }

    file_size = file_stat.st_size;
    return_stmt:
    close(dma_file_descriptor);
    return file_size;
}

bool ZynqHardwareDriver::ResponseQueueHasData(const int minimum_bytes)
{
    return (minimum_bytes <= GetDMAFileSize(output_path));
}

/*----------------------------------------------------------------------------
 * DMA File Transfer Functions
 *----------------------------------------------------------------------------*/

ZynqHardwareDriver::ZynqHardwareDriver(
        std::vector<JobPackage> &incoming_queue, 
        std::vector<JOB_STATUS_MESSAGE> &outgoing_queue
    ) :
    Driver(incoming_queue, outgoing_queue),
    pl_ddr_start_addr(0),
    pl_ddr_last_addr(1l << 32),
    scratchpad_size_in_bytes(100000000*sizeof(PIXEL4_T)),
    pl_ddr_current_malloc_addr(0),
    #ifdef LOOPBACK_TEST
    job_package_axidma_buffer_size(4),
    job_status_axidma_buffer_size(4)
    #else
    job_package_axidma_buffer_size(sizeof(JobPackage)),
    job_status_axidma_buffer_size(sizeof(JOB_STATUS_MESSAGE))
    #endif
{
    output_path = "/dev/axidma";
    input_path = "/dev/axidma";
    // Taken from main() in axidma_transfer.cpp
    int rc;
    struct stat input_stat;

    // Initialize the AXIDMA device
    axidma_dev = axidma_init("/dev/axidma", "/dev/mem");
    if (axidma_dev == NULL) {
        fprintf(stderr, "Error: Failed to initialize the AXI DMA device.\n");
        rc = 1;
        goto destroy_axidma;
    }
    dma_vptr = (char *const) mmap(NULL, DMA_RegSize, PROT_READ|PROT_WRITE, MAP_SHARED, axidma_dev->fd_mem, DMA_BASE);

    // Get the tx and rx channels if they're not already specified
    tx_chans = axidma_get_dma_tx(axidma_dev);
    if (tx_chans->len < 1) {
        fprintf(stderr, "Error: No transmit channels were found.\n");
        rc = -ENODEV;
        goto destroy_axidma;
    }
    rx_chans = axidma_get_dma_rx(axidma_dev);
    if (rx_chans->len < 1) {
        fprintf(stderr, "Error: No receive channels were found.\n");
        rc = -ENODEV;
        goto destroy_axidma;
    }

    std::cout << "AXI DMA File Transfer Info: " << std::endl;
    std::cout << "\tTransmit Channel (axidma): " << tx_chans->data[0] << std::endl;
    std::cout << "\tReceive Channel (axidma): " << rx_chans->data[0] << std::endl;

    // Initialize transfer buffers - must come before scratchpad initialize
    job_package_axidma_buffer = (JobPackage*)AxidmaMalloc(sizeof(JobPackage));
    job_status_axidma_buffer = (JOB_STATUS_MESSAGE*)AxidmaMalloc(sizeof(JOB_STATUS_MESSAGE));

    ASSERT(job_package_axidma_buffer, "job_package_axidma_buffer not allocated properly");
    ASSERT(job_status_axidma_buffer, "job_status_axidma_buffer not allocated properly");

    FlushHardware();

    InitializeHardwareScratchpadMemory();

    goto end;

destroy_axidma:
    axidma_destroy(axidma_dev);
end: {}
}


ZynqHardwareDriver::~ZynqHardwareDriver()
{
    AxidmaFree((void*)job_package_axidma_buffer, job_package_axidma_buffer_size);
    AxidmaFree((void*)job_status_axidma_buffer, job_status_axidma_buffer_size);
    AxidmaFree((void*)scratchpad_start_addr, scratchpad_size_in_bytes);
    axidma_destroy(axidma_dev);
}

void ZynqHardwareDriver::InitializeHardwareScratchpadMemory()
{
    int scratchpad_pixel_count = scratchpad_size_in_bytes / sizeof(PIXEL4_T);
    scratchpad_start_addr = (uintptr_t)DeviceMalloc(scratchpad_size_in_bytes);
    std::cout << "Scratchpad start address = " << (PIXEL_T*)scratchpad_start_addr << " with size=" << scratchpad_size_in_bytes << std::endl;
    uintptr_t scratchpad_end_addr = (uintptr_t)((char*)scratchpad_start_addr + scratchpad_size_in_bytes);
    JobPackage initialization_message;
    initialization_message.job_ID = JobPackage::INITIALIZATION_PACKET_ID(); //
    initialization_message.job_descriptor.INPUT_IMAGES[0] = scratchpad_start_addr;
    initialization_message.job_descriptor.INPUT_IMAGES[1] = scratchpad_end_addr;
    SendJobRequest(initialization_message);
}

void ZynqHardwareDriver::FlushHardware()
{
    #ifdef LOOPBACK_TEST
    JOB_STATUS_MESSAGE response_message;
    int rc;
    std::cout << "Flush Hardware" << std::endl;
    do {
        int response_message_int;
        rc = ReadResponseQueuePacket((uint8_t*)&response_message_int, sizeof(JOB_STATUS_MESSAGE));
    } while (rc == 0);
    #endif
}

bool ZynqHardwareDriver::ReadResponseQueuePacket(uint8_t *response_message_buffer, uint64_t bytes_to_read)
{
    int rc = axidma_oneway_transfer(axidma_dev, 
                                    rx_chans->data[1], 
                                    job_status_axidma_buffer, 
                                    job_status_axidma_buffer_size, 
                                    true);
    if (rc == 0) {
        memcpy(response_message_buffer, job_status_axidma_buffer, job_status_axidma_buffer_size);
        #ifdef LOOPBACK_TEST
        std::cout << "Reading response packet: " << *(int*)response_message_buffer << std::endl;
        #endif
    }

    return (rc == 0) ? true : false;
}

bool ZynqHardwareDriver::IntervalsOverlapAddress(uintptr_t interval_start, uintptr_t interval_end, uintptr_t address_to_check)
{
    ASSERT (interval_start < interval_end, "Invalid interval");
    return interval_start <= address_to_check && address_to_check <= interval_end;
}

void *ZynqHardwareDriver::AxidmaMalloc(size_t size_in_bytes) 
{ 
    return axidma_malloc(axidma_dev, size_in_bytes);
}

void *ZynqHardwareDriver::DeviceMalloc(size_t size_in_bytes)
{
    const int padding = 4096;
   
    std::cout << "pl_ddr_current_malloc_addr: " << pl_ddr_current_malloc_addr << std::endl; 
    uintptr_t alloced_region_start = pl_ddr_current_malloc_addr;
    uintptr_t alloced_region_end = alloced_region_start + size_in_bytes + padding;

    if (scratchpad_start_addr - alloced_region_end <= size_in_bytes + padding) {
	std::cout << "DEVICE_MALLOC: skipping scratchpad region" << std::endl;
        alloced_region_start = scratchpad_start_addr + scratchpad_size_in_bytes;
        alloced_region_end = alloced_region_start + size_in_bytes + padding;
    }

    if (pl_ddr_last_addr - alloced_region_start <= size_in_bytes + padding) {
        alloced_region_start = 0;
        alloced_region_end = alloced_region_start + size_in_bytes + padding;
	std::cout << "DEVICE_MALLOC: PL DDR address wrap-around" << std::endl;
    }

    pl_ddr_current_malloc_addr = alloced_region_end + padding - (alloced_region_end % padding); // Extra padding in case of wide reads
    if (pl_ddr_current_malloc_addr >= pl_ddr_last_addr) {
        pl_ddr_current_malloc_addr = 0;
    }

    std::cout << "DEVICE_MALLOC: " << alloced_region_start << std::endl;
    return (void*)alloced_region_start;
}


void ZynqHardwareDriver::AxidmaFree(void *buffer, size_t buffer_size) 
{ 
    axidma_free(axidma_dev, buffer, buffer_size);// do nothing 
}


#endif

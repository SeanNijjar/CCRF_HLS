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
bool ZynqHardwareDriver::SendJobRequest(JobPackage &job)
{
    #ifdef LOOPBACK_TEST
    int loopback_data = job.job_ID;
    std::cout << "write job_request queue:" << loopback_data << std::endl;
    void *job_buffer = &loopback_data;
    #else
    void *job_buffer = &job;
    #endif

    memcpy(job_package_axidma_buffer, job_buffer, job_package_axidma_buffer_size);
    int rc = axidma_oneway_transfer(axidma_dev, tx_chans->data[0], 
                                    job_package_axidma_buffer, 
                                    job_package_axidma_buffer_size, 
                                    false);
    std::cout << "Done one-way transfer" << std::endl;
    ASSERT(rc == 0, "return code is non-zero: " << rc);
    return (rc == 0) ? true : false;
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
    scratchpad_size_in_bytes(1000000*sizeof(PIXEL4_T)),
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
    axidma_dev = axidma_init();
    if (axidma_dev == NULL) {
        fprintf(stderr, "Error: Failed to initialize the AXI DMA device.\n");
        rc = 1;
        goto destroy_axidma;
    }

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
    std::cout << "\tTransmit Channel: " << tx_chans->data[0] << std::endl;
    std::cout << "\tReceive Channel: " << rx_chans->data[0] << std::endl;

    // Initialize transfer buffers - must come before scratchpad initialize
    job_package_axidma_buffer = (JobPackage*)AxidmaMalloc(sizeof(JobPackage));
    job_status_axidma_buffer = (JOB_STATUS_MESSAGE*)AxidmaMalloc(sizeof(JOB_STATUS_MESSAGE));

    ASSERT(job_package_axidma_buffer, "job_package_axidma_buffer not allocated properly");
    ASSERT(job_status_axidma_buffer, "job_status_axidma_buffer not allocated properly");

    FlushHardware();

    InitializeHardwareScratchpadMemory(1000000);

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

void ZynqHardwareDriver::InitializeHardwareScratchpadMemory(size_t scratchpad_size)
{
    int scratchpad_pixel_count = scratchpad_size / sizeof(PIXEL4_T);
    scratchpad_start_addr = (uintptr_t)AxidmaMalloc(scratchpad_size_in_bytes);
    std::cout << "Scratchpad start address = " << (PIXEL_T*)scratchpad_start_addr << std::endl;
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
                                    rx_chans->data[0], 
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

int ZynqHardwareDriver::TransferFile (
    dma_transfer trans
) {
    int rc;
    
    // Allocate a buffer for the input file, and read it into the buffer
    if (trans.input_size) {
        trans.input_buf = AxidmaMalloc(trans.input_size);
        if (trans.input_buf == NULL) {
            std::cerr << "Failed to allocate the input buffer" << std::endl;
            rc = -ENOMEM;
        }
    }

    if (trans.output_size) {
        // Allocate a buffer for the output file
        trans.output_buf = AxidmaMalloc(trans.output_size);
        if (trans.output_buf == NULL) {
            std::cerr << "Failed to allocate the output buffer" << std::endl;
            rc = -ENOMEM;
        }
    }

    bool free_input_buf = false;
    bool free_output_buf = false;
    // Perform the transfer
    if (rc == 0) {
        if (trans.output_size && trans.input_size) {
            rc = axidma_twoway_transfer(axidma_dev, trans.input_channel, trans.input_buf,
                    trans.input_size, trans.output_channel, trans.output_buf,
                    trans.output_size, true);
            if (rc < 0) {
                std::cerr << "DMA read write transaction failed" << std::endl;
                free_output_buf = true;
                free_input_buf = true;
            }
        } else if (trans.output_size) {
            rc = RobustWrite(trans.output_fd, (uint8_t*)trans.output_buf, trans.output_size);
            if (rc < 0) {
                std::cerr << "Unable to read in output buffer" << std::endl;
                free_output_buf = true;
            }
        } else if (trans.input_size) {
            rc = RobustRead(trans.input_fd, (uint8_t*)trans.input_buf, trans.input_size);
            if (rc < 0) {
                std::cerr << "Unable to read in input buffer" << std::endl;
                free_input_buf = true;
            }
        }
        if (free_input_buf) {
            AxidmaFree(trans.input_buf, trans.input_size);
        }
        if (free_output_buf) {
            AxidmaFree(trans.output_buf, trans.output_size);
        }
    }

    return rc;
}


/**
 * The contents below are from an AXI DMA driver sample from CMU. They were obtained
 * from the file util.c in the sample
 **/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include <fcntl.h>              // Flags for open()
#include <sys/stat.h>           // Open() system call
#include <sys/types.h>          // Types for open()
#include <unistd.h>             // Read() and write()
#include <errno.h>              // Error codes


// Parses the arg string as an integer for the given option
int ZynqHardwareDriver::ParseInt(char option, uint8_t *arg_str, int *data)
{
    int rc;

    rc = sscanf(optarg, "%d", data);
    if (rc < 0) {
        perror("Unable to parse argument");
        return rc;
    } else if (rc != 1) {
        fprintf(stderr, "Error: Unable to parse argument '-%c %s' as an "
                "integer.\n", option, arg_str);
        return -EINVAL;
    }

    return 0;
}

// Parses the arg string as a double for the given option
int ZynqHardwareDriver::ParseDouble(char option, uint8_t *arg_str, double *data)
{
    int rc;

    rc = sscanf(optarg, "%lf", data);
    if (rc < 0) {
        perror("Unable to parse argument");
        return rc;
    } else if (rc != 1) {
        fprintf(stderr, "Error: Unable to parse argument '-%c %s' as an "
                "integer.\n", option, arg_str);
        return -EINVAL;
    }

    return 0;
}

/*----------------------------------------------------------------------------
 * File Operation Utilities
 *----------------------------------------------------------------------------*/

// Performs a robust read, reading out all bytes from the buffer
int ZynqHardwareDriver::RobustRead(int fd, uint8_t *buf, int buf_size)
{
    int bytes_remain, bytes_read;
    int buf_offset;

    // Read out the bytes into the buffer, accounting for EINTR
    bytes_remain = buf_size;
    while (true)
    {
        buf_offset = buf_size - bytes_remain;
        bytes_read = read(fd, buf + buf_offset, bytes_remain);
        bytes_remain = (bytes_read > 0) ? bytes_remain - bytes_read
                                        : bytes_remain;

        /* If we were interrupted by a signal, then repeat the read. Otherwise,
         * if we encountered a different error or reached EOF then stop. */
        if (bytes_read < 0 && bytes_read != -EINTR) {
            return bytes_read;
        } else if (bytes_read == 0) {
            return buf_size - bytes_remain;
        }
    }

    // We should never reach here
    assert(false);
    return -EINVAL;
}

int ZynqHardwareDriver::RobustWrite(int fd, uint8_t *buf, int buf_size)
{
    int bytes_remain, bytes_written;
    int buf_offset;

    // Read out the bytes into the buffer, accounting for EINTR
    bytes_remain = buf_size;
    while (true)
    {
        buf_offset = buf_size - bytes_remain;
        bytes_written = write(fd, buf + buf_offset, bytes_remain);
        bytes_remain = (bytes_written > 0) ? bytes_remain - bytes_written
                                           : bytes_remain;

        /* If we were interrupted by a signal, then repeat the write. Otherwise,
         * if we encountered a different error or reached EOF then stop. */
        if (bytes_written < 0 && bytes_written != -EINTR) {
            return bytes_written;
        } else if (bytes_written == 0) {
            return buf_size - bytes_remain;
        }
    }

    // We should never reach here
    assert(false);
    return -EINVAL;
}

#endif

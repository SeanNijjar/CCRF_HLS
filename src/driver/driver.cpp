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
void ZynqHardwareDriver::SendJobRequest(JobPackage &job)
{
    dma_transfer transfer_details;
    transfer_details.input_size = 0;
    transfer_details.output_channel = tx_chans->data[0];
    transfer_details.output_fd = open(output_path.c_str(), O_WRONLY|O_CREAT|O_TRUNC, S_IWUSR|S_IRUSR|S_IRGRP|S_IWGRP|S_IROTH);
    ASSERT(transfer_details.output_fd >= 0, "Couldn't open output file for transfer to FPGA");
    std::cout << "sizeof(JobPackage)=" << sizeof(job) << std::endl;
    transfer_details.output_size = sizeof(job);//JobDescriptor::BytesNeededForJobDescriptor(&job.job_descriptor) + sizeof(JOB_ID_T); //JobDescriptor::BytesNeededForEntireJob(job) + sizeof(JOB_ID_T);
    transfer_details.output_buf = &job;

    TransferFile(axidma_dev, transfer_details);
    close(transfer_details.output_fd);
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
    Driver(incoming_queue, outgoing_queue)
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
        goto close_output;
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

    goto end;

destroy_axidma:
    axidma_destroy(axidma_dev);
close_output:
    assert(close(trans.output_fd) == 0);
close_input:
    assert(close(trans.input_fd) == 0);
end: {}
}

void ZynqHardwareDriver::ReadResponseQueuePacket(uint8_t *response_message_buffer, uint64_t bytes_to_read)
{
    dma_transfer transfer_info;
    transfer_info.output_size = 0;
    transfer_info.input_size = bytes_to_read;
    transfer_info.input_channel = rx_chans->data[0];
    transfer_info.input_fd = open(input_path.c_str(), O_RDONLY);
    transfer_info.input_buf = response_message_buffer;
    TransferFile(axidma_dev, transfer_info);
    close(transfer_info.input_fd);
}

int ZynqHardwareDriver::TransferFile (
    axidma_dev_t dev, 
    dma_transfer trans
) {
    int rc;

    // Allocate a buffer for the input file, and read it into the buffer
    if (trans.input_size) {
        trans.input_buf = axidma_malloc(dev, trans.input_size);
        if (trans.input_buf == NULL) {
            std::cerr << "Failed to allocate the input buffer" << std::endl;
            rc = -ENOMEM;
        }
    }

    if (trans.output_size) {
        // Allocate a buffer for the output file
        trans.output_buf = axidma_malloc(dev, trans.output_size);
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
            rc = axidma_twoway_transfer(dev, trans.input_channel, trans.input_buf,
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
            axidma_free(dev, trans.input_buf, trans.input_size);
        }
        if (free_output_buf) {
            axidma_free(dev, trans.output_buf, trans.output_size);
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
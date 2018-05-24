#ifndef DRIVER_H
#define DRIVER_H

#include "job_package.hpp"
#include "types.hpp"
#ifdef ZYNQ_COMPILE
#include "libaxidma.hpp"
#endif
#include <string>
#include <vector>

// TODO: Ideally this can be passed to the FPGA
extern uintptr_t CCRF_SCRATCHPAD_START_ADDR;
extern uintptr_t CCRF_SCRATCHPAD_END_ADDR;

class Driver
{
  public:
    Driver(std::vector<JobPackage> &incoming_queue, std::vector<JOB_STATUS_MESSAGE> &outgoing_queue) :
        incoming_job_request_queue(incoming_queue), outgoing_job_response_queue(outgoing_queue)
    {
    }

  public:
    std::vector<JobPackage> &incoming_job_request_queue;
    std::vector<JOB_STATUS_MESSAGE> &outgoing_job_response_queue;
};

/**
 * The below class was written by us, however, some of the helper functions were 
 * used from an AXI-DMA sample from CMU. The following functions were written in
 * axidma_transfer.c and util.[ch] in the sample code:
   *  int ParseInt(char option, char *arg_str, int *data);
   *  int ParseDouble(char option, char *arg_str, double *data);
   *  int RobustRead(int fd, char *buf, int buf_size);
   *  int RobustWrite(int fd, char *buf, int buf_size);
   *  static int transfer_file(axidma_dev_t dev, struct dma_transfer *trans,
          char *output_path)
   * struct dma_transfer
 **/

#ifdef ZYNQ_COMPILE
class ZynqHardwareDriver : public Driver
{
  public:
    ZynqHardwareDriver(std::vector<JobPackage> &incoming_queue, 
                       std::vector<JOB_STATUS_MESSAGE> &outgoing_queue);

  public:
    bool SendJobRequest(JobPackage &job);

    bool ResponseQueueHasData(const int minimum_bytes);
    bool ReadResponseQueuePacket(uint8_t *response_message_buffer, uint64_t bytes_to_read);
    const uint64_t GetDMAFileSize(std::string dma_file_path);

  private:
    struct dma_transfer {
        int input_fd;           // The file descriptor for the input file
        int input_channel;      // The channel used to send the data
        int input_size;         // The amount of data to send
        void *input_buf;        // The buffer to hold the input data
        int output_fd;          // The file descriptor for the output file
        int output_channel;     // The channel used to receive the data
        int output_size;        // The amount of data to receive
        void *output_buf;       // The buffer to hold the output
    };

    int input_channel;
    int output_channel;
    long int output_size; // in Bytes 
    axidma_dev_t axidma_dev; // transfer
    struct dma_transfer trans;
    const array_t *tx_chans, *rx_chans;
    std::string input_path, output_path;

    void FlushHardware();
    int TransferFile(axidma_dev_t dev, dma_transfer trans);
    int ParseInt(char option, uint8_t *arg_str, int *data);
    int ParseDouble(char option, uint8_t *arg_str, double *data);
    int RobustRead(int fd, uint8_t *buf, int buf_size);
    int RobustWrite(int fd, uint8_t *buf, int buf_size);
};
#endif

#endif 
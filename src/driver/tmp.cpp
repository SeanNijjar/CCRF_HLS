#include <time.h>
#include <stdbool.h>
#include <assert.h>

#include <sys/stat.h>           // Open() system call
#include <sys/types.h>          // Types for open()
#include <string.h>             // Memory setting and copying
#include <getopt.h>             // Option parsing
#include <errno.h>              // Error codes

#include "util.h"               // Miscellaneous utilities
#include "conversion.h"         // Convert bytes to MiBs
#include "libaxidma.h"          // Interface ot the AXI DMA library
#include "axidma_ctrl_func.h"

#define TRANS_BYTES 100000

int main()
{
//    char test_array[TRANS_BYTES] = {1,0,1,0,51,62,7,8,9,10,11,12,13,14,15,16,17};
//    char test_array[TRANS_BYTES];
//    for (int i = 0; i < TRANS_BYTES; i++) {
//	test_array[i] = (char)(i % 256);
//    }

    int rc;
    axidma_dev_t axidma_dev;
    int input_channel, input_size, output_channel, output_size;
    void *input_buf,*output_buf;
    const array_t *tx_chans, *rx_chans;

    // Initialize the AXIDMA device/////////////////////////////////////////////
    axidma_dev = axidma_init();
    if (axidma_dev == NULL) {
        fprintf(stderr, "Error: Failed to initialize the AXI DMA device.\n");
        rc = 1;
        goto destroy_axidma;
    }
    ////////////////////////////////////////////////////////////////////////////
    // Allocating Buffers///////////////////////////////////////////////////////
    input_size = TRANS_BYTES*sizeof(char);
    output_size = input_size;
    input_buf = axidma_malloc(axidma_dev, input_size);
    if (input_buf == NULL) {
        fprintf(stderr, "Failed to allocate the input buffer.\n");
        rc = -ENOMEM;
        goto ret;
    }
    memset(input_buf, 1, input_size);
    output_buf = axidma_malloc(axidma_dev, output_size);
    if (output_buf == NULL) {
        fprintf(stderr, "Failed to allocate the input buffer.\n");
        rc = -ENOMEM;
        goto ret;
    }
    ////////////////////////////////////////////////////////////////////////////




    //copy images to input buff, here test_array should be changed to the buffer of real input images////////////
    //memcpy(input_buf,test_array,TRANS_BYTES*sizeof(char));
    ////////////////////////////////////////////////////////////////////////////








    // Get the tx and rx channels///////////////////////////////////////////////
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

    input_channel = tx_chans->data[0];
    output_channel = rx_chans->data[0];
    /////////////////////////////////////////////////////////////////////////////
    printf("test\n");

//######################################################################################################################
    //second DMA write data to PL DMA///////////////////////////////////////////////////////////////
    off_t dmactrlreg_base = DMA_BASE;
    int dmactrlreg_size = DMA_RegSize;
    char *dma_vptr;
    int fd;
    int write_addr = 0x0;
    int write_size = TRANS_BYTES;
    struct timespec ts_start, ts_end;
    int wait_time;

    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) != -1) {
	dma_vptr = mmap(NULL, dmactrlreg_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, dmactrlreg_base);
	printf("Memory mapped at address %p.\n", dma_vptr);
	clock_gettime(CLOCK_MONOTONIC, &ts_start);

	//reset before use////////////////////////////////
	axilite_write(dma_vptr, DMA_S2MM_OFFSET+DMA_CtrlReg_OFFSET, axilite_read(dma_vptr, DMA_S2MM_OFFSET+DMA_CtrlReg_OFFSET) | XAXIDMA_CR_RESET_MASK );
	axilite_write(dma_vptr, DMA_MM2S_OFFSET+DMA_CtrlReg_OFFSET, axilite_read(dma_vptr, DMA_MM2S_OFFSET+DMA_CtrlReg_OFFSET) | XAXIDMA_CR_RESET_MASK );
	//////////////////////////////////////////////////

	wait_time = 0;

	//first DMA write PS mem data to second DMA/////////////////////////////////////////////////////
	rc = axidma_oneway_transfer(axidma_dev, input_channel, input_buf, input_size, false);	
	if (rc < 0) {
		fprintf(stderr, "w : DMA read write transaction failed.\n");
		goto destroy_axidma;
	}
	////////////////////////////////////////////////////////////////////////////////////////////////

	DMA_S2MM(dma_vptr, write_addr, 0x0, write_size);

	do {
		sleep(0.001);
		wait_time++;
	}
	while (!(axilite_read(dma_vptr, DMA_S2MM_OFFSET+DMA_StatsReg_OFFSET) & XAXIDMA_IDLE_MASK) && wait_time < 50000); /*
										check when the transimitting finished*/
	if ( wait_time == 50000 ) {
		printf("timeout!\n");
		goto free_buf;
	} else {
		clock_gettime(CLOCK_MONOTONIC, &ts_end);
		timespec_sub(&ts_end, &ts_start);
		printf("Spent %ld.%09ld seconds for DMA write (%d cycles)\n", ts_end.tv_sec, ts_end.tv_nsec, wait_time);
	}
    /////////////////////////////////////////////////////////////////////////////////////////////////


    /*Data is all sent to PL DDR here, so this is the location to start HDR processing, 
	maybe ccrf scheduler should start from here?
        ################################################################
	code for HDR processing control
	code for HDR processing control
	code for HDR processing control
	code for HDR processing control
	code for HDR processing control
	code for HDR processing control
	code for HDR processing control
	################################################################
	after this is done, the final image should be ready in PL DDR
	so that again, in following lines two DMAs will be used to move data back to PS DDR
    */

    //second DMA read data from PL DDR///////////////////////////////////////////////////////////////

        clock_gettime(CLOCK_MONOTONIC, &ts_start);

	//the write_addr and write_size should be modified, it should be the addr and size of the final image in PL DDR
	DMA_MM2S(dma_vptr, write_addr, 0x0, write_size);
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//don't wait
	close(fd);
    } else {
	printf("memory map error!!!\n");
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////

//####################################################################################################################
    //first DMA receive data from second DMA and write to PS DDR//////////////////////////////////////
    rc = axidma_oneway_transfer(axidma_dev, output_channel, output_buf, output_size, true);
    if (rc < 0) {
        fprintf(stderr, "r : DMA read write transaction failed.\n");
        goto destroy_axidma;
    }

    char * i_buf = input_buf;
    char * o_buf = output_buf;
    bool flag = false;
    for (int i = 0; i < TRANS_BYTES; i++, i_buf++, o_buf++) {
	if (*i_buf != *o_buf) {
		flag = true;
		break;
	}
    }
    if (flag) {
	printf("mismatch\n");
    } else {
	printf("all matched\n");
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////

    rc = (rc < 0) ? -rc : 0;
    if (rc == 0) {
	printf("success!\n");
    }
destroy_axidma:
    axidma_destroy(axidma_dev);
free_buf:
    axidma_free(axidma_dev, output_buf, output_size);
    axidma_free(axidma_dev, input_buf, input_size);
ret:
    return rc;
}
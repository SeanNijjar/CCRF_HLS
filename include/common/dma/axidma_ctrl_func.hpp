#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>

int axilite_read(volatile void* map_base, int offset);
void axilite_write(volatile void* map_base, int offset, int value);
void DMA_Write(volatile void* map_base, int address_low, int address_high, int size);
void DMA_Read(volatile void* map_base, int address_low, int address_high, int size);
void timespec_sub(struct timespec *t1, const struct timespec *t2);
#define DMA_BASE        0xA0010000
#define DMA_RegSize     0x00001000

#define DMA_TX_OFFSET   0x00000000
#define DMA_RX_OFFSET   0x00000030

#define DMA_CtrlReg_OFFSET      0x00000000   /**< Channel control */
#define DMA_StatsReg_OFFSET     0x00000004   /**< Status */

#define DMA_MemLoc_OFFSET       0x00000018
#define DMA_MemLoc_MSB_OFFSET   0x0000001C

#define DMA_BUFFLEN_OFFSET      0x00000028

// @name Bitmasks of DMA_CtrlReg_OFFSET register
#define XAXIDMA_CR_RUNSTOP_MASK 0x00000001 /**< Start/stop DMA channel */
#define XAXIDMA_CR_RESET_MASK   0x00000004 /**< Reset DMA engine */

// @name Bitmasks of DMA_StatsReg_OFFSET register
#define XAXIDMA_HALTED_MASK             0x00000001  /**< DMA channel halted */
#define XAXIDMA_IDLE_MASK               0x00000002  /**< DMA channel idle */
#define XAXIDMA_ERR_SLAVE_MASK          0x00000020  /**< Datamover slave err */
#define XAXIDMA_ERR_DECODE_MASK         0x00000040  /**< Datamover decode*/
#define XAXIDMA_ERR_ALL_MASK            0x00000060  /**< All errors */

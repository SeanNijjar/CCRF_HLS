#include "axidma_ctrl_func.hpp"
#include <assert.h>
int axilite_read(volatile void* map_base, int offset) {
	volatile void* virt_addr = (volatile void*)((char*)map_base + offset);
	int value = *((unsigned int *) virt_addr);
	return value;
}

void axilite_write(volatile void* map_base, int offset, int value) {
	volatile void* virt_addr = (volatile void*)((char*)map_base + offset);
	*((unsigned int *) virt_addr) = value;
}

void DMA_MM2S(volatile void* map_base, int address_low, int address_high, int size) {
	axilite_write(map_base, DMA_MM2S_OFFSET+DMA_CtrlReg_OFFSET, axilite_read(map_base, DMA_MM2S_OFFSET+DMA_CtrlReg_OFFSET) | XAXIDMA_CR_RUNSTOP_MASK); /* Start the MM2S channel running by setting the run/stop bit to 1*/
	axilite_write(map_base, DMA_MM2S_OFFSET+DMA_MemLoc_OFFSET, address_low);
	axilite_write(map_base, DMA_MM2S_OFFSET+DMA_MemLoc_MSB_OFFSET, address_high);
	axilite_write(map_base, DMA_MM2S_OFFSET+DMA_BUFFLEN_OFFSET, size);
}

void DMA_S2MM(volatile void* map_base, int address_low, int address_high, int size) {
	axilite_write(map_base, DMA_S2MM_OFFSET+DMA_CtrlReg_OFFSET, axilite_read(map_base, DMA_S2MM_OFFSET+DMA_CtrlReg_OFFSET) | XAXIDMA_CR_RUNSTOP_MASK); /* Start the S2MM channel running by setting the run/stop bit to 1*/
	axilite_write(map_base, DMA_S2MM_OFFSET+DMA_MemLoc_OFFSET, address_low);
	axilite_write(map_base, DMA_S2MM_OFFSET+DMA_MemLoc_MSB_OFFSET, address_high);
	axilite_write(map_base, DMA_S2MM_OFFSET+DMA_BUFFLEN_OFFSET, size);
}

void timespec_sub(struct timespec *t1, const struct timespec *t2)
{
  assert(t1->tv_nsec >= 0);
  assert(t1->tv_nsec < 1000000000);
  assert(t2->tv_nsec >= 0);
  assert(t2->tv_nsec < 1000000000);
  t1->tv_sec -= t2->tv_sec;
  t1->tv_nsec -= t2->tv_nsec;
  if (t1->tv_nsec >= 1000000000)
  {
    t1->tv_sec++;
    t1->tv_nsec -= 1000000000;
  }
  else if (t1->tv_nsec < 0)
  {
    t1->tv_sec--;
    t1->tv_nsec += 1000000000;
  }
}

#ifndef IOCTL_H
#define IOCTL_H

#include <linux/ioctl.h>

#define IOCTL_MAGIC_NUM 'q'

#define INITIALIZE  _IOR('q',2, init_t *)

#define CLEAN_UP _IO('q',3)

#define AXI_DMA_WR _IO('q',4)

#define AXI_DMA_RD _IOR('q',5,int *)

#define GET_OFFSET _IOW('q',6,int *)


#endif
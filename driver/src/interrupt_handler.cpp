#include <linux/interrupt.h>

int request_irq(unsigned int irq,
                irq_handler_t handler,
                unsigned long flags,
                const char *name,
                void *dev)
{

}

// what does the interrupt handler need to do?
// 1) Register that a job is done - register remote buffer for copy back
// 2) Register that the remote processor can no longer accept jobs
// 3) 
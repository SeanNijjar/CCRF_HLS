#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");

#define DEVICE_NAME "xilaxitimer"

#define IRQ_NUM		91

#define XIL_AXI_TIMER_BASEADDR 0x41C00000
#define XIL_AXI_TIMER_HIGHADDR 0x41C0FFFF

#define XIL_AXI_TIMER_TCSR_OFFSET		0x0
#define XIL_AXI_TIMER_TLR_OFFSET		0x4
#define XIL_AXI_TIMER_TCR_OFFSET		0x8
#define XIL_AXI_TIMER_CSR_INT_OCCURED_MASK	0x00000100

#define XIL_AXI_TIMER_CSR_CASC_MASK		0x00000800
#define XIL_AXI_TIMER_CSR_ENABLE_ALL_MASK	0x00000400
#define XIL_AXI_TIMER_CSR_ENABLE_PWM_MASK	0x00000200
#define XIL_AXI_TIMER_CSR_INT_OCCURED_MASK	0x00000100
#define XIL_AXI_TIMER_CSR_ENABLE_TMR_MASK	0x00000080
#define XIL_AXI_TIMER_CSR_ENABLE_INT_MASK	0x00000040
#define XIL_AXI_TIMER_CSR_LOAD_MASK		0x00000020
#define XIL_AXI_TIMER_CSR_AUTO_RELOAD_MASK	0x00000010
#define XIL_AXI_TIMER_CSR_EXT_CAPTURE_MASK	0x00000008
#define XIL_AXI_TIMER_CSR_EXT_GENERATE_MASK	0x00000004
#define XIL_AXI_TIMER_CSR_DOWN_COUNT_MASK	0x00000002
#define XIL_AXI_TIMER_CSR_CAPTURE_MODE_MASK	0x00000001

#define TIMER_CNT	0xF8000000

// Need 3 registers
typedef unsigned long reg_addr_t;
const reg_addr_t INTERRUPT_SIGNAL_ADDR = ; // might not be needed - I think this information is stored directly on the interrupt ID
const reg_addr_t INTERRUPT_JOB_ID_REG_ADDR = ; // Contains unique ID of the job this interrupt relates to
const reg_addr_t INTERRUPT_TYPE_REG_ADDR = ; // Contains the state information being broadcasted for this job



static struct platform_device *pdev;
void *dev_virtaddr;
static int int_cnt;

static irqreturn_t 

static irqreturn_t TIMER_INTERRUPT_HANDLER(int irq, void *dev)
{
  unsigned int data;
  
  /* 
   * Check Timer Counter Value
   */
  data = ioread32(dev_virtaddr + XIL_AXI_TIMER_TCR_OFFSET);
  printk("xilaxitimer_isr: Interrupt Occurred ! Timer Count = 0x%08X\n",data);
  
  /* 
   * Clear Interrupt
   */
  data = ioread32(dev_virtaddr + XIL_AXI_TIMER_TCSR_OFFSET);
  iowrite32(data | XIL_AXI_TIMER_CSR_INT_OCCURED_MASK,
	    dev_virtaddr + XIL_AXI_TIMER_TCSR_OFFSET);
  
  /* 
   * Disable Timer after 100 Interrupts
   */
  int_cnt++;
  
  if (int_cnt>=100)
    {
      printk("xilaxitimer_isr: 100 interrupts have been occurred. Disabling timer");
      data = ioread32(dev_virtaddr + XIL_AXI_TIMER_TCSR_OFFSET);
      iowrite32(data & ~(XIL_AXI_TIMER_CSR_ENABLE_TMR_MASK),
		dev_virtaddr + XIL_AXI_TIMER_TCSR_OFFSET);
    }
  
  return IRQ_HANDLED;
}

static int __init SetupInterruptHandlers(void)
{
  unsigned int data;
  
  int_cnt = 0;
  
  printk(KERN_INFO "SetupInterruptHandlers: Initialize Module \"%s\"\n", DEVICE_NAME);
  
  /* 
   * Register ISR
   */
  if (request_irq(IRQ_NUM, TIMER_INTERRUPT_HANDLER, 0, DEVICE_NAME, NULL)) {
    printk(KERN_ERR "SetupInterruptHandlers: Cannot register IRQ %d\n", IRQ_NUM);
    return -EIO;
  }
  else {
    printk(KERN_INFO "SetupInterruptHandlers: Registered IRQ %d\n", IRQ_NUM);
  }
  
  /* 
   * Map Physical address to Virtual address
   */
  dev_virtaddr = ioremap_nocache(XIL_AXI_TIMER_BASEADDR,
				 XIL_AXI_TIMER_HIGHADDR - XIL_AXI_TIMER_BASEADDR + 1);
  
  /* 
   * Set Timer Counter
   */
  iowrite32(TIMER_CNT,
	    dev_virtaddr + XIL_AXI_TIMER_TLR_OFFSET);
  data = ioread32(dev_virtaddr + XIL_AXI_TIMER_TLR_OFFSET);
  printk("SetupInterruptHandlers: Set timer count 0x%08X\n",data);
  
  /* 
   * Set Timer mode and enable interrupt
   */
  iowrite32(XIL_AXI_TIMER_CSR_LOAD_MASK,
	    dev_virtaddr + XIL_AXI_TIMER_TCSR_OFFSET);
  iowrite32(XIL_AXI_TIMER_CSR_ENABLE_INT_MASK | XIL_AXI_TIMER_CSR_AUTO_RELOAD_MASK,
	    dev_virtaddr + XIL_AXI_TIMER_TCSR_OFFSET);

  /* 
   * Register Device Module
   */
  pdev = platform_device_register_simple(DEVICE_NAME, 0, NULL, 0);              
  if (pdev == NULL) {                                                     
    printk(KERN_WARNING "SetupInterruptHandlers: Adding platform device \"%s\" failed\n", DEVICE_NAME);
    kfree(pdev);                                                             
    return -ENODEV;                                                          
  }
  
  /* 
   * Start Timer
   */
  data = ioread32(dev_virtaddr + XIL_AXI_TIMER_TCSR_OFFSET);
  iowrite32(data | XIL_AXI_TIMER_CSR_ENABLE_TMR_MASK,
	    dev_virtaddr + XIL_AXI_TIMER_TCSR_OFFSET);

  return 0;
} 

static void __exit UnregisterInterruptHandlers(void)
{
    /* 
    * Exit Device Module
    */
    iounmap(dev_virtaddr);
    free_irq(IRQ_NUM, NULL);
    platform_device_unregister(pdev);                                             
    printk(KERN_INFO "UnregisterInterruptHandlers: Exit Device Module \"%s\".\n", DEVICE_NAME);

}

// typedef irqreturn_t (*irq_handler_t)(int, void *);

// This call allocates interrupt resources and enables the interrupt line and IRQ handling.
// From the point this call is made your handler function may be invoked. Since your handler
// function must clear any interrupt the board raises, you must take care both to initialise
// your hardware and to set up the interrupt handler in the right order. 
//
// Dev_id must be globally unique. Normally the address of the device data structure is used
// as the cookie. Since the handler receives this value it makes sense to use it. 
//
// If your interrupt is shared you must pass a non NULL dev_id as this is required when freeing the interrupt.
int request_irq(unsigned int irq, // interrupt line to allocate
                irq_handler_t handler, // interrupt handler - the function that will handle this interrupt
                unsigned long flags, // interrupt type flags
                const char *name, // ASCII name of the claiming device
                void *dev) // a handle passed back to the handler function
{

}

// Remove an interrupt handler. The handler is removed and if the interrupt line is no
// longer in use by any driver it is disabled. On a shared IRQ the caller must ensure
// the interrupt is disabled on the card it drives before calling this function. The
// function does not return until any executing interrupts for this IRQ have completed.
// irq - Interrupt line to free
// dev_id - Device identity to free
void free_irq (	unsigned int irq, void * dev_id);

// Use when processing an interrupt?
// Undoes the effect of one call to disable_irq. If this matches the last disable, processing of interrupts on this IRQ line is re-enabled.
void enable_irq (unsigned int irq);

// Use when processing an interrupt to stop others from interrupting the interrupt?
// Disable the selected interrupt line. Enables and Disables are nested. This function
// waits for any pending IRQ handlers for this interrupt to complete before returning.
// If you use this function while holding a resource the IRQ handler may need you will deadlock.
void disable_irq (unsigned int irq);


// SAMPLE IRQ REQUEST
if (request_irq(irqn, my_interrupt, IRQF_SHARED, "my_device", my_dev)) {
    printk(KERN_ERR "my_device: cannot register IRQ %d\n", irqn);
    return -EIO;
}

// WRITING AN INTERRUPT HANDLER:
static irqreturn_t intr_handler(int irq, void *dev)

// what does the interrupt handler need to do?
// 1) Register that a job is done - register remote buffer for copy back
// 2) Register that the remote processor can no longer accept jobs
// 3) 

module_init(xilaxitimer_init);
module_exit(xilaxitimer_edit);

MODULE_AUTHOR ("Xilinx");
MODULE_DESCRIPTION("Test Driver for Zynq PL AXI Timer.");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("custom:xilaxitimer");
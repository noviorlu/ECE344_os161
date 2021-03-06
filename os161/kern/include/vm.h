#ifndef _VM_H_
#define _VM_H_

#include <machine/vm.h>
#include "opt-dumbvm.h"

#include <coremap.h>
#include <swapdisk.h>
#include <tlbfunc.h>

/*
 * VM system-related definitions.
 *
 * You'll probably want to add stuff here.
 */


/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted*/


/* Initialization function */
void vm_postbootstrap(void);
void vm_prebootstrap(void);

/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress);

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(int npages);
void free_kpages(vaddr_t addr);

typedef enum{
    STACK,
    HEAP,
    BASE1,
    BASE2,
}REGION;

#endif /* _VM_H_ */

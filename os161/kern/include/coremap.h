#ifndef _COREMAP_H_
#define _COREMAP_H_

#include <synch.h>
#include <types.h>
#include <vnode.h>
#include <lib.h>
#include <../arch/mips/include/spl.h>

#include <pagetable.h>
#include <tlbfunc.h>


typedef int ppage;

/*
 * status used in finding free PPage that can be allocated
 */
typedef enum{
    FREE,
    KERNEL,
    USER
}CORESTATUS;

struct coremap_entry{
    
    CORESTATUS page_stat;
    
    // Only used when allocating swapout Physical page to disk
    // need to use vaddr to evacuate TLB entry
    vaddr_t vaddr;

    PTE* entry_ptr;
    
    /* 
     * Initalized 0, only when fork will add to this.
     * When a process forked, during as_copy, program will come to each 
     * physical Page that belongs to that Process and add one to shared. 
     * 
     * This value is also examined when as_destory() execute. Shared = 0 
     * means only this ppage is used by the process that currently examine. 
     * In this case this  page can be freed and remember to clear TLB value.
    */
    int shared;
    
    /*
     * Not used Yet, might be use by Optimization Thread
     */
    int modify;
};
/* 
 * ram_getsize returns a paddr that is maskable by TLBLO_PPAGE, this 
 * means that the last 12 bits is always 0. And while mapping
 * core_endpaddr the last 12 bit should also be 0.
 */
int core_startpaddr;
/*
 * endpaddr is probabily core_startpaddr + PAGE_SIZE
 */
int core_endpaddr;

int core_total;
int core_clock;
struct coremap_entry* coremap;
struct semaphore* core_mutex;

void coremap_preboostrap();
void coremap_postboostrap();
ppage alloc_ppage(unsigned long npages);
void free_ppage(paddr_t ppage);
void saveCoremapInfo(
    ppage idx, 
    CORESTATUS page_stat, 
    vaddr_t vaddr, 
    PTE* entry_ptr, 
    int shared, 
    int modify);



paddr_t PPAGE_TO_PADDR(ppage page);
ppage PADDR_TO_PPAGE(paddr_t paddr);

#endif /* _COREMAP_H_ */

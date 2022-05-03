#ifndef _TLBFUNC_H_
#define _TLBFUNC_H_

#include <../arch/mips/include/tlb.h>

void tlb_boostrap();
void tlb_insert(vaddr_t vaddr, paddr_t paddr);
void tlb_disable_all();

void tlb_disable_vaddr(vaddr_t vaddr);



#endif /* _TLBFUNC_H_ */

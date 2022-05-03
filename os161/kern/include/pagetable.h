#ifndef _PAGETABLE_H_
#define _PAGETABLE_H_

#include <types.h>
#include <lib.h>

#define PTENTRY     16
#define PTSHIFT     4
#define PTLAYERS    5
// #define PTENTRYSTART(vaddr) (vaddr >> 12)
#define PTMASK      0x0000000f

// #define PTE_TO_PADDR(entry_ptr) (entry_ptr & ~ONDISK)
typedef struct{
    paddr_t paddr;
    int share;
}PTE;

typedef struct{
    vaddr_t levelPT[PTENTRY];
}PT;
#define PT_PAGE         0xfffff000
#define PT_READWRITE    0x00000400
#define PT_VALID        0x00000200
#define PT_SWAP         0x00000001

int examingPT(vaddr_t* pt, vaddr_t faultaddress, PTE** entry_ptr);
void printPTE(vaddr_t* pt, vaddr_t faultaddress);
void setPTE(vaddr_t* pt, vaddr_t faultaddress);
int readPTE(vaddr_t* pt, vaddr_t faultaddress, vaddr_t** last_layer_pt);

void free_PT(vaddr_t* pt);
void cpy_PT(vaddr_t* pt, vaddr_t* newpt);
#endif /* _PAGETABLE_H_ */
